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
#include "sys/asm.h"
#include "xlenint.h"
#include <limits.h>
#include <stddef.h>
#include <string.h>

static inline uint8_t
__libc_fast_xlen_aligned (void *dst, const void *src)
{
#if defined(__riscv_misaligned_fast)
  return 1;
#else
  return !(((uintxlen_t)src & (SZREG - 1)) | ((uintxlen_t)dst & (SZREG - 1)));
#endif
}

void *__inhibit_loop_to_libcall
memmove (void *dst_void, const void *src_void, size_t length)
{
  unsigned char *dst = dst_void;
  const unsigned char *src = src_void;
  uintxlen_t *aligned_dst;
  const uintxlen_t *aligned_src;

  if (src < dst && dst < src + length)
    {
      /* Destructive overlap...have to copy backwards */
      src += length;
      dst += length;

      if (length >= SZREG && __libc_fast_xlen_aligned (dst, src))
        {
          aligned_dst = (uintxlen_t *)dst;
          aligned_src = (uintxlen_t *)src;

          /* Copy one uintxlen_t word at a time if possible.  */
          while (length >= SZREG)
            {
              *--aligned_dst = *--aligned_src;
              length -= SZREG;
            }

          /* Pick up any residual with a byte copier.  */
          dst = (unsigned char *)aligned_dst;
          src = (unsigned char *)aligned_src;
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
      if (length >= SZREG && __libc_fast_xlen_aligned (dst, src))
        {
          aligned_dst = (uintxlen_t *)dst;
          aligned_src = (uintxlen_t *)src;

          /* Copy 4X uintxlen_t words at a time if possible.  */
          while (length >= (SZREG * 4))
            {
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              length -= SZREG * 4;
            }

          /* Copy one uintxlen_t word at a time if possible.  */
          while (length >= SZREG)
            {
              *aligned_dst++ = *aligned_src++;
              length -= SZREG;
            }

          /* Pick up any residual with a byte copier.  */
          dst = (unsigned char *)aligned_dst;
          src = (unsigned char *)aligned_src;
        }

      while (length--)
        {
          *dst++ = *src++;
        }
    }

  return dst_void;
}
#endif
