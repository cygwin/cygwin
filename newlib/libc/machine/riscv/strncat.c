#if defined(__OPTIMIZE_SIZE__) || defined(PREFER_SIZE_OVER_SPEED) || !defined(__riscv_vector)
# include "../../string/strncat.c"
#else
/* strncat defined in strncat-asm.S */
#endif
