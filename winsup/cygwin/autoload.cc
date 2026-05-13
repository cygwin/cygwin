/* autoload.cc: all dynamic load stuff.

This file is part of Cygwin.

This software is a copyrighted work licensed under the terms of the
Cygwin license.  Please consult the file "CYGWIN_LICENSE" for
details. */

#include "winsup.h"
#include "miscfuncs.h"
#include <fenv.h>
#define USE_SYS_TYPES_FD_SET
#include <winsock2.h>

bool NO_COPY wsock_started;

/* Macro for defining "auto-load" functions.
 * Note that this is self-modifying code *gasp*.
 *
 * The first invocation of a auto-load function will trigger the loading of the
 * DLL.  This will then be followed by the discovery of the procedure's entry
 * point.  The pointer the auto-load routine calls through (located via the
 * return address) is then updated to transfer directly to the DLL function on
 * the next invocation.
 *
 * Subsequent calls to routines whose transfer address has not been
 * determined will skip the "load the dll" step, starting at the
 * "discovery of the entry point" step.
 *
 * So, immediately following the code to call to one of the above routines
 * we have a struct func_info:
 *
 *  DLL info (8 bytes)	 Pointer to a block of information concerning
 *			 the DLL (see below).
 *  DLL notimp (2 bytes) Bool value flagging that non-existence of this
 *			 function is not a fatal error.
 *  DLL error (2 bytes)  Error value returned if function load fails.
 *			 Depends on the return type of the function.
 *			 Default is 0 == BOOL FALSE or == HANDLE NULL or
 *			 == Pointer NULL.
 *  func addr (8 bytes)  Address of the actual Win32 function.  For the
 *			 reason why this is necessary, see the below
 *			 description of the load_state.
 *  func name (n bytes)	 asciz string containing the name of the function
 *			 to be loaded.
 *
 * The DLL info block (struct dll_info) consists of the following
 *
 *  load_state (8 bytes) Pointer to the routine used to eventually invoke the
 *			 function.
 *
 *			 Initially, this points to an init function which loads
 *			 the DLL, then calls a function to which calls
 *			 GetProcAddress for the given DLL handle and function
 *			 name
 *
 *			 If the DLL load has already been done, only the
 *			 GetProcAddress part is done.
 *
 *			 The jump is not tweaked directly. Rather, the address
 *			 of the Win32 function is stored in the aforementioned
 *			 Win32 function address slot and fetched there for an
 *			 indirect jump.  (This is necessary to access the full
 *			 64-bit address space.)
 *
 *  DLL handle (8 bytes) The handle to use when loading the DLL.
 *  DLL locker (4 bytes) Word to use to avoid multi-thread access during
 *			 initialization.
 *  extra init (8 bytes) Extra initialization function.
 *  DLL name (2n bytes)	 WCHAR string containing the name of the DLL.
 */

#if defined(__x86_64__)
# define WORD64 ".quad"
#elif defined(__aarch64__)
# define WORD64 ".xword"
#else
# error unimplemented for this target
#endif

/* LoadDLLprime is used to prime the DLL info information, providing an
   additional initialization routine to call prior to calling the first
   function.

   The macro WORD64 stands in for .quad/.xword, and .balign (which means
   "align to N bytes" on all targets, unlike .align) is used for
   alignment.  */
#define LoadDLLprime(dllname, init_also, no_resolve_on_fork) __asm__ ("\n\
.ifndef " #dllname "_primed                              \n\
  .section   .data_cygwin_nocopy,\"w\"                   \n\
  .balign    8                                           \n\
." #dllname "_info:                                      \n\
  " WORD64 "   _std_dll_init                             \n\
  " WORD64 "   " #no_resolve_on_fork "                   \n\
  .long      -1                                          \n\
  .balign    8                                           \n\
  " WORD64 "   " #init_also "                            \n\
  .string16  \"" #dllname ".dll\"                        \n\
  .text                                                  \n\
  .set       " #dllname "_primed, 1                      \n\
.endif                                                   \n\
");

/* Standard DLL load macro.  May invoke a fatal error if the function isn't
   found. */
#define LoadDLLfunc(name, dllname) \
  LoadDLLfuncEx (name, dllname, 0)
#define LoadDLLfuncEx(name, dllname, notimp) \
  LoadDLLfuncEx2(name, dllname, notimp, 0)
#define LoadDLLfuncEx2(name, dllname, notimp, err) \
  LoadDLLfuncEx3(name, dllname, notimp, err, 0)

