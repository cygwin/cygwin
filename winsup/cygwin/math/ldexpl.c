/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <math.h>
#include <errno.h>

long double ldexpl(long double x, int expn)
{
  long double res = 0.0L;
  if (!isfinite (x) || x == 0.0L)
    return x;

#if defined(__x86_64__) || defined(__i386__)
  __asm__ __volatile__ ("fscale"
	    : "=t" (res)
	    : "0" (x), "u" ((long double) expn));
#elif __SIZEOF_LONG_DOUBLE__ == __SIZEOF_DOUBLE__
  res = ldexp((double)x, expn);
#endif

  if (!isfinite (res) || res == 0.0L)
    errno = ERANGE;

  return res;
}
