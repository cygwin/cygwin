#include "test.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cygwin.h>
#include <unistd.h>

char * find_winchild (void)
{
  static const char winchild[] = "/winchild";
  char *mingwtestdir = getenv ("mingwtestdir");
  if (!mingwtestdir)
    {
      Dl_info dli;
      if (dladdr (&find_winchild, &dli))
        {
	  ssize_t i = strlen (dli.dli_fname) - 1;
	  for (int slashes = 0; i >= 0 && slashes < 3; --i)
	    if (dli.dli_fname[i] == '/')
	      slashes++;
	  stpcpy (stpcpy (dli.dli_fname + i + 1, "/mingw"), winchild);
	  return realpath (dli.dli_fname, NULL);
	}
      else
        {
	  return realpath ("../../mingw/winchild", NULL);
	}
    }
  else
    {
      char *ret, *tmp = malloc (strlen (mingwtestdir) + sizeof (winchild));
      stpcpy (stpcpy (tmp, mingwtestdir), winchild);
      ret = realpath (tmp, NULL);
      free (tmp);
      return ret;
    }
}

static char tmppath[] = "pspawn.XXXXXX";
static char tmpcwd[] = "tmpcwd.XXXXXX";
static char tmppath2[sizeof (tmpcwd) + 9] = {0};

static void cleanup_tmpfiles (void)
{
  if (tmppath2[0])
    unlink (tmppath2);
  rmdir (tmpcwd);
  unlink (tmppath);
}

int main (void)
{
  posix_spawn_file_actions_t fa;
  pid_t pid;
  int status;
  int fd, fdcloexec, cwdfd;
  char *childargv[] = {"winchild", NULL, NULL, NULL};
  char *winchild = find_winchild ();

  /* unbuffer stdout */
  setvbuf(stdout, NULL, _IONBF, 0);

  /* temp regular file */
  negError (fd = mkstemp (tmppath));
  atexit (cleanup_tmpfiles);
  negError (close (fd));

  /* temp directory */
  nullError (mkdtemp (tmpcwd));

  /* temp file within temp directory */
  stpcpy (stpcpy (stpcpy (tmppath2, tmpcwd), "/"), "tmpfile2");
  negError (fd = open (tmppath2, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR));
  negError (close (fd));

  /* open file descriptors to test inheritance */
  negError (fd = open ("/dev/null", O_RDONLY, 0644));
  negError (fdcloexec = open ("/dev/full", O_RDONLY|O_CLOEXEC, 0644));

  /* test posix_spawn_file_actions_addopen */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addopen (&fa, 0, "/dev/zero", O_RDONLY,
					     0644));
  childargv[1] = "0";
  childargv[2] = "\\Device\\Null";
  errCode (posix_spawn (&pid, winchild, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_adddup2 */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_adddup2 (&fa, fd, 0));
  errCode (posix_spawn (&pid, winchild, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_adddup2 with CLOEXEC fd */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_adddup2 (&fa, fdcloexec, 0));
  errCode (posix_spawn (&pid, winchild, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_adddup2 with out to err */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addopen (&fa, 1, "/dev/zero", O_WRONLY,
					     0644));
  errCode (posix_spawn_file_actions_adddup2 (&fa, 1, 2));
  childargv[1] = "2";
  errCode (posix_spawn (&pid, winchild, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_addopen with real file */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addopen (&fa, 1, tmppath, O_WRONLY, 0644));
  childargv[1] = "1";
  childargv[2] = cygwin_create_path (CCP_POSIX_TO_WIN_A|CCP_ABSOLUTE, tmppath);
  errCode (posix_spawn (&pid, winchild, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));
  free (childargv[2]);

  /* test posix_spawn_file_actions_addchdir */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addchdir (&fa, tmpcwd));
  childargv[1] = "CWD";
  childargv[2] = cygwin_create_path (CCP_POSIX_TO_WIN_A|CCP_ABSOLUTE, tmpcwd);
  errCode (posix_spawn (&pid, winchild, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));

  /* test posix_spawn_file_actions_addfchdir */
  negError (cwdfd = open (tmpcwd, O_SEARCH|O_DIRECTORY|O_CLOEXEC, 0755));
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addfchdir (&fa, cwdfd));
  errCode (posix_spawn (&pid, winchild, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));
  free (childargv[2]);

  /* test posix_spawn_file_actions_addfchdir followed by addopen */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_addfchdir (&fa, cwdfd));
  errCode (posix_spawn_file_actions_addopen (&fa, 1, "tmpfile2", O_WRONLY, 0644));
  childargv[1] = "1";
  childargv[2] = cygwin_create_path (CCP_POSIX_TO_WIN_A|CCP_ABSOLUTE, tmppath2);
  errCode (posix_spawn (&pid, winchild, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));
  free (childargv[2]);

  /* test posix_spawn_file_actions_adddup2 of directory handle */
  errCode (posix_spawn_file_actions_init (&fa));
  errCode (posix_spawn_file_actions_adddup2 (&fa, cwdfd, 0));
  childargv[1] = "0";
  childargv[2] = cygwin_create_path (CCP_POSIX_TO_WIN_A|CCP_ABSOLUTE, tmpcwd);
  errCode (posix_spawn (&pid, winchild, &fa, NULL, childargv, environ));
  negError (waitpid (pid, &status, 0));
  exitStatus (status, 0);
  errCode (posix_spawn_file_actions_destroy (&fa));
  free (childargv[2]);

  negError (close (cwdfd));
  negError (close (fd));
  negError (close (fdcloexec));

  return 0;
}
