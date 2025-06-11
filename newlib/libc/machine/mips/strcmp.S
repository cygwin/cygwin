/*
 * Copyright (C) 2014-2018 MIPS Tech, LLC
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef ANDROID_CHANGES
# include "machine/asm.h"
# include "machine/regdef.h"
#elif _LIBC
# include "machine/asm.h"
# include "machine/regdef.h"
#else
# include <regdef.h>
# include <sys/asm.h>
#endif

/* Testing on a little endian machine showed using CLZ was a
   performance loss, so we are not turning it on by default.  */
#if defined(ENABLE_CLZ) && (__mips_isa_rev > 1)
# define USE_CLZ
#endif

/* Some asm.h files do not have the L macro definition.  */
#ifndef L
# if _MIPS_SIM == _ABIO32
#  define L(label) $L ## label
# else
#  define L(label) .L ## label
# endif
#endif

/* Some asm.h files do not have the PTR_ADDIU macro definition.  */
#ifndef PTR_ADDIU
# ifdef USE_DOUBLE
#  define PTR_ADDIU       daddiu
# else
#  define PTR_ADDIU       addiu
# endif
#endif

/* Allow the routine to be named something else if desired.  */
#ifndef STRCMP_NAME
# define STRCMP_NAME strcmp
#endif

#ifdef ANDROID_CHANGES
LEAF(STRCMP_NAME, 0)
#else
LEAF(STRCMP_NAME)
#endif
	.set	nomips16

	or	t0, a0, a1
	andi	t0,0x3
	bne	t0, zero, L(byteloop)

/* Both strings are 4 byte aligned at this point.  */

	li	t8, 0x01010101
	li	t9, 0x7f7f7f7f

#define STRCMP32(OFFSET) \
	lw	vt0, OFFSET(a0); \
	lw	vt1, OFFSET(a1); \
	subu	t0, vt0, t8; \
	nor	t1, vt0, t9; \
	bne	vt0, vt1, L(worddiff); \
	and	t0, t0, t1; \
	bne	t0, zero, L(returnzero)

L(wordloop):
	STRCMP32(0)
	STRCMP32(4)
	STRCMP32(8)
	STRCMP32(12)
	STRCMP32(16)
	STRCMP32(20)
	STRCMP32(24)
	lw	vt0, 28(a0)
	lw	vt1, 28(a1)
	subu	t0, vt0, t8
	nor	t1, vt0, t9
	bne	vt0, vt1, L(worddiff)
	and	t0, t0, t1
	PTR_ADDIU a0, a0, 32
	bne	t0, zero, L(returnzero)
	PTR_ADDIU a1, a1, 32
	b	L(wordloop)

L(returnzero):
	move	va0, zero
	jr	ra

L(worddiff):
#ifdef USE_CLZ
	subu	t0, vt0, t8
	nor	t1, vt0, t9
	and	t1, t0, t1
	xor	t0, vt0, vt1
	or	t0, t0, t1
# if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	wsbh	t0, t0
	rotr	t0, t0, 16
# endif
	clz	t1, t0
	and	t1, 0xf8
# if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	neg	t1
	addu	t1, 24
# endif
	rotrv	vt0, vt0, t1
	rotrv	vt1, vt1, t1
	and	vt0, vt0, 0xff
	and	vt1, vt1, 0xff
	subu	va0, vt0, vt1
	jr	ra
#else /* USE_CLZ */
# if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	andi	t0, vt0, 0xff
	andi	t1, vt1, 0xff
	beq	t0, zero, L(wexit01)
	srl	t8, vt0, 8
	bne	t0, t1, L(wexit01)

	srl	t9, vt1, 8
	andi	t8, t8, 0xff
	andi	t9, t9, 0xff
	beq	t8, zero, L(wexit89)
	srl	t0, vt0, 16
	bne	t8, t9, L(wexit89)

	srl	t1, vt1, 16
	andi	t0, t0, 0xff
	andi	t1, t1, 0xff
	beq	t0, zero, L(wexit01)
	srl	t8, vt0, 24
	bne	t0, t1, L(wexit01)

	srl	t9, vt1, 24
# else /* __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ */
	srl	t0, vt0, 24
	srl	t1, vt1, 24
	beq	t0, zero, L(wexit01)
	srl	t8, vt0, 16
	bne	t0, t1, L(wexit01)

	srl	t9, vt1, 16
	andi	t8, t8, 0xff
	andi	t9, t9, 0xff
	beq	t8, zero, L(wexit89)
	srl	t0, vt0, 8
	bne	t8, t9, L(wexit89)

	srl	t1, vt1, 8
	andi	t0, t0, 0xff
	andi	t1, t1, 0xff
	beq	t0, zero, L(wexit01)
	andi	t8, vt0, 0xff
	bne	t0, t1, L(wexit01)

	andi	t9, vt1, 0xff
# endif /* __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ */

L(wexit89):
	subu	va0, t8, t9
	jr	ra
L(wexit01):
	subu	va0, t0, t1
	jr	ra
#endif /* USE_CLZ */

/* It might seem better to do the 'beq' instruction between the two 'lbu'
   instructions so that the nop is not needed but testing showed that this
   code is actually faster (based on glibc strcmp test).  */
#define BYTECMP01(OFFSET) \
	lbu	vt1, OFFSET(a1); \
	nop; \
	beq	vt0, zero, L(bexit01); \
	lbu	t8, OFFSET+1(a0); \
	bne	vt0, vt1, L(bexit01)

#define BYTECMP89(OFFSET) \
	lbu	t9, OFFSET(a1); \
	nop;	\
	beq	t8, zero, L(bexit89); \
	lbu	vt0, OFFSET+1(a0); \
	bne	t8, t9, L(bexit89)

L(byteloop):
	lbu	vt0, 0(a0)
	BYTECMP01(0)
	BYTECMP89(1)
	BYTECMP01(2)
	BYTECMP89(3)
	BYTECMP01(4)
	BYTECMP89(5)
	BYTECMP01(6)
	lbu	t9, 7(a1)
	nop
	beq	t8, zero, L(bexit89)
	PTR_ADDIU a0, a0, 8
	bne	t8, t9, L(bexit89)
	PTR_ADDIU a1, a1, 8
	b	L(byteloop)

L(bexit01):
	subu	va0, vt0, vt1
	jr	ra
L(bexit89):
	subu	va0, t8, t9
	jr	ra

	.set	at

END(STRCMP_NAME)