/* Main DLL setup stuff. */
#if defined(__x86_64__)
#define LoadDLLfuncEx3(name, dllname, notimp, err, no_resolve_on_fork) \
  LoadDLLprime (dllname, dll_func_load, no_resolve_on_fork) \
  __asm__ ("						\n\
  .section	." #dllname "_autoload_text,\"wx\"	\n\
  .global	" #name "				\n\
  .global	_win32_" #name "			\n\
  .align	16					\n\
" #name ":						\n\
_win32_" #name ":					\n\
  movq		3f(%rip),%rax				\n\
  jmp		*%rax					\n\
1:movq		2f(%rip),%rax				\n\
  push		%rbp		# Keep 16 byte aligned	\n\
  push		%r9					\n\
  push		%r8					\n\
  push		%rdx					\n\
  push		%rcx					\n\
  call		*(%rax)					\n\
2:.quad		." #dllname "_info			\n\
  .hword	" #notimp "				\n\
  .hword	((" #err ") & 0xffff)			\n\
3:.quad		1b					\n\
  .asciz	\"" #name "\"				\n\
  .text							\n\
");
#elif defined(__aarch64__)
#define LoadDLLfuncEx3(name, dllname, notimp, err, no_resolve_on_fork) \
  LoadDLLprime (dllname, dll_func_load, no_resolve_on_fork) \
  __asm__ ( "\n\
  .section   ." #dllname "_autoload_text,\"wx\"          \n\
  .global    " #name "                                   \n\
  .global    _win32_" #name "                            \n\
  .p2align   4                                           \n\
" #name ":                                               \n\
_win32_" #name ":                                        \n\
  adr        x16, 3f          // x16 = &func_addr slot (label 3 == func_info+12)\n\
  ldr        x16, [x16]       // x16 = *(&func_addr) = label 1 initially, real addr later\n\
  br         x16              // fast-path branch into the resolved DLL func\n\
1:                                                       \n\
  sub        sp, sp, #80      // reserve 80 byte frame for caller's arg regs + LR\n\
  stp        x0, x1, [sp, #0]                            \n\
  stp        x2, x3, [sp, #16]                           \n\
  stp        x4, x5, [sp, #32]                           \n\
  stp        x6, x7, [sp, #48]                           \n\
  stp        x8, x30, [sp, #64] // x30 here == label 2 == &func_info\n\
  adr        x16, 2f          // x16 = &func_info\n\
  ldr        x17, [x16]       // x17 = func_info->dll (= dll_info*)\n\
  ldr        x17, [x17]       // x17 = dll_info->load_state (e.g. _std_dll_init)\n\
  blr        x17              // call thunk; LR = label 2 = arg to std_dll_init\n\
2:                                                      \n\
  .xword     ." #dllname "_info // +0  func_info.dll\n\
  .hword     " #notimp "        // +8  func_info.decoration[lo]\n\
  .hword     ((" #err ") & 0xffff) // +10 func_info.decoration[hi]\n\
3:                                                       \n\
  .xword     1b                // +12 func_info.func_addr (patched by dll_func_load)\n\
  .asciz     \"" #name "\"     // +20 func_info.name (asciz)\n\
  .text                                                  \n\
");
#else
#error unimplemented for this target
#endif

/* DLL loader helper functions used during initialization. */

/* The function which finds the address, given the name and overwrites
   the call so that future invocations go straight to the function in
   the DLL. */
extern "C" void dll_func_load () __asm__ ("dll_func_load");

/* Called by the primary initialization function "init_std_dll" to
   setup the stack and eliminate future calls to init_std_dll for other
   functions from this DLL.  */
extern "C" void dll_chain () __asm__ ("dll_chain");

extern "C" {

#if defined(__x86_64__)
__asm__ ("								\n\
	 .section .rdata,\"r\"							\n\
msg1:									\n\
	.ascii	\"couldn't dynamically determine load address for '%s' (handle %p), %E\\0\"\n\
									\n\
	 .text								\n\
	.p2align 4,,15							\n\
noload:									\n\
	movq	40(%rsp),%rdx	# Get the address of the information block\n\
	movl	8(%rdx),%eax	# Should we 'ignore' the lack		\n\
	test	$1,%eax		#  of this function?			\n\
	jz	1f		# Nope.					\n\
	andl	$0xffff0000,%eax# upper word (== desired return value)	\n\
	sarl	$16,%eax	# swap to low order word		\n\
	movl	%eax,32(%rsp)	# Save for later (in shadow space)	\n\
	movl	$127,%ecx	# ERROR_PROC_NOT_FOUND			\n\
	call	SetLastError	# Set it				\n\
	movl	32(%rsp),%eax	# Get back return value			\n\
	addq	$40,%rsp	# Revert stack				\n\
	pop	%r10		# Drop pointer to 'return address'	\n\
	pop	%rcx		# Restore arg registers			\n\
	pop	%rdx							\n\
	pop	%r8							\n\
	pop	%r9							\n\
	pop	%rbp		# ...and restore frame pointer		\n\
	ret			# Return				\n\
1:									\n\
	movq	(%rdx),%rax	# Handle value				\n\
	movq	8(%rax),%r8						\n\
	lea	20(%rdx),%rdx	# Location of name of function		\n\
	lea	msg1(%rip),%rcx	# The message				\n\
	call	api_fatal	# Print message. Never returns		\n\
									\n\
	.globl	dll_func_load						\n\
dll_func_load:								\n\
	movq	(%rsp),%rdx	# 'Return address' contains load info	\n\
	movq	(%rdx),%rcx	# Where handle lives			\n\
	movq	8(%rcx),%rcx	# Address of Handle to DLL		\n\
	addq	$20,%rdx	# Address of name of function to load	\n\
	subq	$40,%rsp	# Shadow space + 8 byte for alignment	\n\
	call	GetProcAddress	# Load it				\n\
	test	%rax,%rax	# Success?				\n\
	jne	gotit		# Yes					\n\
	jmp	noload		# Issue an error or return		\n\
gotit:									\n\
	addq	$40,%rsp	# Revert stack				\n\
	pop	%r10		# Pointer to 'return address'		\n\
	movq	%rax,12(%r10)	# Move absolute address to address slot	\n\
	subq	$25,%r10	# Point to jmp				\n\
	pop	%rcx		# Restore arg registers			\n\
	pop	%rdx							\n\
	pop	%r8							\n\
	pop	%r9							\n\
	pop	%rbp		# ...and restore frame pointer		\n\
	jmp	*%r10		# Jump to actual function		\n\
									\n\
	.global	dll_chain						\n\
dll_chain:								\n\
	push	%rax		# Restore 'return address'		\n\
	jmp	*%rdx		# Jump to next init function		\n\
");
#elif defined(__aarch64__)
__asm__ ( "\n\
  .section .rdata,\"r\"                                  \n\
msg1:                                                    \n\
  .ascii \"couldn't dynamically determine load address for '%s' (handle %p), %E\\0\" \n\
                                                         \n\
  .text                                                  \n\
  .p2align 2                                             \n\
 noload:                                                 \n\
  ldr        x2, [sp]          // x2 = func_info* (pushed by dll_chain)\n\
  ldr        w3, [x2, #8]      // w3 = func_info.decoration\n\
  tbz        w3, #0, 1f        // notimp bit clear -> fatal\n\
  asr        w4, w3, #16       // w4 = err (sign-extend high half)\n\
  str        w4, [sp, #8]      // spill into dll_chain's xzr slot\n\
  mov        w0, #127          // ERROR_PROC_NOT_FOUND   \n\
  bl         SetLastError                                \n\
  ldr        w0, [sp, #8]      // reload err as return value\n\
  ldr        x30, [sp, #88]    // restore caller LR (16 + 72)\n\
  add        sp, sp, #96       // drop dll_chain (16) + trampoline (80) frames\n\
  ret                                                    \n\
1:                                                       \n\
  add        x1, x2, #20       // x1 = &func_info.name   \n\
  ldr        x3, [x2]          // x3 = dll_info*         \n\
  ldr        x2, [x3, #8]      // x2 = dll_info->handle  \n\
  adrp       x0, msg1                                    \n\
  add        x0, x0, #:lo12:msg1                         \n\
  bl         api_fatal         // never returns          \n\
                                                         \n\
  .globl     dll_func_load                               \n\
dll_func_load:                                           \n\
  ldr        x2, [sp]          // x2 = func_info* (pushed by dll_chain)\n\
  ldr        x3, [x2]          // x3 = dll_info*         \n\
  ldr        x0, [x3, #8]      // x0 = dll handle (arg1 to GetProcAddress)\n\
  add        x1, x2, #20       // x1 = &func_info.name (arg2)\n\
  bl         GetProcAddress                              \n\
  cbz        x0, noload        // 0 -> not found, jump to error path\n\
  ldr        x2, [sp]          // reload func_info*      \n\
  str        x0, [x2, #12]     // patch func_addr slot (label 3)\n\
  sub        x16, x2, #52      // x16 = trampoline entry; '#52' = 12 byte prologue\n\
                               // + 40 byte slow path; keep in sync with LoadDLLfuncEx3\n\
  add        sp, sp, #16       // drop dll_chain frame   \n\
  ldp        x0, x1, [sp, #0]  // restore caller's arg regs\n\
  ldp        x2, x3, [sp, #16]                           \n\
  ldp        x4, x5, [sp, #32]                           \n\
  ldp        x6, x7, [sp, #48]                           \n\
  ldp        x8, x30, [sp, #64]                          \n\
  add        sp, sp, #80       // drop trampoline frame  \n\
  br         x16               // re-enter trampoline; fast path now hits real func\n\
                                                         \n\
  .global    dll_chain                                   \n\
dll_chain:                                               \n\
  stp        x0, xzr, [sp, #-16]! // x0 = func_info* (= ret.high); push for dll_func_load\n\
  br         x1                   // x1 = dll->init (= ret.low); tail-call resolver\n\
");
#else
#error unimplemented for this target
#endif

/* C representations of the two info blocks described above.
   FIXME: These structures confuse gdb for some reason.  GDB can print
   the whole structure but has problems with the name field? */
struct dll_info
{
  UINT_PTR load_state;
  HANDLE handle;
  LONG here;
  void (*init) ();
  WCHAR name[];
};

struct func_info
{
  struct dll_info *dll;
  LONG decoration; // actually notimp & error;
  UINT_PTR func_addr;
  char name[];
};

/* Mechanism for setting up info for passing to dll_chain routines. */
typedef __uint128_t two_addr_t;
union retchain
{
  struct {uintptr_t high; uintptr_t low;};
  two_addr_t ll;
};

/* This function handles the problem described here:

  http://www.microsoft.com/technet/security/advisory/2269637.mspx
  https://msdn.microsoft.com/library/ff919712 */
static __inline bool
dll_load (HANDLE& handle, PWCHAR name)
{
  HANDLE h = NULL;
  WCHAR dll_path[MAX_PATH];

  /* Try loading with full path, which sometimes fails for no good reason. */
  wcpcpy (wcpcpy (dll_path, windows_system_directory), name);
  h = LoadLibraryW (dll_path);
  /* If it failed, try loading just by name. */
  if (!h)
    h = LoadLibraryW (name);
  if (!h)
    return false;
  handle = h;
  return true;
}

#define RETRY_COUNT 10

/* The standard DLL initialization routine. */
#if defined(__x86_64__)

/* On x86_64, we need assembler wrappers for std_dll_init and wsock_init.
   In the x86_64 ABI it's no safe bet that frame[1] (aka 8(%rbp)) contains
   the return address.  Consequentially, if we try to overwrite frame[1]
   with the address of dll_chain, we end up with a scrambled stack, the
   result depending on the optimization settings and the current frame of
   mind of the compiler.  So for x86_64, we disable overwriting the return
   address in the real std_dll_init/wsock_init function, but rather do this
   in the wrapper, after return from the function, when we exactly know
   where the original return address is stored on the stack. */

#define INIT_WRAPPER(func) \
__asm__ ("								\n\
	.text								\n\
	.p2align 4,,15							\n\
	.seh_proc _" #func "						\n\
_" #func ":								\n\
	pushq	%rbp							\n\
	.seh_pushreg %rbp						\n\
	movq	%rsp,%rbp						\n\
	.seh_setframe %rbp,0						\n\
	subq	$0x20,%rsp						\n\
	.seh_stackalloc 32						\n\
	.seh_endprologue						\n\
	movq	0x28(%rsp),%rcx		# return address as parameter	\n\
	call	" #func "						\n\
	movdqa	%xmm0,0x10(%rsp)	# 128 bit return value in xmm0	\n\
	movq	0x10(%rsp),%rax		# copy over to %rax and %rdx	\n\
	movq	0x18(%rsp),%rdx						\n\
	leaq	dll_chain(%rip),%rcx	# load address of dll_chain	\n\
	movq	%rcx,0x28(%rsp)		# and overwrite return address	\n\
	addq	$0x20,%rsp						\n\
	popq	%rbp							\n\
	ret								\n\
	.seh_endproc							\n\
");

INIT_WRAPPER (std_dll_init)

#elif defined(__aarch64__)
#define INIT_WRAPPER(func) __asm__ ( "\n\
  .text                                                  \n\
  .p2align 2                                             \n\
  .seh_proc _" #func "                                   \n\
_" #func ":                                              \n\
  stp        x29, x30, [sp, #-16]!                       \n\
  .seh_save_fplr_x 16                                    \n\
  .seh_endprologue                                       \n\
  mov        x0, x30                                     \n\
  bl         " #func "                                   \n\
  ldp        x29, xzr, [sp], #16                         \n\
  adrp       x30, dll_chain                              \n\
  add        x30, x30, #:lo12:dll_chain                  \n\
  ret                                                    \n\
  .seh_endproc                                           \n\
");

INIT_WRAPPER (std_dll_init)

#else
#error unimplemented for this target
#endif

__attribute__ ((used, noinline)) static two_addr_t
std_dll_init (struct func_info *func)
{
  struct dll_info *dll = func->dll;
  retchain ret;

  if (InterlockedIncrement (&dll->here))
    do
      {
	InterlockedDecrement (&dll->here);
	yield ();
      }
    while (InterlockedIncrement (&dll->here));
  else if ((uintptr_t) dll->handle <= 1)
    {
      fenv_t fpuenv;
      fegetenv (&fpuenv);
      DWORD err = ERROR_SUCCESS;
      int i;
      /* MSDN seems to imply that LoadLibrary can fail mysteriously, so,
	 since there have been reports of this in the mailing list, retry
	 several times before giving up. */
      for (i = 1; i <= RETRY_COUNT; i++)
	{
	  /* If loading the library succeeds, just leave the loop. */
	  if (dll_load (dll->handle, dll->name))
	    break;
	  /* Otherwise check error code returned by LoadLibrary.  If the
	     error code is neither NOACCESS nor DLL_INIT_FAILED, break out
	     of the loop. */
	  err = GetLastError ();
	  if (err != ERROR_NOACCESS && err != ERROR_DLL_INIT_FAILED)
	    break;
	  if (i < RETRY_COUNT)
	    yield ();
	}
      if ((uintptr_t) dll->handle <= 1)
	{
	  if ((func->decoration & 1))
	    dll->handle = INVALID_HANDLE_VALUE;
	  else
	    api_fatal ("unable to load %W, %E", dll->name);
	}
      fesetenv (&fpuenv);
    }

  /* Set "arguments" for dll_chain. */
  ret.low = (uintptr_t) dll->init;
  ret.high = (uintptr_t) func;

  InterlockedDecrement (&dll->here);
  return ret.ll;
}

/* Initialization function for winsock stuff. */

#if defined(__x86_64__) || defined(__aarch64__)
/* See above comment preceeding std_dll_init. */
INIT_WRAPPER (wsock_init)
#else
#error unimplemented for this target
#endif

__attribute__ ((used, noinline)) static two_addr_t
wsock_init (struct func_info *func)
{
  /* CV 2016-03-09: Moved wsadata into wsock_init to workaround a problem
     with the NO_COPY definition of wsadata and here starting with gcc-5.3.0.
     See the git log for a description. */
  static WSADATA NO_COPY wsadata;
  static LONG NO_COPY here = -1L;
  struct dll_info *dll = func->dll;

  while (InterlockedIncrement (&here))
    {
      InterlockedDecrement (&here);
      yield ();
    }

  if (!wsock_started)
    {
      int (*wsastartup) (int, WSADATA *);

      /* Don't use autoload to load WSAStartup to eliminate recursion. */
      wsastartup = (int (*)(int, WSADATA *))
		   GetProcAddress ((HMODULE) (dll->handle), "WSAStartup");
      if (wsastartup)
	{
	  int res = wsastartup (MAKEWORD (2, 2), &wsadata);

	  debug_printf ("res %d", res);
	  debug_printf ("wVersion %d", wsadata.wVersion);
	  debug_printf ("wHighVersion %d", wsadata.wHighVersion);
	  debug_printf ("szDescription %s", wsadata.szDescription);
	  debug_printf ("szSystemStatus %s", wsadata.szSystemStatus);
	  debug_printf ("iMaxSockets %d", wsadata.iMaxSockets);
	  debug_printf ("iMaxUdpDg %d", wsadata.iMaxUdpDg);

	  wsock_started = 1;
	}
    }
  InterlockedDecrement (&here);
  volatile retchain ret;
  /* Set "arguments for dll_chain. */
  ret.low = (uintptr_t) dll_func_load;
  ret.high = (uintptr_t) func;
  return ret.ll;
}

LoadDLLprime (ws2_32, _wsock_init, 0)

LoadDLLfunc (CheckTokenMembership, advapi32)
LoadDLLfunc (CreateProcessAsUserW, advapi32)
LoadDLLfunc (DeregisterEventSource, advapi32)
LoadDLLfunc (DecryptFileW, advapi32)
LoadDLLfunc (EncryptFileW, advapi32)
LoadDLLfunc (LogonUserW, advapi32)
LoadDLLfunc (LookupAccountNameW, advapi32)
LoadDLLfunc (LookupAccountSidW, advapi32)
LoadDLLfunc (LsaClose, advapi32)
LoadDLLfunc (LsaEnumerateAccountRights, advapi32)
LoadDLLfunc (LsaFreeMemory, advapi32)
LoadDLLfunc (LsaLookupSids, advapi32)
LoadDLLfunc (LsaOpenPolicy, advapi32)
LoadDLLfunc (LsaQueryInformationPolicy, advapi32)
LoadDLLfunc (LsaRetrievePrivateData, advapi32)
LoadDLLfunc (LsaStorePrivateData, advapi32)
LoadDLLfunc (RegOpenUserClassesRoot, advapi32)
LoadDLLfunc (RegOpenCurrentUser, advapi32)
LoadDLLfunc (RegCloseKey, advapi32)
LoadDLLfunc (RegCreateKeyExW, advapi32)
LoadDLLfunc (RegEnumKeyExW, advapi32)
LoadDLLfunc (RegEnumValueW, advapi32)
LoadDLLfunc (RegGetKeySecurity, advapi32)
LoadDLLfunc (RegOpenKeyExW, advapi32)
LoadDLLfunc (RegQueryInfoKeyW, advapi32)
LoadDLLfunc (RegQueryValueExW, advapi32)
LoadDLLfunc (RegisterEventSourceW, advapi32)
LoadDLLfunc (ReportEventW, advapi32)
LoadDLLfunc (SystemFunction036, advapi32)	/* Aka "RtlGenRandom" */

LoadDLLfunc (AuthzAccessCheck, authz)
LoadDLLfunc (AuthzFreeContext, authz)
LoadDLLfunc (AuthzInitializeContextFromSid, authz)
LoadDLLfunc (AuthzInitializeContextFromToken, authz)
LoadDLLfunc (AuthzInitializeResourceManager, authz)

LoadDLLfunc (DnsQuery_A, dnsapi)
LoadDLLfunc (DnsFree, dnsapi)

LoadDLLfunc (GetAdaptersAddresses, iphlpapi)
LoadDLLfunc (GetIfEntry, iphlpapi)
LoadDLLfunc (GetIpAddrTable, iphlpapi)
LoadDLLfunc (GetIpForwardTable, iphlpapi)
LoadDLLfunc (GetNetworkParams, iphlpapi)
LoadDLLfunc (GetTcpTable, iphlpapi)
LoadDLLfunc (GetTcp6Table, iphlpapi)
LoadDLLfunc (GetUdpTable, iphlpapi)

LoadDLLfunc (AllocConsoleWithOptions, kernel32)
LoadDLLfuncEx2 (DiscardVirtualMemory, kernel32, 1, 127)
LoadDLLfuncEx (ClosePseudoConsole, kernel32, 1)
LoadDLLfuncEx (CreatePseudoConsole, kernel32, 1)
LoadDLLfuncEx (IsWow64Process2, kernel32, 1)
LoadDLLfuncEx (ResizePseudoConsole, kernel32, 1)

/* MSDN claims these are exported by kernel32.dll, but only
   QueryUnbiasedInterruptTime actually is.  The others are only
   available via KernelBase.dll. */
LoadDLLfunc (QueryInterruptTime, KernelBase)
LoadDLLfunc (QueryInterruptTimePrecise, KernelBase)
LoadDLLfunc (QueryUnbiasedInterruptTimePrecise, KernelBase)
LoadDLLfuncEx (SetThreadDescription, KernelBase, 1)
LoadDLLfunc (VirtualAlloc2, KernelBase)

LoadDLLfunc (NtMapViewOfSectionEx, ntdll)
LoadDLLfuncEx (RtlSetProcessPlaceholderCompatibilityMode, ntdll, 1)

LoadDLLfunc (ldap_bind_s, wldap32)
LoadDLLfunc (ldap_count_entries, wldap32)
LoadDLLfunc (ldap_count_valuesW, wldap32)
LoadDLLfunc (ldap_first_entry, wldap32)
LoadDLLfunc (ldap_get_next_page_s, wldap32)
LoadDLLfunc (ldap_get_valuesW, wldap32)
LoadDLLfunc (ldap_get_values_lenW, wldap32)
LoadDLLfunc (ldap_initW, wldap32)
LoadDLLfunc (ldap_msgfree, wldap32)
LoadDLLfunc (ldap_next_entry, wldap32)
LoadDLLfunc (ldap_search_abandon_page, wldap32)
LoadDLLfunc (ldap_search_init_pageW, wldap32)
LoadDLLfunc (ldap_search_sW, wldap32)
LoadDLLfunc (ldap_set_option, wldap32)
LoadDLLfunc (ldap_sslinitW, wldap32)
LoadDLLfunc (ldap_unbind, wldap32)
LoadDLLfunc (ldap_value_freeW, wldap32)
LoadDLLfunc (ldap_value_free_len, wldap32)
LoadDLLfunc (LdapGetLastError, wldap32)
LoadDLLfunc (LdapMapErrorToWin32, wldap32)

LoadDLLfunc (WNetCloseEnum, mpr)
LoadDLLfunc (WNetEnumResourceW, mpr)
LoadDLLfunc (WNetGetNetworkInformationW, mpr)
LoadDLLfunc (WNetGetProviderNameW, mpr)
LoadDLLfunc (WNetGetResourceInformationW, mpr)
LoadDLLfunc (WNetOpenEnumW, mpr)

LoadDLLfunc (DsEnumerateDomainTrustsW, netapi32)
LoadDLLfunc (DsGetDcNameW, netapi32)
LoadDLLfunc (NetApiBufferFree, netapi32)
LoadDLLfunc (NetGroupEnum, netapi32)
LoadDLLfunc (NetLocalGroupEnum, netapi32)
LoadDLLfunc (NetLocalGroupGetInfo, netapi32)
LoadDLLfunc (NetUseGetInfo, netapi32)
LoadDLLfunc (NetUserEnum, netapi32)
LoadDLLfunc (NetUserGetGroups, netapi32)
LoadDLLfunc (NetUserGetInfo, netapi32)
LoadDLLfunc (NetUserGetLocalGroups, netapi32)

LoadDLLfunc (CoInitialize, ole32)
LoadDLLfunc (CoUninitialize, ole32)
LoadDLLfunc (CoTaskMemFree, ole32)

LoadDLLfunc (EnumProcessModules, psapi)

LoadDLLfunc (LsaConnectUntrusted, secur32)
LoadDLLfunc (LsaDeregisterLogonProcess, secur32)
LoadDLLfunc (LsaFreeReturnBuffer, secur32)
LoadDLLfunc (LsaLogonUser, secur32)
LoadDLLfunc (LsaLookupAuthenticationPackage, secur32)
LoadDLLfunc (LsaRegisterLogonProcess, secur32)
LoadDLLfunc (TranslateNameW, secur32)

LoadDLLfunc (SHCreateItemFromParsingName, shell32)
LoadDLLfunc (SHGetDesktopFolder, shell32)
LoadDLLfunc (SHGetKnownFolderItem, shell32)

LoadDLLfunc (CreateFontW, gdi32)
LoadDLLfunc (DeleteObject, gdi32)
LoadDLLfunc (EnumFontFamiliesExW, gdi32)
LoadDLLfunc (GetGlyphIndicesW, gdi32)
LoadDLLfunc (SelectObject, gdi32)

LoadDLLfunc (CloseClipboard, user32)
LoadDLLfunc (CloseDesktop, user32)
LoadDLLfunc (CloseWindowStation, user32)
LoadDLLfunc (CreateDesktopW, user32)
LoadDLLfunc (CreateWindowExW, user32)
LoadDLLfunc (CreateWindowStationW, user32)
LoadDLLfunc (DefWindowProcW, user32)
LoadDLLfunc (DestroyWindow, user32)
LoadDLLfunc (DispatchMessageW, user32)
LoadDLLfunc (EmptyClipboard, user32)
LoadDLLfunc (EnumChildWindows, user32)
LoadDLLfunc (EnumWindows, user32)
LoadDLLfunc (GetClassNameA, user32)
LoadDLLfunc (GetClipboardData, user32)
LoadDLLfunc (GetDC, user32)
LoadDLLfunc (GetForegroundWindow, user32)
LoadDLLfunc (GetKeyboardLayout, user32)
LoadDLLfunc (GetMessageW, user32)
LoadDLLfunc (GetPriorityClipboardFormat, user32)
LoadDLLfunc (GetProcessWindowStation, user32)
LoadDLLfunc (GetThreadDesktop, user32)
LoadDLLfunc (GetUserObjectInformationW, user32)
LoadDLLfunc (GetWindowThreadProcessId, user32)
LoadDLLfunc (MessageBeep, user32)
LoadDLLfunc (MessageBoxW, user32)
LoadDLLfunc (MsgWaitForMultipleObjectsEx, user32)
LoadDLLfunc (OpenClipboard, user32)
LoadDLLfunc (PeekMessageW, user32)
LoadDLLfunc (PostMessageW, user32)
LoadDLLfunc (PostQuitMessage, user32)
LoadDLLfunc (RegisterClassW, user32)
LoadDLLfunc (RegisterClipboardFormatW, user32)
LoadDLLfunc (SendNotifyMessageW, user32)
LoadDLLfunc (SetClipboardData, user32)
LoadDLLfunc (SetParent, user32)
LoadDLLfunc (SetProcessWindowStation, user32)
LoadDLLfunc (SetThreadDesktop, user32)
LoadDLLfunc (UnregisterClassW, user32)

LoadDLLfuncEx (CreateEnvironmentBlock, userenv, 1)
LoadDLLfuncEx2 (CreateProfile, userenv, 1, 1)
LoadDLLfunc (DestroyEnvironmentBlock, userenv)
LoadDLLfunc (LoadUserProfileW, userenv)

LoadDLLfuncEx3 (waveInAddBuffer, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveInClose, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveInGetNumDevs, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveInOpen, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveInPrepareHeader, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveInReset, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveInStart, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveInUnprepareHeader, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveOutClose, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveOutGetNumDevs, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveOutGetVolume, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveOutOpen, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveOutPrepareHeader, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveOutReset, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveOutSetVolume, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveOutUnprepareHeader, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveOutWrite, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveOutMessage, winmm, 1, 0, 1)
LoadDLLfuncEx3 (waveOutGetDevCapsA, winmm, 1, 0, 1)

LoadDLLfunc (accept, ws2_32)
LoadDLLfunc (bind, ws2_32)
LoadDLLfunc (closesocket, ws2_32)
LoadDLLfunc (connect, ws2_32)
LoadDLLfunc (FreeAddrInfoW, ws2_32)
LoadDLLfunc (GetAddrInfoW, ws2_32)
LoadDLLfunc (GetNameInfoW, ws2_32)
LoadDLLfunc (gethostbyaddr, ws2_32)
LoadDLLfunc (gethostbyname, ws2_32)
LoadDLLfunc (gethostname, ws2_32)
LoadDLLfunc (getpeername, ws2_32)
LoadDLLfunc (getprotobyname, ws2_32)
LoadDLLfunc (getprotobynumber, ws2_32)
LoadDLLfunc (getservbyname, ws2_32)
LoadDLLfunc (getservbyport, ws2_32)
LoadDLLfunc (getsockname, ws2_32)
LoadDLLfunc (getsockopt, ws2_32)
LoadDLLfunc (ioctlsocket, ws2_32)
LoadDLLfunc (listen, ws2_32)
LoadDLLfunc (setsockopt, ws2_32)
LoadDLLfunc (shutdown, ws2_32)
LoadDLLfunc (socket, ws2_32)
LoadDLLfunc (WSAAsyncSelect, ws2_32)
LoadDLLfunc (WSADuplicateSocketW, ws2_32)
LoadDLLfunc (WSAEnumNetworkEvents, ws2_32)
LoadDLLfunc (WSAEventSelect, ws2_32)
LoadDLLfunc (WSAGetLastError, ws2_32)
LoadDLLfunc (WSAIoctl, ws2_32)
LoadDLLfunc (WSAPoll, ws2_32)
LoadDLLfunc (WSARecv, ws2_32)
LoadDLLfunc (WSARecvFrom, ws2_32)
LoadDLLfunc (WSASendMsg, ws2_32)
LoadDLLfunc (WSASendTo, ws2_32)
LoadDLLfunc (WSASetLastError, ws2_32)
LoadDLLfunc (WSASocketW, ws2_32)
// LoadDLLfunc (WSAStartup, ws2_32)
LoadDLLfunc (WSAWaitForMultipleEvents, ws2_32)

LoadDLLfunc (PdhAddEnglishCounterW, pdh)
LoadDLLfunc (PdhCollectQueryData, pdh)
LoadDLLfunc (PdhGetFormattedCounterValue, pdh)
LoadDLLfunc (PdhOpenQueryW, pdh)
}
