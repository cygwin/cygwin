#if defined(__riscv_vector) && !defined(__OPTIMIZE_SIZE__) && !defined(PREFER_SIZE_OVER_SPEED)
.text
.global memcmp
.type memcmp, @function
memcmp:
#if __riscv_landing_pad
  lpad 0
#endif
  beqz a2, .Lequal
.Lloop:
  vsetvli a3, a2, e8, m8, ta, ma

  vle8.v v0, (a0)
  vle8.v v8, (a1)

  vmsne.vv v16, v0, v8
  sub a2, a2, a3
  vfirst.m a4, v16

  bgez a4, .Lfound

  add a0, a0, a3
  add a1, a1, a3

  bnez a2, .Lloop

.Lequal:
  li a0, 0
  ret
.Lfound:
  vrgather.vx v16, v0, a4
  vrgather.vx v24, v8, a4
  vmv.x.s a0, v16
  vmv.x.s a4, v24
  andi a0, a0, 0xff
  andi a4, a4, 0xff
  sub a0, a0, a4
  ret
.size memcmp, .-memcmp
#endif
