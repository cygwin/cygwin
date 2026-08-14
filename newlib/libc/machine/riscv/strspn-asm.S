#include <sys/asm.h>

#if defined(__riscv_vector) && !defined(__riscv_e) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
/* size_t strspn(const char *a0, const char *a1)
   Length of the initial segment of a0 made up entirely of bytes in a1.
   Build a 256-byte "accept" table from a1 (NUL is never marked, since a1 is
   a string), then scan a0 and stop at the first byte not in the set; a NUL
   in a0 stops the scan naturally.  The scan uses vle8ff (page-boundary safe);
   the gather only touches the fully-mapped 256-byte table.  */
ENTRY(strspn)
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
.Lbuild:
  lbu t0, 0(a1)
  beqz t0, .Lscan
  add t2, sp, t0
  sb t1, 0(t2)
  addi a1, a1, 1
  j .Lbuild
.Lscan:
  mv a2, a0
.Lloop:
  vsetvli t3, zero, e8, m1, ta, ma
  vle8ff.v v8, (a0)
  csrr t3, vl
  vluxei8.v v16, (sp), v8       /* gather accept[a0[i]] */
  vmseq.vi v0, v16, 0           /* lanes NOT in the accept set */
  vfirst.m t4, v0
  bgez t4, .Lfound
  add a0, a0, t3
  j .Lloop
.Lfound:
  add a0, a0, t4
  sub a0, a0, a2
  addi sp, sp, 256
  ret
END(strspn)
#endif
