/* resource.cc: getrusage () and friends.

   Written by Steve Chamberlain (sac@cygnus.com), Doug Evans (dje@cygnus.com),
   Geoffrey Noer (noer@cygnus.com) of Cygnus Support.
   Rewritten by Sergey S. Okhapkin (sos@prospect.com.ru)

This file is part of Cygwin.

This software is a copyrighted work licensed under the terms of the
Cygwin license.  Please consult the file "CYGWIN_LICENSE" for
details. */

#include "winsup.h"
#include <unistd.h>
#include <sys/param.h>
#include "pinfo.h"
#include "cygtls.h"
#include "path.h"
#include "fhandler.h"
#include "pinfo.h"
#include "dtable.h"
#include "cygheap.h"
#include "child_info.h"
#include "shared_info.h"
#include "ntdll.h"

/* add timeval values */
static void
add_timeval (struct timeval *tv1, struct timeval *tv2)
{
  tv1->tv_sec += tv2->tv_sec;
  tv1->tv_usec += tv2->tv_usec;
  if (tv1->tv_usec >= USPERSEC)
    {
      tv1->tv_usec -= USPERSEC;
      tv1->tv_sec++;
    }
}

/* add rusage values of r2 to r1 */
void
add_rusage (struct rusage *r1, struct rusage *r2)
{
  add_timeval (&r1->ru_utime, &r2->ru_utime);
  add_timeval (&r1->ru_stime, &r2->ru_stime);
  r1->ru_maxrss += r2->ru_maxrss;
  r1->ru_ixrss += r2->ru_ixrss;
  r1->ru_idrss += r2->ru_idrss;
  r1->ru_isrss += r2->ru_isrss;
  r1->ru_minflt += r2->ru_minflt;
  r1->ru_majflt += r2->ru_majflt;
  r1->ru_nswap += r2->ru_nswap;
  r1->ru_inblock += r2->ru_inblock;
  r1->ru_oublock += r2->ru_oublock;
  r1->ru_msgsnd += r2->ru_msgsnd;
  r1->ru_msgrcv += r2->ru_msgrcv;
  r1->ru_nsignals += r2->ru_nsignals;
  r1->ru_nvcsw += r2->ru_nvcsw;
  r1->ru_nivcsw += r2->ru_nivcsw;
}

/* FIXME: what about other fields? */
void
fill_rusage (struct rusage *r, HANDLE h)
{
  KERNEL_USER_TIMES kut;

  struct timeval tv;

  memset (&kut, 0, sizeof kut);
  memset (r, 0, sizeof *r);
  NtQueryInformationProcess (h, ProcessTimes, &kut, sizeof kut, NULL);
  totimeval (&tv, &kut.KernelTime, 0, 0);
  add_timeval (&r->ru_stime, &tv);
  totimeval (&tv, &kut.UserTime, 0, 0);
  add_timeval (&r->ru_utime, &tv);

  VM_COUNTERS vmc;
  NTSTATUS status = NtQueryInformationProcess (h, ProcessVmCounters, &vmc,
					       sizeof vmc, NULL);
  if (NT_SUCCESS (status))
    {
      r->ru_maxrss += (long) (vmc.WorkingSetSize / 1024);
      r->ru_majflt += vmc.PageFaultCount;
    }
}

extern "C" int
getrusage (int intwho, struct rusage *rusage_in)
{
  int res = 0;
  struct rusage r;

  if (intwho == RUSAGE_SELF)
    {
      memset (&r, 0, sizeof (r));
      fill_rusage (&r, GetCurrentProcess ());
      *rusage_in = r;
    }
  else if (intwho == RUSAGE_CHILDREN)
    *rusage_in = myself->rusage_children;
  else
    {
      set_errno (EINVAL);
      res = -1;
    }

  syscall_printf ("%R = getrusage(%d, %p)", res, intwho, rusage_in);
  return res;
}

/* Default stacksize in case RLIMIT_STACK is RLIM_INFINITY is 2 Megs with
   system-dependent number of guard pages.  The pthread stacksize does not
   include the guardpage size, so we have to subtract the default guardpage
   size.  Additionally the Windows stack handling disallows to commit the
   last page, so we subtract it, too. */
#define DEFAULT_STACKSIZE (2 * 1024 * 1024)
#define DEFAULT_STACKGUARD (wincap.def_guard_page_size() + wincap.page_size ())

muto NO_COPY rlimit_stack_guard;
static struct rlimit rlimit_stack = { 0, RLIM_INFINITY };

static void
__set_rlimit_stack (const struct rlimit *rlp)
{
  rlimit_stack_guard.init ("rlimit_stack_guard")->acquire ();
  rlimit_stack = *rlp;
  rlimit_stack_guard.release ();
}

