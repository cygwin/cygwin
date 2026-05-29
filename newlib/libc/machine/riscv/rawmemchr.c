#if defined(__OPTIMIZE_SIZE__) || defined(PREFER_SIZE_OVER_SPEED) || !defined(__riscv_vector)
# include "../../string/rawmemchr.c"
#else
/* rawmemchr defined in rawmemchr-asm.S */
#endif
