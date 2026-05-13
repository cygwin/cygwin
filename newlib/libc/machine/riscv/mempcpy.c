#if defined(__OPTIMIZE_SIZE__) || defined(PREFER_SIZE_OVER_SPEED) || !defined(__riscv_vector)
# include "../../string/mempcpy.c"
#else
/* mempcpy defined in mempcpy-asm.S */
#endif
