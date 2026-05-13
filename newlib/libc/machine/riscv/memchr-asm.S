#if defined(__riscv_vector) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
.text
.global memchr
.type memchr, @function
memchr:
#if __riscv_landing_pad
  lpad 0
#endif
  beqz a2, .Lnot_found
  andi a1, a1, 0xff
.Lloop:
  vsetvli zero, a2, e8, m8, ta, ma

  vle8ff.v v0, (a0)
  /* Find the c(a1) inside the loaded data.  */
  vmseq.vx v8, v0, a1
  vfirst.m a4, v8

  bgez a4, .Lfound

  csrr a3, vl
  sub a2, a2, a3
  add a0, a0, a3

  bnez a2, .Lloop

.Lnot_found:
  li a0, 0
  ret
.Lfound:
  add a0, a0, a4
  ret
.size memchr, .-memchr
#endif
