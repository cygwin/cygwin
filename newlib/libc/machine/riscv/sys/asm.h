/* Copyright (c) 2017-2024  SiFive Inc. All rights reserved.

   This copyrighted material is made available to anyone wishing to use,
   modify, copy, or redistribute it subject to the terms and conditions
   of the FreeBSD License.   This program is distributed in the hope that
   it will be useful, but WITHOUT ANY WARRANTY expressed or implied,
   including the implied warranties of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  A copy of this license is available at
   http://www.opensource.org/licenses.
*/

#ifndef _SYS_ASM_H
#define _SYS_ASM_H

/*
 * Macros to handle different pointer/register sizes for 32/64-bit code
 */
#if __riscv_xlen == 64
# define PTRLOG 3
# define SZREG	8
# define REG_S sd
# define REG_L ld
#elif __riscv_xlen == 32
# define PTRLOG 2
# define SZREG	4
# define REG_S sw
# define REG_L lw
#else
# error __riscv_xlen must equal 32 or 64
#endif

#ifndef __riscv_float_abi_soft
/* For ABI uniformity, reserve 8 bytes for floats, even if double-precision
   floating-point is not supported in hardware.  */
# define SZFREG 8
# ifdef __riscv_float_abi_single
#  define FREG_L flw
#  define FREG_S fsw
# elif defined(__riscv_float_abi_double)
#  define FREG_L fld
#  define FREG_S fsd
# elif defined(__riscv_float_abi_quad)
#  define FREG_L flq
#  define FREG_S fsq
# else
#  error unsupported FLEN
# endif
#endif

#define FEATURE_1_AND 0xc0000000
/* Add a NT_GNU_PROPERTY_TYPE_0 note.  */
#if __riscv_xlen == 32
# define GNU_PROPERTY(type, value)	\
  .section .note.gnu.property, "a";	\
  .p2align 2;				\
  .word 4;				\
  .word 12;				\
  .word 5;				\
  .asciz "GNU";				\
  .word type;				\
  .word 4;				\
  .word value;				\
  .text
#else
# define GNU_PROPERTY(type, value)	\
  .section .note.gnu.property, "a";	\
  .p2align 3;				\
  .word 4;				\
  .word 16;				\
  .word 5;				\
  .asciz "GNU";				\
  .word type;				\
  .word 4;				\
  .word value;				\
  .word 0;				\
  .text
#endif

/* Add GNU property note with the supported features to all asm code
   where asm.h is included.  */
#undef __VALUE_FOR_FEATURE_1_AND
#if defined (__riscv_landing_pad) || defined (__riscv_shadow_stack)
# if defined (__riscv_landing_pad)
#  if defined (__riscv_shadow_stack)
#   define __VALUE_FOR_FEATURE_1_AND 0x3
#  else
#   define __VALUE_FOR_FEATURE_1_AND 0x1
#  endif
# else
#  if defined (__riscv_shadow_stack)
#   define __VALUE_FOR_FEATURE_1_AND 0x2
#  else
#   error "What?"
#  endif
# endif
#endif

#if defined (__VALUE_FOR_FEATURE_1_AND)
GNU_PROPERTY (FEATURE_1_AND, __VALUE_FOR_FEATURE_1_AND)
#endif
#undef __VALUE_FOR_FEATURE_1_AND

#ifdef __riscv_landing_pad
# define LPAD       lpad 0
#else
# define LPAD
#endif

#endif /* sys/asm.h */
