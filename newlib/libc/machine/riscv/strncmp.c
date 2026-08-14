#if defined(__OPTIMIZE_SIZE__) || defined(PREFER_SIZE_OVER_SPEED) || !defined(__riscv_vector)
# include "../../string/strncmp.c"
#else
/* strncmp defined in strncmp-asm.S */
#endif
