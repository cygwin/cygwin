#if defined(__riscv_vector) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
.text
.global rawmemchr
.type rawmemchr, @function
rawmemchr:
#if __riscv_landing_pad
  lpad 0
#endif
  andi a1, a1, 0xff
.Lloop:
  vsetvli t0, zero, e8, m1, ta, ma

  vle8ff.v v0, (a0)
  vmseq.vx v8, v0, a1
  vfirst.m a2, v8

  bgez a2, .Lfound

  csrr a3, vl
  add a0, a0, a3
  j .Lloop

.Lfound:
  add a0, a0, a2
  ret
.size rawmemchr, .-rawmemchr
#endif
