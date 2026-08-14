#include <sys/asm.h>

#if defined(__riscv_vector) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
ENTRY(strncat)
  mv a3, a0
  /* the strlen of a0.  */
.Lstrlen_loop:
  vsetvli a4, zero, e8, m1, ta, ma

  vle8ff.v v8, (a3)
  /* find the '\0'.  */
  vmseq.vx v0, v8, zero
  csrr a4, vl
  vfirst.m a5, v0
  add a3, a3, a4
  bltz a5, .Lstrlen_loop

  sub a3, a3, a4
  add a3, a3, a5

  /* copy at most a2 bytes of a1 to a3.  */
.Lstrcpy_loop:
  vsetvli zero, a2, e8, m1, ta, ma

  vle8ff.v v8, (a1)
  vmseq.vx v1, v8, zero
  csrr a4, vl
  vfirst.m a5, v1
  vmsif.m v0, v1
  add a1, a1, a4
  sub a2, a2, a4
  vse8.v v8, (a3), v0.t
  add a3, a3, a4
  beqz a2, .Lfill_zero
  bltz a5, .Lstrcpy_loop
  ret
.Lfill_zero:
  bgez a5, .Lfill_zero_end
  sb zero, (a3)
.Lfill_zero_end:
  ret
END(strncat)
#endif
