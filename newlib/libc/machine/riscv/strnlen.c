#if defined(__OPTIMIZE_SIZE__) || defined(PREFER_SIZE_OVER_SPEED) || !defined(__riscv_vector)
# include "../../string/strnlen.c"
#else
/* strnlen defined in strnlen-asm.S */
#endif
