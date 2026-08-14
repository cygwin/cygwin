#include <sys/asm.h>

#if defined(__riscv_vector) && !defined(__riscv_e) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
/* char *strpbrk(const char *a0, const char *a1)
   Return a pointer to the first byte of a0 that appears in a1, or NULL.
   Equivalent to s + strcspn(s, b), returning NULL when that lands on the
   terminating NUL.  Same page-safe vle8ff scan + stack-table gather as
   strcspn; the NUL is marked as a stopper so the scan terminates.  */
ENTRY(strpbrk)
  addi sp, sp, -256
  li t1, 256
  mv t0, sp
.Lzero:
  vsetvli t2, t1, e8, m8, ta, ma
  vmv.v.i v8, 0
  vse8.v v8, (t0)
  add t0, t0, t2
  sub t1, t1, t2
  bnez t1, .Lzero
  li t1, 1
  /* mark every byte of a1 (the set), but NOT NUL yet */
.Lbuild:
  lbu t0, 0(a1)
  beqz t0, .Lsetnul
  add t2, sp, t0
  sb t1, 0(t2)
  addi a1, a1, 1
  j .Lbuild
.Lsetnul:
  /* mark NUL as a stopper so the scan halts at end of string */
  sb t1, 0(sp)
.Lloop:
  vsetvli t3, zero, e8, m1, ta, ma
  vle8ff.v v8, (a0)
  csrr t3, vl
  vluxei8.v v16, (sp), v8
  vmsne.vi v0, v16, 0
  vfirst.m t4, v0
  bgez t4, .Lfound
  add a0, a0, t3
  j .Lloop
.Lfound:
  add a0, a0, t4
  lbu t5, 0(a0)         /* did we stop on a real match or on the NUL? */
  bnez t5, .Ldone
  li a0, 0
.Ldone:
  addi sp, sp, 256
  ret
END(strpbrk)
#endif
