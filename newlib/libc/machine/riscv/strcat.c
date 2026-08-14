#if defined(__OPTIMIZE_SIZE__) || defined(PREFER_SIZE_OVER_SPEED) || !defined(__riscv_vector)
# include "../../string/strcat.c"
#else
/* strcat defined in strcat-asm.S */
#endif