static void
__get_rlimit_stack (struct rlimit *rlp)
{
  rlimit_stack_guard.init ("rlimit_stack_guard")->acquire ();
  if (!rlimit_stack.rlim_cur)
    {
      /* Fetch the default stacksize from the executable header... */
      PIMAGE_DOS_HEADER dosheader;
      PIMAGE_NT_HEADERS ntheader;

      dosheader = (PIMAGE_DOS_HEADER) GetModuleHandle (NULL);
      ntheader = (PIMAGE_NT_HEADERS) ((PBYTE) dosheader + dosheader->e_lfanew);
      rlimit_stack.rlim_cur = ntheader->OptionalHeader.SizeOfStackReserve;
      /* ...and subtract the guardpages. */
      rlimit_stack.rlim_cur -= DEFAULT_STACKGUARD;
    }
  *rlp = rlimit_stack;
  rlimit_stack_guard.release ();
}

/* Interface to stack limit called from pthread_create and
   pthread_attr_getstacksize. */
size_t
get_rlimit_stack (void)
{
  struct rlimit rl;

  __get_rlimit_stack (&rl);
  /* RLIM_INFINITY doesn't make much sense.  As in glibc, use an
     "architecture-specific default". */
  if (rl.rlim_cur == RLIM_INFINITY)
    rl.rlim_cur = DEFAULT_STACKSIZE - DEFAULT_STACKGUARD;
  /* Always return at least minimum stacksize. */
  else if (rl.rlim_cur < PTHREAD_STACK_MIN)
    rl.rlim_cur = PTHREAD_STACK_MIN;
  return (size_t) rl.rlim_cur;
}

enum limit_flags_t
{
  PER_PROCESS = 1,
  PER_USER = 2,

  SOFT_LIMIT = 4,
  HARD_LIMIT = 8
};

static PWCHAR
job_shared_name (PWCHAR buf, int flags)
{
  __small_swprintf (buf, L"rlimit.%C.%W.%u",
			 (flags & HARD_LIMIT) ? L'H' : L'S',
			 (flags & PER_USER) ? L"uid" : L"pid",
			 (flags & PER_USER) ? getuid () : getpid ());
  return buf;
}

static PJOBOBJECT_EXTENDED_LIMIT_INFORMATION
__get_os_limits (JOBOBJECT_EXTENDED_LIMIT_INFORMATION &jobinfo, int flags)
{
  OBJECT_ATTRIBUTES attr;
  UNICODE_STRING uname;
  WCHAR jobname[32];
  HANDLE job = NULL;
  NTSTATUS status;

  RtlInitUnicodeString (&uname, job_shared_name (jobname, flags));
  InitializeObjectAttributes (&attr, &uname, 0,
			      flags & PER_USER ? get_shared_parent_dir ()
					       : get_session_parent_dir (),
			      NULL);
  /* May fail, just check NULL job in that case. */
  NtOpenJobObject (&job, JOB_OBJECT_QUERY, &attr);
  status = NtQueryInformationJobObject (job,
					JobObjectExtendedLimitInformation,
					&jobinfo, sizeof jobinfo, NULL);
  if (job)
    NtClose (job);
  return NT_SUCCESS (status) ? &jobinfo : NULL;
}

static int
__set_os_limits (JOBOBJECT_EXTENDED_LIMIT_INFORMATION &jobinfo, int flags)
{
  NTSTATUS status = STATUS_SUCCESS;
  OBJECT_ATTRIBUTES attr;
  UNICODE_STRING uname;
  WCHAR jobname[32];
  HANDLE job = NULL;

  RtlInitUnicodeString (&uname, job_shared_name (jobname, flags));
  InitializeObjectAttributes (&attr, &uname, OBJ_OPENIF,
			      flags & PER_USER ? get_shared_parent_dir ()
					       : get_session_parent_dir (),
			      NULL);
  status = NtCreateJobObject (&job, JOB_OBJECT_ALL_ACCESS, &attr);
  if (!NT_SUCCESS (status))
    {
      __seterrno_from_nt_status (status);
      return -1;
    }
  jobinfo.BasicLimitInformation.LimitFlags
    |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;
  /* Every process gets its own per-process jobs, so always breakaway
     silently from per-process jobs. */
  if (flags & PER_PROCESS)
    jobinfo.BasicLimitInformation.LimitFlags
      |= JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
  status = NtSetInformationJobObject (job,
				      JobObjectExtendedLimitInformation,
				      &jobinfo, sizeof jobinfo);
  /* Assign the process to the job if it's not already assigned. */
  NTSTATUS in_job = NtIsProcessInJob (NtCurrentProcess (), job);
  if (NT_SUCCESS (status) && in_job == STATUS_PROCESS_NOT_IN_JOB)
    {
      status = NtAssignProcessToJobObject (job, NtCurrentProcess ());
      if (!NT_SUCCESS (status))
	debug_printf ("NtAssignProcessToJobObject: %y\r", status);
    }

  /* Keep the handle ONLY if we just assigned the process to the job */
  if (!NT_SUCCESS (status) || in_job == STATUS_PROCESS_IN_JOB)
    NtClose (job);

  if (!NT_SUCCESS (status))
    {
      __seterrno_from_nt_status (status);
      return -1;
    }
  return 0;
}

