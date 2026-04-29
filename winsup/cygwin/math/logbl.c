/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Changes for long double by Ulrich Drepper <drepper@cygnus.com>
 * Public domain.
 */

#include <math.h>

long double
logbl (long double x)
{
  long double res = 0.0L;

#if defined(__x86_64__) || defined(__i386__)
  asm volatile (
       "fxtract\n\t"
       "fstp	%%st" : "=t" (res) : "0" (x));
#elif __SIZEOF_LONG_DOUBLE__ == __SIZEOF_DOUBLE__
  res = logb((double)x);
#endif
  return res;
}
