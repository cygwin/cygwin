/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <_mingw_mac.h>

	.file	"scalbnl.S"
	.text
#if defined(__x86_64__)
	.align 8
#elif defined(__i386__)
	.align 4
#endif
.globl __MINGW_USYMBOL(scalbnl)
	.def	__MINGW_USYMBOL(scalbnl);	.scl	2;	.type	32;	.endef
__MINGW_USYMBOL(scalbnl):
#if defined(__x86_64__)
	subq  $24, %rsp
	andl    $-1, %r8d
	movq	%r8, (%rsp)
	fildl	(%rsp)
	fldt	(%rdx)
	fscale
	fstp	%st(1)
	movq	%rcx,%rax
	movq	$0,8(%rcx)
	fstpt	(%rcx)
	addq $24, %rsp
	ret
#elif defined(__i386__)
	fildl	16(%esp)
	fldt	4(%esp)
	fscale
	fstp	%st(1)
	ret
#elif __SIZEOF_LONG_DOUBLE__ == __SIZEOF_DOUBLE__
	scvtf d1, w1
	fmov d2, #2.0
	bl pow
	fmul d0, d0, d1
	ret
#else
	.error "Not supported on your platform yet"
#endif

.globl __MINGW_USYMBOL(scalblnl)
	.set	__MINGW_USYMBOL(scalblnl),__MINGW_USYMBOL(scalbnl)
