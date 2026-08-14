#include <sys/asm.h>

#if defined(__riscv_vector) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
ENTRY(strnlen)
  mv a2, a0
  mv a0, a1
.Lloop:
  beqz a1, .Ldone
  vsetvli zero, a1, e8, m1, ta, ma
  vle8ff.v v0, (a2)
  vmseq.vi v8, v0, 0
  vfirst.m a4, v8      /* first occurrence of \0 */
  csrr a3, vl
  add a2, a2, a3
  sub a1, a1, a3
  bltz a4, .Lloop

  add a1, a1, a3
  sub a0, a0, a1
  add a0, a0, a4
.Ldone:
  ret
END(strnlen)
#endif
