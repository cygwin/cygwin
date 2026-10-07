/* Test signal delivery after a pthread_once initializer handles a signal. */

#include "test.h"
#include <poll.h>
#include <signal.h>
#include <time.h>

static volatile sig_atomic_t seen;

static void
handler (int signo)
{
  (void) signo;
  ++seen;
}

static void
initialize (void)
{
  assert (raise (SIGALRM) == 0);
}

int
main (void)
{
  struct sigaction action = {0};
  stack_t stack = {0};
  stack.ss_sp = malloc (SIGSTKSZ);
  stack.ss_size = SIGSTKSZ;
  assert (stack.ss_sp != NULL);
  assert (sigaltstack (&stack, NULL) == 0);

  action.sa_handler = handler;
  assert (sigemptyset (&action.sa_mask) == 0);

  for (int onstack = 0; onstack < 2; ++onstack)
    {
      pthread_once_t once = PTHREAD_ONCE_INIT;
      action.sa_flags = onstack ? SA_ONSTACK : 0;
      assert (sigaction (SIGALRM, &action, NULL) == 0);
      seen = 0;
      assert (pthread_once (&once, initialize) == 0);
      assert (seen == 1);

      struct timespec before, after;
      assert (clock_gettime (CLOCK_MONOTONIC, &before) == 0);
      alarm (1);
      assert (poll (NULL, 0, 5000) == -1 && errno == EINTR);
      assert (clock_gettime (CLOCK_MONOTONIC, &after) == 0);
      long elapsed_ms = (after.tv_sec - before.tv_sec) * 1000
			+ (after.tv_nsec - before.tv_nsec) / 1000000;
      assert (elapsed_ms < 4000);
      assert (seen == 2);
    }

  stack.ss_flags = SS_DISABLE;
  assert (sigaltstack (&stack, NULL) == 0);
  free (stack.ss_sp);
  return 0;
}
