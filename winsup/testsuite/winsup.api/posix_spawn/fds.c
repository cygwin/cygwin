#include "test.h"
#include <fcntl.h>
#include <limits.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int handle_child (char *devfd, char *target)
{
  char buf[PATH_MAX];
  ssize_t ret;

  ret = readlink (devfd, buf, PATH_MAX);
  if (ret < 0)
    {
      int err = errno;
      if (err == ENOENT && !strcmp (target, "<ENOENT>"))
	return 0;
      error_at_line (1, err, __FILE__, __LINE__ - 6,
		     "ret = readlink (devfd, buf, PATH_MAX)");
    }
  testAssertMsg (ret < PATH_MAX, "Path too long for PATH_MAX buffer");
  buf[ret] = '\0';
  if (strcmp (target, buf))
      error_at_line (1, 0, __FILE__, __LINE__ - 12,
		     "Target '%s' != expected '%s'", buf, target);

  return 0;
}

int main (int argc, char **argv)
{
  posix_spawn_file_actions_t fa;
  pid_t pid;
  int status;
  int fd, fdcloexec;
  char buf[16];
  char *childargv[] = {"fds", "--child", buf, "", NULL};

  /* unbuffer stdout */
  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc == 4 && !strcmp (argv[1], "--child"))
    return handle_child (argv[2], argv[3]);

  /* open file descriptors to test inheritance */
  negError (fd = open ("/dev/null", O_RDONLY, 0644));
  negError (fdcloexec = open ("/dev/full", O_RDONLY|O_CLOEXEC, 0644));

  /* ensure fd is inherited by default */
  sprintf (buf, "/dev/fd/%d", fd);
  childargv[3] = "/dev/null";
  errCode (posix_spawn (&pid, MYSELF, NULL, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);

  /* ensure CLOEXEC fd is closed */
  sprintf (buf, "/dev/fd/%d", fdcloexec);
  childargv[3] = "<ENOENT>";
  errCode (posix_spawn (&pid, MYSELF, NULL, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);

  /* test posix_spawn_file_actions_addopen */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addopen (&fa, 0, "/dev/zero", O_RDONLY,
					     0644));
  strcpy (buf, "/dev/fd/0");
  childargv[3] = "/dev/zero";
  errCode (posix_spawn (&pid, MYSELF, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_addopen with O_CLOEXEC */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addopen (&fa, 0, "/dev/zero",
					     O_RDONLY|O_CLOEXEC, 0644));
  childargv[3] = "<ENOENT>";
  errCode (posix_spawn (&pid, MYSELF, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_adddup2 */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_adddup2 (&fa, fd, 0));
  childargv[3] = "/dev/null";
  errCode (posix_spawn (&pid, MYSELF, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_adddup2 with CLOEXEC fd */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_adddup2 (&fa, fdcloexec, 0));
  childargv[3] = "/dev/full";
  errCode (posix_spawn (&pid, MYSELF, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_adddup2 with out to err */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addopen (&fa, 1, "/dev/zero", O_WRONLY,
					     0644));
  errCode (posix_spawn_file_actions_adddup2 (&fa, 1, 2));
  strcpy (buf, "/dev/fd/2");
  childargv[3] = "/dev/zero";
  errCode (posix_spawn (&pid, MYSELF, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_addclose */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addclose (&fa, fd));
  sprintf (buf, "/dev/fd/%d", fd);
  childargv[3] = "<ENOENT>";
  errCode (posix_spawn (&pid, MYSELF, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* TODO: test new fds (open or dup2) not 0 through 2 */
  /* TODO: test error cases */

  negError (close (fd));
  negError (close (fdcloexec));

  return 0;
}
