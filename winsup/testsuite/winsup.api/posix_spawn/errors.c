#include "test.h"
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char tmppath[] = "pspawn.XXXXXX";
static const char exit0[] = "exit 0\n";

void cleanup_tmpfile (void)
{
  unlink (tmppath);
}

int main (void)
{
  posix_spawn_file_actions_t fa;
  pid_t pid;
  int fd;
  char *childargv[] = {"ls", NULL};
  char tmpsub[sizeof (tmppath) + 3];
  char *p;

  /* unbuffer stdout */
  setvbuf(stdout, NULL, _IONBF, 0);

  negError (fd = mkstemp (tmppath));
  atexit (cleanup_tmpfile);
  negError (write (fd, exit0, sizeof (exit0) - 1));
  negError (close (fd));

  /* expected ENOENT: posix_spawn without full path */
  errCodeExpected (ENOENT,
      posix_spawn (&pid, childargv[0], NULL, NULL, childargv, environ));

  /* expected EACCES: posix_spawn with path to non-executable file */
  errCodeExpected (EACCES,
      posix_spawn (&pid, tmppath, NULL, NULL, childargv, environ));

  negError (chmod (tmppath, 0755));

  /* expected ENOEXEC: posix_spawn with unrecognized file format */
  errCodeExpected (ENOEXEC,
      posix_spawn (&pid, tmppath, NULL, NULL, childargv, environ));

  p = stpcpy (tmpsub, tmppath);
  p = stpcpy (p, "/ls");

#ifndef __CYGWIN__
  /* Cygwin returns ENOENT rather than ENOTDIR here */
  /* expected ENOTDIR: posix_spawn with file as non-leaf entry in path */
  errCodeExpected (ENOTDIR,
      posix_spawn (&pid, tmpsub, NULL, NULL, childargv, environ));
#endif

  /* expected ENOENT: relative path after chdir */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addchdir_np (&fa, "/tmp"));
  errCodeExpected (ENOENT,
      posix_spawn (&pid, tmppath, &fa, NULL, childargv, environ));
  errCode (posix_spawn_file_actions_destroy (&fa));

  return 0;
}
