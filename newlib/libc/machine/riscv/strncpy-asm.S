#include <sys/asm.h>

#if defined(__riscv_vector) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
/* char *strncpy(char *a0, const char *a1, size_t a2)
   Copy at most a2 bytes; if a NUL is met in src, the rest of dest is
   zero-filled.  Source is read with vle8ff (page-boundary safe) and vl is
   also capped by the remaining count so we never read past byte a2.  */
ENTRY(strncpy)
  mv a3, a0                 /* keep a running destination pointer */
  beqz a2, .Ldone
.Lloop:
  vsetvli zero, a2, e8, m1, ta, ma
  vle8ff.v v8, (a1)
  vmseq.vi v1, v8, 0        /* NUL mask */
  csrr a4, vl               /* actual count (may be truncated by fault) */
  vfirst.m a5, v1
  bgez a5, .Lnul
  /* no NUL: copy whole chunk */
  vse8.v v8, (a3)
  add a1, a1, a4
  add a3, a3, a4
  sub a2, a2, a4
  bnez a2, .Lloop
  j .Ldone
.Lnul:
  /* NUL at index a5: copy the [0,a5) prefix, then zero-fill the rest */
  vmsbf.m v0, v1            /* mask = elements before first NUL */
  vse8.v v8, (a3), v0.t
  add a3, a3, a5
  sub a2, a2, a5            /* remaining bytes to zero (>=1) */
.Lfill:
  beqz a2, .Ldone
  vsetvli a4, a2, e8, m8, ta, ma
  vmv.v.i v8, 0
  vse8.v v8, (a3)
  add a3, a3, a4
  sub a2, a2, a4
  bnez a2, .Lfill
.Ldone:
  ret
END(strncpy)
#endif
