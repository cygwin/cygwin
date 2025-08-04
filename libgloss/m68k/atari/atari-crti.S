/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: BSD-2-Clause
*/

#ifdef _HAVE_INITFINI_ARRAY
#define _init	__libc_init_array
#define _fini	__libc_fini_array
#endif

	.equ	DEFAULT_STACK, 2000

	.global _BasePage
	.weak 	_STACK_SIZE
	.weak 	_HEAP_SIZE
	.global _HeapPtr
	.global _HeapBottom
	.global _HeapTop
	.global _atari_4ba_at_prg_start
	.global __BSS_SEGMENT_END

	.section	".init"
	.global	_init
	.type	_init,#function
_init:
	move.l	4(a7),a0
	move.l	a0, _BasePage

	| Init stack
	lea		__BSS_SEGMENT_END, a2
	lea		_STACK_SIZE, a1
	cmpa.w	#0, a1
	jeq		default_stack_size
	add.l	(a1), a2
	jra		stack_size_selected
default_stack_size:
	lea		DEFAULT_STACK(a2), a2
stack_size_selected:
	move.l	a2, a7

	| Init heap
	move.l	a7, d0
	move.l	d0, _HeapBottom
	move.l	d0, _HeapPtr
	move.l	4(a0), _HeapTop

	lea		_HEAP_SIZE, a1
	cmpa.w	#0, a1
	jeq		heap_setup_done
	add.l	(a1), d0
	sub.l	(a0), d0			| d0 is now the TPA size

	| Program do not want all memory, so we shrink it.
	move.l	d0, -(a7)
	move.l	a0, -(a7)
	clr.w	-(a7)
	move.w	#0x4a, -(a7)	| Mshrink()
	trap	#1
	lea		12(a7), a7
	tst.l	d0
	jpl		heap_setup_done
    | Error, just quit with d0 as return code.
	move.w	d0,-(a7)
	move.w	#0x4c,-(a7)
	trap	#1
super_init:
	| Init stuff that needs supervisor mode set.
	move.l	0x4ba, _atari_4ba_at_prg_start
	rts
heap_setup_done:
	move.l	#super_init, -(a7)
	move.w	#0x26, -(a7)
	trap	#14
	addq.l	#6, a7
    | crtbegin.o follows here with global constructors etc. init.


	.section	".fini"
	/*
		Empty.
	*/


	.bss
	.lcomm 	_BasePage, 4
	.lcomm 	_HeapPtr, 4
	.lcomm 	_HeapBottom, 4
	.lcomm 	_HeapTop, 4
	.lcomm	_atari_4ba_at_prg_start, 4
	.even