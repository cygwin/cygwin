#include "test.h"
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (void)
{
  pid_t pid;
  int status;
  /* the test installation has very limited binaries on the PATH, but sh is one
     of them and 'true' should be a builtin */
  char *childargv[] = {"sh", "-c", "true", NULL};
  char *childenv[] = {NULL};

  /* unbuffer stdout */
  setvbuf(stdout, NULL, _IONBF, 0);

  /* can posix_spawnp find a program even with an empty environment? */
  errCode (posix_spawnp (&pid, childargv[0], NULL, NULL, childargv, childenv));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);

  return 0;
}
