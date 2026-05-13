#if defined(__riscv_vector) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
.text
.global memrchr
.type memrchr, @function
memrchr:
#if __riscv_landing_pad
  lpad 0
#endif
  andi a1, a1, 0xff
  add a0, a0, a2
.Lloop:
  beqz a2, .Lnohit

  vsetvli a3, a2, e8, m4, ta, ma
  sub a0, a0, a3
  vle8.v v8, (a0)
  sub a2, a2, a3

  vmseq.vx v0, v8, a1
  vfirst.m a4, v0
  bltz a4, .Lloop

  vsetvli zero, a3, e16, m8, ta, ma
  vid.v v16

  /* Reduce max index over hit lanes.  */
  vmv.s.x v24, x0
  vredmaxu.vs v24, v16, v24, v0.t
  vmv.x.s a5, v24

  add a0, a0, a5
  ret
.Lnohit:
  li a0, 0
  ret
.size memrchr, .-memrchr
#endif
