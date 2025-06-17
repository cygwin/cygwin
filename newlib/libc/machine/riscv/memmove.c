/* Copyright (c) 2019  SiFive Inc. All rights reserved.

   This copyrighted material is made available to anyone wishing to use,
   modify, copy, or redistribute it subject to the terms and conditions
   of the FreeBSD License.   This program is distributed in the hope that
   it will be useful, but WITHOUT ANY WARRANTY expressed or implied,
   including the implied warranties of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  A copy of this license is available at
   http://www.opensource.org/licenses.
*/

#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
/* memmove defined in memmove-asm.S */
#else

#include "../../string/local.h"
#include <_ansi.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

/*SUPPRESS 20*/
void *__inhibit_loop_to_libcall
memmove (void *dst_void, const void *src_void, size_t length)
{
  char *dst = dst_void;
  const char *src = src_void;
  long *aligned_dst;
  const long *aligned_src;

  if (src < dst && dst < src + length)
    {
      /* Destructive overlap...have to copy backwards */
      src += length;
      dst += length;

      if (!TOO_SMALL_LITTLE_BLOCK (length) && !UNALIGNED_X_Y (src, dst))
        {
          aligned_dst = (long *)dst;
          aligned_src = (long *)src;

          /* Copy one long word at a time if possible.  */
          while (!TOO_SMALL_LITTLE_BLOCK (length))
            {
              *--aligned_dst = *--aligned_src;
              length -= LITTLE_BLOCK_SIZE;
            }

          /* Pick up any residual with a byte copier.  */
          dst = (char *)aligned_dst;
          src = (char *)aligned_src;
        }

      while (length--)
        {
          *--dst = *--src;
        }
    }
  else
    {
      /* Use optimizing algorithm for a non-destructive copy to closely
         match memcpy. If the size is small or either SRC or DST is unaligned,
         then punt into the byte copy loop.  This should be rare.  */
      if (!TOO_SMALL_LITTLE_BLOCK (length) && !UNALIGNED_X_Y (src, dst))
        {
          aligned_dst = (long *)dst;
          aligned_src = (long *)src;

          /* Copy 4X long words at a time if possible.  */
          while (!TOO_SMALL_BIG_BLOCK (length))
            {
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              length -= BIG_BLOCK_SIZE;
            }

          /* Copy one long word at a time if possible.  */
          while (!TOO_SMALL_LITTLE_BLOCK (length))
            {
              *aligned_dst++ = *aligned_src++;
              length -= LITTLE_BLOCK_SIZE;
            }

          /* Pick up any residual with a byte copier.  */
          dst = (char *)aligned_dst;
          src = (char *)aligned_src;
        }

      while (length--)
        {
          *dst++ = *src++;
        }
    }

  return dst_void;
}
#endif
