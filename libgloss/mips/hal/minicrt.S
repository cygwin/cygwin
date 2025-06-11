#include <mips/asm.h>
#include <mips/regdef.h>

LEAF(_start)
	/* Preserve return address */
	move s0, ra
	/* Set up global pointer for small data access */
	LA gp, _gp
	/* Set up stack pointer */
	LA sp, __stack
	/* Zero the BSS */
	LA v0, _fbss
	LA v1, _end
	beq v0,v1,2f
1:
	addiu v0,v0,4
	sw zero,-4(v0)
	bne v0,v1,1b
2:
	/* Set arguments to be null for main */
	li a0, 0

	/* Reserve the ABI required argument area */
	addiu sp, sp, -(NARGSAVE * SZARG)
	/* Jump to C code */
	jal main

	/* Return to boot */
	move ra, s0
	jr ra
END(_start)
