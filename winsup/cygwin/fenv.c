/* no-op function as entry point for applications built between
   2010-09-11 and 2011-03-16.  That's the timeframe of _feinitialise
   being called from mainCRTStartup in crt0.o. */
void _feinitialise (void)
{}

#if defined(__aarch64__)

#include <fenv.h>
#include <stddef.h>

/* _fe_nomask_env is exported by cygwin.din but not used at all for AArch64. */
const fenv_t *_fe_nomask_env = NULL;

#endif /* __aarch64__ */
