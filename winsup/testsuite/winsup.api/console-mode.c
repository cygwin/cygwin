/* Check console-mode restoration with redirected standard handles.

   This file is part of Cygwin.

   This software is a copyrighted work licensed under the terms of the
   Cygwin license.  Please consult the file "CYGWIN_LICENSE" for details. */

#include <assert.h>
#include <fcntl.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>

int
main (int argc, char **argv)
{
  char exe[MAX_PATH], command[MAX_PATH + 32];
  DWORD mode, result;
  HANDLE output;
  int fd;

  if (argc > 1 && strcmp (argv[1], "child") == 0)
    return 0;

  result = GetModuleFileNameA (NULL, exe, sizeof (exe));
  assert (result > 0 && result < sizeof (exe));
  if (argc == 1)
    {
      STARTUPINFOA startup = { .cb = sizeof (startup),
			      .dwFlags = STARTF_USESHOWWINDOW,
			      .wShowWindow = SW_HIDE };
      PROCESS_INFORMATION process;
      snprintf (command, sizeof (command), "\"%s\" worker", exe);
      assert (CreateProcessA (exe, command, NULL, NULL, FALSE,
			     CREATE_NEW_CONSOLE, NULL, NULL,
			     &startup, &process));
      assert (WaitForSingleObject (process.hProcess, INFINITE)
	      == WAIT_OBJECT_0);
      assert (GetExitCodeProcess (process.hProcess, &result));
      assert (CloseHandle (process.hThread));
      assert (CloseHandle (process.hProcess));
      if (result)
	fprintf (stderr, "Console-mode worker failed: %lu\n", result);
      return result != 0;
    }

  output = GetStdHandle (STD_OUTPUT_HANDLE);
  assert (GetConsoleMode (output, &mode));
  assert (mode & ENABLE_PROCESSED_OUTPUT);
  fd = open ("/dev/null", O_RDWR);
  assert (fd >= 0);
  assert (dup2 (fd, STDOUT_FILENO) == STDOUT_FILENO);
  assert (dup2 (fd, STDERR_FILENO) == STDERR_FILENO);
  assert (close (fd) == 0);
  snprintf (command, sizeof (command), "\"%s\" child", exe);
  const char *arguments[] = { "cygrun", "-notimeout", command, NULL };
  const char *mingwtestdir = getenv ("mingwtestdir");
  assert (mingwtestdir);
  char cygrun[strlen (mingwtestdir) + sizeof ("/cygrun")];
  sprintf (cygrun, "%s/cygrun", mingwtestdir);
  /* cygrun starts the child without Cygwin's spawn bookkeeping. */
  for (int i = 0; i < 2; ++i)
    assert (spawnv (_P_WAIT, cygrun, arguments) == 0);
  assert (GetConsoleMode (output, &mode));
  return !(mode & ENABLE_PROCESSED_OUTPUT);
}
