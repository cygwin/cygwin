/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <_mingw_mac.h>

	.file	"remainderf.S"
	.text
#if defined(__x86_64__)
	.align 8
#elif defined(__i386__)
	.align 4
#endif
.globl __MINGW_USYMBOL(remainder)
	.def	__MINGW_USYMBOL(remainderf);	.scl	2;	.type	32;	.endef
__MINGW_USYMBOL(remainderf):
#if defined(__x86_64__)
	movss	%xmm1,-12(%rsp)
	flds	-12(%rsp)
	movss	%xmm0,-12(%rsp)
	flds	-12(%rsp)
1:	fprem1
	fstsw	%ax
	sahf
	jp	1b
	fstp	%st(1)
	fstps	-12(%rsp)
	movss	-12(%rsp),%xmm0
	ret
#elif defined(__i386__)
	flds	8(%esp)
	flds	4(%esp)
1:	fprem1
	fstsw	%ax
	sahf
	jp	1b
	fstp	%st(1)
	ret
#elif __SIZEOF_LONG_DOUBLE__ == __SIZEOF_DOUBLE__
	/* AArch64: system libm provides remainderf() natively, no stub needed */
#else
	.error "Not supported on your platform yet"
#endif
