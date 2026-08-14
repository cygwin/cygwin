#include <sys/asm.h>

#if defined(__riscv_vector) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
ENTRY(strcpy)
  mv a2, a0
.Lloop:
  vsetvli a3, zero, e8, m1, ta, ma
  vle8ff.v v8, (a1)
  vmseq.vx v1, v8, zero
  csrr a4, vl
  vfirst.m a5, v1
  vmsif.m v0, v1
  add a1, a1, a4
  vse8.v v8, (a2), v0.t
  add a2, a2, a4
  bltz a5, .Lloop

  ret
END(strcpy)
#endif