static void
__get_rlimit_as (struct rlimit *rlp)
{
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobinfo;

  rlp->rlim_cur = RLIM_INFINITY;
  rlp->rlim_max = RLIM_INFINITY;
  if (__get_os_limits (jobinfo, PER_PROCESS | HARD_LIMIT)
      && (jobinfo.BasicLimitInformation.LimitFlags
	  & JOB_OBJECT_LIMIT_PROCESS_MEMORY))
    rlp->rlim_max = jobinfo.ProcessMemoryLimit;
  if (__get_os_limits (jobinfo, PER_PROCESS | SOFT_LIMIT)
      && (jobinfo.BasicLimitInformation.LimitFlags
	  & JOB_OBJECT_LIMIT_PROCESS_MEMORY))
    rlp->rlim_cur = jobinfo.ProcessMemoryLimit;
}

static int
__set_rlimit_as_single (rlim_t val, int flags)
{
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobinfo = { 0 };

  __get_os_limits (jobinfo, PER_PROCESS | flags);
  if (val == RLIM_INFINITY)
    {
      jobinfo.BasicLimitInformation.LimitFlags
	&= ~JOB_OBJECT_LIMIT_PROCESS_MEMORY;
      jobinfo.ProcessMemoryLimit = 0;
    }
  else /* Per Linux man page, round down to system pagesize. */
    {
      jobinfo.BasicLimitInformation.LimitFlags
	|= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
      jobinfo.ProcessMemoryLimit
	= rounddown (val, wincap.allocation_granularity ());
    }
  return __set_os_limits (jobinfo, PER_PROCESS | flags);
}

static int
__set_rlimit_as (const struct rlimit *rlp)
{
  int ret = __set_rlimit_as_single (rlp->rlim_max, HARD_LIMIT);
  if (ret == 0)
    ret = __set_rlimit_as_single (rlp->rlim_cur, SOFT_LIMIT);
  return ret;
}

static int
__set_rlimit_nproc_single (rlim_t val, int flags)
{
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobinfo = { 0 };

  __get_os_limits (jobinfo, PER_USER | flags);
  /* ActiveProcessLimit is a DWORD */
  if (val == RLIM_INFINITY || val > UINT_MAX)
    {
      jobinfo.BasicLimitInformation.LimitFlags
	&= ~JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
      jobinfo.BasicLimitInformation.ActiveProcessLimit = 0;
    }
  else
    {
      jobinfo.BasicLimitInformation.LimitFlags
	|= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
      jobinfo.BasicLimitInformation.ActiveProcessLimit = val;
    }
  return __set_os_limits (jobinfo, PER_USER | flags);
}

int
__set_rlimit_nproc (const struct rlimit *rlp)
{
  int ret = __set_rlimit_nproc_single (rlp->rlim_max, HARD_LIMIT);
  if (ret == 0)
    ret = __set_rlimit_nproc_single (rlp->rlim_cur, SOFT_LIMIT);
  return ret;
}

void
__get_rlimit_nproc (struct rlimit *rlp)
{
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobinfo;

  rlp->rlim_cur = RLIM_INFINITY;
  rlp->rlim_max = RLIM_INFINITY;
  if (__get_os_limits (jobinfo, PER_USER | HARD_LIMIT)
      && (jobinfo.BasicLimitInformation.LimitFlags
	  & JOB_OBJECT_LIMIT_ACTIVE_PROCESS))
    rlp->rlim_max = jobinfo.BasicLimitInformation.ActiveProcessLimit;
  if (__get_os_limits (jobinfo, PER_USER | SOFT_LIMIT)
      && (jobinfo.BasicLimitInformation.LimitFlags
	  & JOB_OBJECT_LIMIT_ACTIVE_PROCESS))
    rlp->rlim_cur = jobinfo.BasicLimitInformation.ActiveProcessLimit;
}

/* Called during fork/exec in the parent to collect the per-process rlimits. */
void
child_info::collect_process_rlimits ()
{
  __get_rlimit_as (&rlimit_as);
  debug_printf ("parent rlimit AS: max %U cur %U",
		rlimit_as.rlim_max, rlimit_as.rlim_cur);
}

