#include <_ansi.h>
#include <sys/types.h>
#include "sys/syscall.h"

extern int __trap34 (int function, ...);

int
ftruncate (int file, off_t length)
{
  return __trap34 (SYS_ftruncate, file, length, 0);
}
