/* Copyright (C) 2015 by  Red Hat, Incorporated. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

#include <float.h>
#include "fdlibm.h"

long double
__ieee754_hypotl (long double x, long double y)
{
#ifdef _LDBL_EQ_DBL
  return __ieee754_hypot (x, y);
#else
  long double a = __builtin_fabsl (x), b = __builtin_fabsl (y), t;

  if (isinf (a) || isinf (b))
    return __builtin_infl ();	/* even when the other one is a NaN */
  if (isnan (a) || isnan (b))
    return a + b;
  if (a < b)
    t = a, a = b, b = t;

  /* Here b cannot change the result: do not square it, which could
     underflow for nothing.  a + b is still inexact when b is not 0.  */
  if (b <= a * LDBL_EPSILON)
    return a + b;

#if LDBL_MAX_EXP == 16384
  /* Scale by a power of two so that neither square overflows or
     underflows; b is within 2^LDBL_MANT_DIG of a here.  */
  if (a > 0x1p8000L)
    {
      a *= 0x1p-12000L;
      b *= 0x1p-12000L;
      return sqrtl ((a * a) + (b * b)) * 0x1p12000L;
    }
  if (a < 0x1p-8000L)
    {
      a *= 0x1p12000L;
      b *= 0x1p12000L;
      return sqrtl ((a * a) + (b * b)) * 0x1p-12000L;
    }
#endif
  return sqrtl ((a * a) + (b * b));
#endif
}
