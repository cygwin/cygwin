/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <_mingw_mac.h>

	.file	"remainder.S"
	.text
#if defined(__x86_64__)
	.align 8
#elif defined(__i386__)
	.align 4
#endif
.globl __MINGW_USYMBOL(remainder)
	.def	__MINGW_USYMBOL(remainder);	.scl	2;	.type	32;	.endef
__MINGW_USYMBOL(remainder):
#if defined(__x86_64__)
	movsd	%xmm0,-16(%rsp)
	movsd	%xmm1,-32(%rsp)
	fldl	-32(%rsp)
	fldl	-16(%rsp)
1:	fprem1
	fstsw	%ax
	sahf
	jp	1b
	fstp	%st(1)
	fstpl	-16(%rsp)
	movsd	-16(%rsp),%xmm0
	ret
#elif defined(__i386__)
	fldl	12(%esp)
	fldl	4(%esp)
1:	fprem1
	fstsw	%ax
	sahf
	jp	1b
	fstp	%st(1)
	ret
#elif __SIZEOF_LONG_DOUBLE__ == __SIZEOF_DOUBLE__
	/* AArch64: system libm provides remainder() natively, no stub needed */
#else
	.error "Not supported on your platform yet"
#endif
