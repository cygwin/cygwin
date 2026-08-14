#include <sys/asm.h>

#if defined(__riscv_vector) && !defined(__riscv_e) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
/* size_t strcspn(const char *a0, const char *a1)
   Length of the initial segment of a0 made up of bytes NOT in a1.
   Build a 256-byte "stop" table on the stack (a1's bytes plus NUL marked),
   then scan a0: a gathered table entry != 0 ends the segment.  The scan of
   a0 uses vle8ff so it never loads across a page boundary; the indexed
   gather only touches the fully-mapped 256-byte table.  */
ENTRY(strcspn)
  addi sp, sp, -256
  /* zero the table */
  li t1, 256
  mv t0, sp
.Lzero:
  vsetvli t2, t1, e8, m8, ta, ma
  vmv.v.i v8, 0
  vse8.v v8, (t0)
  add t0, t0, t2
  sub t1, t1, t2
  bnez t1, .Lzero
  /* mark NUL as a stopper so the scan halts at end of string */
  li t1, 1
  sb t1, 0(sp)
  /* mark every byte of a1 */
.Lbuild:
  lbu t0, 0(a1)
  beqz t0, .Lscan
  add t2, sp, t0
  sb t1, 0(t2)
  addi a1, a1, 1
  j .Lbuild
.Lscan:
  mv a2, a0             /* remember start */
.Lloop:
  vsetvli t3, zero, e8, m1, ta, ma
  vle8ff.v v8, (a0)
  csrr t3, vl
  vluxei8.v v16, (sp), v8       /* gather table[a0[i]] */
  vmsne.vi v0, v16, 0           /* lanes that are stoppers */
  vfirst.m t4, v0
  bgez t4, .Lfound
  add a0, a0, t3
  j .Lloop
.Lfound:
  add a0, a0, t4
  sub a0, a0, a2
  addi sp, sp, 256
  ret
END(strcspn)
#endif
