#include <sys/asm.h>

#if defined(__riscv_vector) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
ENTRY(strncmp)
  beqz a2, .Lzero_length
.Lloop:
  vsetvli zero, a2, e8, m1, ta, ma

  vle8ff.v v0, (a0)
  /* v0[i] == 0.  */
  vmseq.vx v8, v0, zero

  vle8ff.v v4, (a1)
  /* v0[i] != v4[i].  */
  vmsne.vv v9, v0, v4

  csrr a3, vl

  /* r = mask1 | mask2
     Combine the first NUL in a0 with the first differing byte so a single
     vfirst.m locates whichever comes first.  */
  vmor.mm v8, v8, v9

  sub a2, a2, a3

  vfirst.m a4, v8

  bgez a4, .Lend_loop

  add a0, a0, a3
  add a1, a1, a3
  bnez a2, .Lloop
.Lend_loop:

  add a0, a0, a4
  add a1, a1, a4
  lbu a4, 0(a0)
  lbu a5, 0(a1)

  sub a0, a4, a5
  ret

.Lzero_length:
  li a0, 0
  ret
END(strncmp)
#endif
