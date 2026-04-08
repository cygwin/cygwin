#include <sys/time.h>
#include <errno.h>

int
gettimeofday (struct timeval *tv, void *tz)
{
  errno = ENOSYS;
  return -1;
}
