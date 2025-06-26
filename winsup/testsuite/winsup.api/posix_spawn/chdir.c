#include "test.h"
#include <fcntl.h>
#include <limits.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Linux is behind the times a bit (also needs the *chdir_np functions) */
#ifndef O_SEARCH
#  define O_SEARCH O_PATH
#endif

int handle_child (char *expected)
{
  char buf[PATH_MAX + 1];

  nullError (getcwd (buf, sizeof (buf)));
  testAssertMsg (!strcmp (buf, expected), "cwd '%s' != expected '%s'",
		 buf, expected);

  return 0;
}

int handle_childfds (char *expectedcwd, char *expectedfd0, char *expectedfd1)
{
  char buf[PATH_MAX + 1];
  ssize_t ret;

  testAssert (handle_child (expectedcwd) == 0);

  negError (ret = readlink ("/dev/fd/0", buf, PATH_MAX));
  testAssertMsg (ret < PATH_MAX, "Path too long for PATH_MAX buffer");
  buf[ret] = '\0';
  testAssertMsg (!strcmp (buf, expectedfd0), "fd 0 '%s' != expected '%s'",
		 buf, expectedfd0);

  negError (ret = readlink ("/dev/fd/1", buf, PATH_MAX));
  testAssertMsg (ret < PATH_MAX, "Path too long for PATH_MAX buffer");
  buf[ret] = '\0';
  testAssertMsg (!strcmp (buf, expectedfd1), "fd 1 '%s' != expected '%s'",
		 buf, expectedfd1);

  return 0;
}

static char tmpcwd[] = "tmpcwd.XXXXXX";
static char tmppath[] = "tmpfile.XXXXXX";
static char tmppath2[sizeof (tmpcwd) + 9] = {0};

static void cleanup_tmpfiles (void)
{
  if (tmppath2[0])
    unlink (tmppath2);
  rmdir (tmpcwd);
  unlink (tmppath);
}

int main (int argc, char **argv)
{
  posix_spawn_file_actions_t fa;
  pid_t pid;
  int status;
  int fd;
  char buf[PATH_MAX + 1];
  char buffd0[PATH_MAX + 1];
  char buffd1[PATH_MAX + 1];
  char *childargv[] = {"chdir", "--child", buf, NULL, NULL, NULL};

  /* unbuffer stdout */
  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc == 3 && !strcmp (argv[1], "--child"))
    return handle_child (argv[2]);
  else if (argc == 5 && !strcmp (argv[1], "--child"))
    return handle_childfds (argv[2], argv[3], argv[4]);

  /* make a directory and a couple of files for testing */
  nullError (mkdtemp (tmpcwd));
  atexit (cleanup_tmpfiles);

  negError (fd = mkstemp (tmppath));
  negError (close (fd));

  stpcpy (stpcpy (stpcpy (tmppath2, tmpcwd), "/"), "tmpfile2");
  negError (fd = open (tmppath2, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR));
  negError (close (fd));


  /* ensure cwd is inherited by default */
  nullError (getcwd (buf, sizeof (buf)));
  stpcpy (stpcpy (stpcpy (buffd0, buf), "/"), tmppath);
  errCode (posix_spawn (&pid, MYSELF, NULL, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);

  /* test posix_spawn_file_actions_addchdir */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addchdir_np (&fa, tmpcwd));

  strcat (buf, "/");
  strcat (buf, tmpcwd);
  stpcpy (stpcpy (stpcpy (buffd1, buf), "/"), "tmpfile2");

  errCode (posix_spawn (&pid, MYSELF, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_addfchdir */
  negError (fd = open (tmpcwd, O_SEARCH|O_DIRECTORY|O_CLOEXEC, 0755));
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addfchdir_np (&fa, fd));
  errCode (posix_spawn (&pid, MYSELF, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));
  negError (close (fd));

  /* test posix_spawn_file_actions_addchdir + addopen */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addopen (&fa, 0, tmppath, O_RDONLY, 0644));
  errCode (posix_spawn_file_actions_addchdir_np (&fa, tmpcwd));
  errCode (posix_spawn_file_actions_addopen (&fa, 1, "tmpfile2", O_WRONLY, 0644));
  childargv[3] = buffd0;
  childargv[4] = buffd1;
  errCode (posix_spawn (&pid, MYSELF, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_addfchdir + addopen */
  negError (fd = open (tmpcwd, O_SEARCH|O_DIRECTORY|O_CLOEXEC, 0755));
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addopen (&fa, 0, tmppath, O_RDONLY, 0644));
  errCode (posix_spawn_file_actions_addfchdir_np (&fa, fd));
  errCode (posix_spawn_file_actions_addopen (&fa, 1, "tmpfile2", O_WRONLY, 0644));
  errCode (posix_spawn (&pid, MYSELF, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_addfchdir + adddup2 of directory fd */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_adddup2 (&fa, fd, 0));
  errCode (posix_spawn_file_actions_addfchdir_np (&fa, fd));
  errCode (posix_spawn_file_actions_addopen (&fa, 1, "tmpfile2", O_WRONLY, 0644));
  childargv[3] = buf;
  errCode (posix_spawn (&pid, MYSELF, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  negError (close (fd));

  return 0;
}
