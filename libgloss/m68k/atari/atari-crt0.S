/*
	Copyright (C) 2025 Mikael Hildenborg
	SPDX-License-Identifier: BSD-2-Clause
*/

	.global exit

	.section	".init"
	.global	__call_main_with_argc_argv
	.type	__call_main_with_argc_argv,#function
__call_main_with_argc_argv:

/*
	Need to implement "The Atari Extended Argument Specification".
	Also, there is no guarantee that the command line is zero terminated.
*/
    | this occurs after crtbegin.o have done all global constructors etc.

	move.l	a7, a6
	moveq	#0, d0
	moveq	#0, d2
	movea.l	_BasePage, a0
	lea		128(a0), a0		| add offset to the cmdline
	lea		_cmdline, a1
	move.b	(a0)+, d2		| d2 contains number of bytes in command line (max 127)
	beq.s	3f
	clr.b	(a1, d2.w)		| end our decoded args with a zero.
	bra.s	2f
1:
	move.b	(a0, d2.w), d1
	cmp.b	#' ', d1
	bne.s	4f
	moveq	#0, d1
	lea		1(a1, d2.w), a2
	tst.b	(a2)
	beq.s	4f
	move.l	a2, -(a7)
	addq.w	#1, d0
4:
	move.b	d1, (a1, d2.w)
2:
	subq.w	#1, d2
	bcc.s	1b
	move.l	a1, -(a7)
	addq.w	#1, d0
3:
	pea		_procname		| first argument is always the proc name. That we do not know...
	addq.w	#1, d0
	move.l	a7, a5
	move.l	a6, -(a7)		| To know where to move it back again.

	move.l	a5, -(a7)		| argv
	move.l	d0, -(a7)		| argc
	jsr		main
	move.l	8(a7), a7		| move it back.
	move.l	d0, -(a7)
	jsr		exit			| not expected to return
	illegal

	.global _exit
	.type	_exit,#function
_exit:
	move.l 4(a7), d0
	move.w	d0, _program_return_code
    | crtend.o follows here with global destructors etc.

	.data
_procname:
	.asciz	"yourapp.lol"
	.even

	.bss
	.lcomm	_cmdline, 128
	.even
