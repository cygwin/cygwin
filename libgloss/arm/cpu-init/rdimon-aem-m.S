/* Copyright (c) 2026 ARM Ltd.  All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. The name of the company may not be used to endorse or promote
    products derived from this software without specific prior written
    permission.

 THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/* This file gives a basic initialisation of an M-profile core.

   We define a hardfault handler, required to prevent the simulator
   from entering into an infinite loop.  This handler is located at
   address 0xc by default when linking with mprofile-mps2.specs which
   uses --section-start=.hardfault_handler_addr=0xc.

   Enable the FPU when __ARM_FP is set.  */

#if  (__ARM_ARCH_PROFILE == 'M')
    .syntax    unified
    .thumb

    @ CPU Initialisation
    .globl     _rdimon_hw_init_hook
    .type      _rdimon_hw_init_hook, %function

_rdimon_hw_init_hook:
    /* Enable FPU for M-profile only .  */
#if __ARM_FP
    /* Read CPACR */
    ldr        r0, =0xE000ED88
    ldr        r1, [R0]
    /* Enable full access to CP10 and CP11.  */
    orr        r1, r1, #(0xFF << 20)
    str        r1, [R0]
#if __ARM_ARCH >= 800
    ldr        r0, =0xE002ED88
    str        r1, [R0]
#endif
    dsb
    isb
#endif
    bx lr

    /* On M-profile, call abort in case of hardfault: this causes the
       simulator to abort execution instead of going into an infinite
       loop.  */
    .section .hardfault_handler_addr, "a"
    .word abort
#endif
