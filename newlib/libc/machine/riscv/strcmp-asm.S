#include <sys/asm.h>

#if defined(__riscv_vector) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
ENTRY(strcmp)
.Lloop:
  vsetvli a2, zero, e8, m1, ta, ma
  vle8ff.v v0, (a0)
  /* check if v0[i] == 0 */
  vmseq.vx v16, v0, zero

  vle8ff.v v8, (a1)
  /* check if v0[i] != v8[i] */
  vmsne.vv v17, v0, v8

  /* find the index x for v0[x]==0 */
  vfirst.m a3, v16
  /* find the index x for v0[x]!=v8[x] */
  vfirst.m a4, v17

  bgez a3, .Lcheck1
  bgez a4, .Lcheck2

  /* get the current vl updated by vle8ff. */
  csrr a2, vl
  add a0, a0, a2
  add a1, a1, a2
  j .Lloop

  /* a3>=0 */
.Lcheck1:
  bltz a4, .Lreturn_at_nul
  blt a4, a3, .Lcheck2
.Lreturn_at_nul:
  /* a4<0, or a4>=0 && a3<=a4 */
  add a0, a0, a3
  add a1, a1, a3
  lbu a3, 0(a0)
  lbu a4, 0(a1)
  sub a0, a3, a4
  ret

  /* a3<0 && a4>=0, or a4<a3 */
.Lcheck2:
  add a0, a0, a4
  add a1, a1, a4
  lbu a3, 0(a0)
  lbu a4, 0(a1)
  sub a0, a3, a4
  ret
END(strcmp)
#endif
