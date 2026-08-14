#if defined(__OPTIMIZE_SIZE__) || defined(PREFER_SIZE_OVER_SPEED) || !defined(__riscv_vector)
# include "../../string/strncpy.c"
#else
/* strncpy defined in strncpy-asm.S */
#endif
