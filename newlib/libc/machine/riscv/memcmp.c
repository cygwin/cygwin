#if defined(__OPTIMIZE_SIZE__) || defined(PREFER_SIZE_OVER_SPEED) || !defined(__riscv_vector)
# include "../../string/memcmp.c"
#else
/* memcmp defined in memcmp-asm.S */
#endif
