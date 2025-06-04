/* Define macros for CPU-agnostic register access.  The _CX_foo
   macros are for access into CONTEXT, the _MC_foo ones for access into
   mcontext. The idea is to access the registers in terms of their job,
   not in terms of their name on the given target. */
#if defined(__x86_64__)
#define _CX_instPtr	Rip
#define _CX_stackPtr	Rsp
#define _CX_framePtr	Rbp
/* For special register access inside mcontext. */
#define _MC_retReg	rax
#define _MC_instPtr	rip
#define _MC_stackPtr	rsp
#define _MC_uclinkReg	rbx	/* MUST be callee-saved reg */
#elif defined(__aarch64__)
#define _CX_instPtr	Pc
#define _CX_stackPtr	Sp
#define _CX_framePtr	Fp
/* For special register access inside mcontext. */
#define _MC_retReg	x0
#define _MC_instPtr	pc
#define _MC_stackPtr	sp
#define _MC_uclinkReg	x19	/* MUST be callee-saved reg */
#else
#error unimplemented for this target
#endif
