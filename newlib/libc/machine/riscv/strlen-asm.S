#include <sys/asm.h>

#if defined(__riscv_vector) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
ENTRY(strlen)
  mv a1, a0
.Lloop:
  vsetvli a2, zero, e8, m2, ta, ma
  vle8ff.v v0, (a1)
  csrr a2, vl
  vmseq.vi v2, v0, 0
  vfirst.m a3, v2
  add a1, a1, a2
  bltz a3, .Lloop

  add a0, a0, a2
  add a1, a1, a3
  sub a0, a1, a0

  ret
END(strlen)
#endif