/* Called during fork/exec in the child to duplicate the per-process rlimits. */
void
child_info::inherit_process_rlimits ()
{
  if (rlimit_as.rlim_max != RLIM_INFINITY
      || rlimit_as.rlim_cur != RLIM_INFINITY)
    __set_rlimit_as (&rlimit_as);
  debug_printf ("child rlimit AS: max %U cur %U",
		rlimit_as.rlim_max, rlimit_as.rlim_cur);
}

static void
__setup_user_rlimits_single (int flags)
{
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobinfo = { 0 };
  NTSTATUS status = STATUS_SUCCESS;
  OBJECT_ATTRIBUTES attr;
  UNICODE_STRING uname;
  WCHAR jobname[32];
  HANDLE job = NULL;

  RtlInitUnicodeString (&uname, job_shared_name (jobname, PER_USER | flags));
  InitializeObjectAttributes (&attr, &uname, OBJ_OPENIF,
			      get_shared_parent_dir (), NULL);
  status = NtCreateJobObject (&job, JOB_OBJECT_ALL_ACCESS, &attr);
  if (!NT_SUCCESS (status))
    {
      debug_printf ("NtCreateJobObject (%S): status %y", &uname, status);
      return;
    }
  /* Did we just create the job? */
  if (status != STATUS_OBJECT_NAME_EXISTS)
    {
      jobinfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;
      status = NtSetInformationJobObject (job,
					  JobObjectExtendedLimitInformation,
					  &jobinfo, sizeof jobinfo);
    }
  NTSTATUS in_job = NtIsProcessInJob (NtCurrentProcess (), job);
  /* Assign the process to the job if it's not already assigned. */
  if (NT_SUCCESS (status) && in_job == STATUS_PROCESS_NOT_IN_JOB)
    {
      status = NtAssignProcessToJobObject (job, NtCurrentProcess ());
      if (!NT_SUCCESS (status))
	debug_printf ("NtAssignProcessToJobObject: %y\r", status);
    }
  /* Never close the handle. */
}

void
setup_user_rlimits ()
{
  __setup_user_rlimits_single (HARD_LIMIT);
  __setup_user_rlimits_single (SOFT_LIMIT);
}

extern "C" int
getrlimit (int resource, struct rlimit *rlp)
{
  __try
    {
      rlp->rlim_cur = RLIM_INFINITY;
      rlp->rlim_max = RLIM_INFINITY;

      switch (resource)
	{
	case RLIMIT_CPU:
	case RLIMIT_FSIZE:
	case RLIMIT_DATA:
	  break;
	case RLIMIT_AS:
	  __get_rlimit_as (rlp);
	  break;
	case RLIMIT_NPROC:
	  __get_rlimit_nproc (rlp);
	  break;
	case RLIMIT_STACK:
	  __get_rlimit_stack (rlp);
	  break;
	case RLIMIT_NOFILE:
	  rlp->rlim_cur = rlp->rlim_max = OPEN_MAX;
	  break;
	case RLIMIT_CORE:
	  rlp->rlim_cur = cygheap->rlim_core;
	  break;
	default:
	  set_errno (EINVAL);
	  __leave;
	}
      return 0;
    }
  __except (EFAULT) {}
  __endtry
  return -1;
}

extern "C" int
setrlimit (int resource, const struct rlimit *rlp)
{
  struct rlimit oldlimits;

  if (getrlimit (resource, &oldlimits) < 0)
    return -1;

  __try
    {
      /* Check if the request is to actually change the resource settings.
	 If it does not result in a change, take no action and do not fail. */
      if (oldlimits.rlim_cur == rlp->rlim_cur &&
	  oldlimits.rlim_max == rlp->rlim_max)
	return 0;

      /* soft limit > hard limit ? EINVAL */
      if (rlp->rlim_cur > rlp->rlim_max)
	{
	  set_errno (EINVAL);
	  __leave;
	}

      /* hard limit > current hard limit ? EPERM if not admin */
      if (rlp->rlim_max > oldlimits.rlim_max
	  && !check_token_membership (well_known_admins_sid))
	{
	  set_errno (EPERM);
	  __leave;
	}

      switch (resource)
	{
	case RLIMIT_AS:
	  return __set_rlimit_as (rlp);
	  break;
	case RLIMIT_NPROC:
	  return __set_rlimit_nproc (rlp);
	  break;
	case RLIMIT_CORE:
	  cygheap->rlim_core = rlp->rlim_cur;
	  break;
	case RLIMIT_NOFILE:
	  if (rlp->rlim_cur != RLIM_INFINITY)
	    return setdtablesize (rlp->rlim_cur);
	  break;
	case RLIMIT_STACK:
	  __set_rlimit_stack (rlp);
	  break;
	default:
	  set_errno (EINVAL);
	  __leave;
	}
      return 0;
    }
  __except (EFAULT)
  __endtry
  return -1;
}
