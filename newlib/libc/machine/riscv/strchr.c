#if defined(__OPTIMIZE_SIZE__) || defined(PREFER_SIZE_OVER_SPEED) || !defined(__riscv_vector) || defined(__riscv_e)
# include "../../string/strchr.c"
#else
/* strchr defined in strchr-asm.S */
#endif
