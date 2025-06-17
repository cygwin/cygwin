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

static inline void
__libc_aligned_copy_unrolled (uintxlen_t *aligned_dst,
                              const uintxlen_t *aligned_src)
{
  uintxlen_t dst0 = *aligned_src++;
  uintxlen_t dst1 = *aligned_src++;
  uintxlen_t dst2 = *aligned_src++;
  uintxlen_t dst3 = *aligned_src++;
  uintxlen_t dst4 = *aligned_src++;
  uintxlen_t dst5 = *aligned_src++;
  uintxlen_t dst6 = *aligned_src++;
  uintxlen_t dst7 = *aligned_src++;
  uintxlen_t dst8 = *aligned_src;

  *aligned_dst++ = dst0;
  *aligned_dst++ = dst1;
  *aligned_dst++ = dst2;
  *aligned_dst++ = dst3;
  *aligned_dst++ = dst4;
  *aligned_dst++ = dst5;
  *aligned_dst++ = dst6;
  *aligned_dst++ = dst7;
  *aligned_dst = dst8;
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

          /* If possible, unroll the word-copy loop by a factor 9 to
             match memcpy. This speeds up the copying process for longer
             lengths while barely degrading performance for lengths < SZREG*9.
             Since we are copying backwards, decrement the addresses
             before copying.
           */
          while (length >= SZREG * 9)
            {
              aligned_dst -= 9;
              aligned_src -= 9;
              __libc_aligned_copy_unrolled (aligned_dst, aligned_src);
              length -= (SZREG * 9);
            }

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

          /* If possible, unroll the word-copy loop by a factor 9 to
             match memcpy. This speeds up the copying process for longer
             lengths while barely degrading performance for lengths < SZREG*9.
           */
          while (length >= SZREG * 9)
            {
              __libc_aligned_copy_unrolled (aligned_dst, aligned_src);
              aligned_dst += 9;
              aligned_src += 9;
              length -= (SZREG * 9);
            }

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
