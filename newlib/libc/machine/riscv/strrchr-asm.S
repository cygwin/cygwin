#include <sys/asm.h>

#if defined(__riscv_vector) && !defined(__riscv_e) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
/* char *strrchr(const char *a0, int a1)
   Return the last occurrence of (char)a1 in a0, including the terminating
   NUL when a1==0, else NULL.  Single forward pass: each chunk is loaded with
   vle8ff (page-boundary safe) and the highest matching lane index is found
   with a masked max-index reduction.  */
ENTRY(strrchr)
  li a6, 0                  /* result pointer = NULL so far */
.Lloop:
  vsetvli a2, zero, e8, m1, ta, ma
  vle8ff.v v8, (a0)
  csrr a2, vl
  vmseq.vi v1, v8, 0        /* NUL lanes */
  vfirst.m a3, v1           /* first NUL position (or -1) */
  vmseq.vx v2, v8, a1       /* lanes equal to (char)c */
  vmsif.m v3, v1            /* valid lanes: up to & including first NUL */
  vmand.mm v0, v2, v3       /* candidate matches within the string */
  vfirst.m a4, v0           /* any candidate in this chunk? */
  bltz a4, .Lskip
  /* reduce the maximum lane index over candidate lanes (need e16 to be
     safe for large VLEN where vl can exceed 255).  */
  vsetvli zero, a2, e16, m2, ta, ma
  vid.v v16
  vmv.s.x v24, x0
  vredmaxu.vs v24, v16, v24, v0.t
  vmv.x.s a5, v24
  add a6, a0, a5
.Lskip:
  bgez a3, .Ldone           /* reached end of string */
  add a0, a0, a2
  j .Lloop
.Ldone:
  mv a0, a6
  ret
END(strrchr)
#endif
