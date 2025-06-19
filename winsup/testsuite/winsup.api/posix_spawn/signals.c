#include "test.h"
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int handle_child (char *arg)
{
  struct sigaction sa;
  sigset_t mask;
  int ret;

  negError (sigaction (SIGUSR1, NULL, &sa));
  negError (sigprocmask (SIG_SETMASK, NULL, &mask));
  negError (ret = sigismember (&mask, SIGUSR2));

  if (!strcmp (arg, "inherited"))
    {
      testAssert (sa.sa_handler == SIG_IGN);
      testAssertMsg (ret, "SIGUSR2 not masked");
    }
  else
    {
      testAssert (sa.sa_handler == SIG_DFL);
      testAssertMsg (!ret, "SIGUSR2 masked");
    }

  return 0;
}

int main (int argc, char **argv)
{
  posix_spawnattr_t sa;
  pid_t pid;
  int status;
  sigset_t sigusr1mask, sigusr2mask, emptymask;
  char *childargv[] = {"signal", "--child", "inherited", NULL};

  /* unbuffer stdout */
  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc == 3 && !strcmp (argv[1], "--child"))
    return handle_child (argv[2]);

  negError (sigemptyset (&sigusr1mask));
  negError (sigaddset (&sigusr1mask, SIGUSR1));
  negError (sigemptyset (&sigusr2mask));
  negError (sigaddset (&sigusr2mask, SIGUSR2));
  negError (sigemptyset (&emptymask));

  /* set all signals to default */
  for (int i = 1; i < NSIG; ++i)
    if (i != SIGKILL && i != SIGSTOP)
      signal (i, SIG_DFL);

  /* change some signal states to test signal-related posix_spawn flags */
  sigError (signal (SIGUSR1, SIG_IGN));
  negError (sigprocmask (SIG_SETMASK, &sigusr2mask, NULL));

  /* ensure ignored and blocked signals are inherited by default */
  errCode (posix_spawn (&pid, MYSELF, NULL, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);

  errCode (posix_spawnattr_init (&sa));
  errCode (posix_spawnattr_setsigmask (&sa, &emptymask));
  errCode (posix_spawnattr_setsigdefault (&sa, &sigusr1mask));
  errCode (posix_spawnattr_setflags (&sa,
	POSIX_SPAWN_SETSIGDEF|POSIX_SPAWN_SETSIGMASK));

  /* ensure setsigmask and setsigdefault work */
  childargv[2] = "spawnattr";
  errCode (posix_spawn (&pid, MYSELF, NULL, &sa, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);

  errCode (posix_spawnattr_destroy (&sa));

  return 0;
}
