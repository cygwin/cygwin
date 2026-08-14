#if defined(__OPTIMIZE_SIZE__) || defined(PREFER_SIZE_OVER_SPEED) || !defined(__riscv_vector) || defined(__riscv_e)
# include "../../string/strspn.c"
#else
/* strspn defined in strspn-asm.S */
#endif
