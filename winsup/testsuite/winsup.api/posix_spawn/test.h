#ifndef _POSIX_SPAWN_TEST_H_
#define _POSIX_SPAWN_TEST_H_

#define _GNU_SOURCE
#include <errno.h>
#include <error.h>
#include <signal.h>
#include <sys/wait.h>

#define negError(x) do { \
  if ((x) < 0) \
    error_at_line(1, errno, __FILE__, __LINE__, "%s", #x); \
} while (0)

#define nullError(x) do { \
  if (!(x)) \
    error_at_line(1, errno, __FILE__, __LINE__, "%s", #x); \
} while (0)

#define sigError(x) do { \
  if ((x) == SIG_ERR) \
    error_at_line(1, errno, __FILE__, __LINE__, "%s", #x); \
} while (0)

#define errCodeExpected(expected, x) do { \
  int _errcode = (x); \
  if (_errcode != (expected)) \
    error_at_line(1, _errcode, __FILE__, __LINE__, "%s", #x); \
} while (0)

#define errCode(x) errCodeExpected(0, x)

#define exitStatus(status, expectedExitCode) do { \
  if (WIFSIGNALED ((status))) \
    error_at_line (128 + WTERMSIG ((status)), 0, __FILE__, __LINE__ - 2, \
		   "child terminated with signal %d", WTERMSIG ((status))); \
  else if (WIFEXITED ((status)) && WEXITSTATUS ((status)) != (expectedExitCode)) \
    error_at_line (WEXITSTATUS ((status)), 0, __FILE__, __LINE__ - 2, \
		   "child exited with code %d", WEXITSTATUS ((status))); \
} while (0)

/* first vararg to testAssertMsg must be string msg */
#define testAssertMsg(cond, ...) do { \
  if (!(cond)) \
    error_at_line (1, 0, __FILE__, __LINE__, __VA_ARGS__); \
} while (0);

#define testAssert(cond) testAssertMsg(cond, "%s", #cond)

#define MYSELF "/proc/self/exe"

#endif /* _POSIX_SPAWN_TEST_H_ */

