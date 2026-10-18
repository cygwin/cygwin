#include <stdio.h>
#include <windows.h>

static int
typeahead (char *command)
{
  HANDLE input = GetStdHandle (STD_INPUT_HANDLE);
  const char ready[] = "TYPEAHEAD_READY";
  DWORD n;
  if (GetFileType (input) != FILE_TYPE_PIPE
      || !WriteFile (GetStdHandle (STD_OUTPUT_HANDLE),
		     ready, sizeof ready - 1, &n, NULL)
      || n != sizeof ready - 1)
    {
      fprintf (stderr, "Typeahead pipe readiness failed: %lu\n",
	       GetLastError ());
      return 1;
    }
  /* Emulate cmd reading two queued commands and launching ps for each. */
  for (int i = 0; i < 2; i++)
    {
      DWORD available;
      ULONGLONG deadline = GetTickCount64 () + 3000;
      do
	{
	  if (!PeekNamedPipe (input, NULL, 0, NULL, &available, NULL))
	    {
	      fprintf (stderr, "Cannot inspect queued input: %lu\n",
		       GetLastError ());
	      return 1;
	    }
	  if (GetTickCount64 () >= deadline)
	    {
	      fprintf (stderr, "Queued command %d did not reach native input\n",
		       i + 1);
	      return 1;
	    }
	  if (available < (i ? 2 : 4))
	    Sleep (1);
	}
      while (available < (i ? 2 : 4));
      char line[2];
      if (!ReadFile (input, line, sizeof line, &n, NULL)
	  || n != sizeof line || line[0] != '1' + i || line[1] != '\n')
	{
	  fprintf (stderr, "Cannot read queued command %d: %lu\n",
		   i + 1, GetLastError ());
	  return 1;
	}
      STARTUPINFOA si = { 0 };
      PROCESS_INFORMATION pi;
      si.cb = sizeof si;
      si.dwFlags = STARTF_USESTDHANDLES;
      si.hStdInput = input;
      si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
      si.hStdError = GetStdHandle (STD_ERROR_HANDLE);
      char child_command[MAX_PATH + 32];
      int len = snprintf (child_command, sizeof child_command, "%s", command);
      if (len < 0 || (size_t) len >= sizeof child_command)
	{
	  fprintf (stderr, "Typeahead child command is too long\n");
	  return 1;
	}
      if (!CreateProcessA (NULL, child_command, NULL, NULL, TRUE, 0,
			   NULL, NULL, &si, &pi))
	{
	  fprintf (stderr, "Cannot launch typeahead child: %lu\n",
		   GetLastError ());
	  return 1;
	}
      DWORD status;
      if (WaitForSingleObject (pi.hProcess, 3000) != WAIT_OBJECT_0
	  || !GetExitCodeProcess (pi.hProcess, &status) || status)
	{
	  fprintf (stderr, "Typeahead child did not finish successfully\n");
	  return 1;
	}
      CloseHandle (pi.hProcess);
      CloseHandle (pi.hThread);
    }
  return 0;
}

int
main (int argc, char **argv)
{
  if (argc == 2)
    return typeahead (argv[1]);
  SECURITY_ATTRIBUTES sa = { sizeof sa, NULL, TRUE };
  HANDLE input = CreateFileA ("CONIN$", GENERIC_READ,
			     FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
			     OPEN_EXISTING, 0, NULL);
  DWORD mode, written;
  if (input == INVALID_HANDLE_VALUE || !GetConsoleMode (input, &mode))
    {
      fprintf (stderr, "Native console readiness failed: %lu\n",
	       GetLastError ());
      return 1;
    }
  if (argc == 3)
    {
      /* Emulate native Git launching a Cygwin editor. */
      STARTUPINFOA si = { 0 };
      PROCESS_INFORMATION pi;
      HANDLE output, child_pipe;
      si.cb = sizeof si;
      si.dwFlags = STARTF_USESTDHANDLES;
      if (sscanf (argv[2], "%p,%p", &si.hStdInput, &output) != 2)
	{
	  fprintf (stderr, "Invalid inherited terminal handles\n");
	  return 1;
	}
      /* Only stdin is a PTY; stdout must not mask a missed input handoff. */
      if (!CreatePipe (&child_pipe, &si.hStdOutput, &sa, 0)
	  || !SetHandleInformation (child_pipe, HANDLE_FLAG_INHERIT, 0))
	{
	  fprintf (stderr, "Cannot create child output pipe: %lu\n",
		   GetLastError ());
	  return 1;
	}
      si.hStdError = si.hStdOutput;
      if (!CreateProcessA (NULL, argv[1], NULL, NULL, TRUE, 0,
			   NULL, NULL, &si, &pi))
	{
	  fprintf (stderr, "Cannot launch Cygwin editor: %lu\n",
		   GetLastError ());
	  return 1;
	}
      CloseHandle (si.hStdOutput);
      /* Forward Vim's queries to MinTTY without another Cygwin PTY fd. */
      char buf[128];
      DWORD n;
      while (ReadFile (child_pipe, buf, sizeof buf, &n, NULL))
	if (!WriteFile (output, buf, n, &written, NULL) || written != n)
	  {
	    fprintf (stderr, "Cannot forward child output: %lu\n",
		     GetLastError ());
	    return 1;
	  }
      if (GetLastError () != ERROR_BROKEN_PIPE)
	{
	  fprintf (stderr, "Cannot read child output: %lu\n",
		   GetLastError ());
	  return 1;
	}
      CloseHandle (child_pipe);
      DWORD status;
      if (WaitForSingleObject (pi.hProcess, 8000) != WAIT_OBJECT_0
	  || !GetExitCodeProcess (pi.hProcess, &status))
	{
	  fprintf (stderr, "Cygwin editor did not finish\n");
	  return 1;
	}
      if (status)
	fprintf (stderr, "Cygwin editor exit status: %lu\n", status);
      CloseHandle (pi.hProcess);
      CloseHandle (pi.hThread);
      return status;
    }
  if (argc != 1
      || !WriteFile (GetStdHandle (STD_OUTPUT_HANDLE), "R", 1, &written, NULL)
      || written != 1)
    {
      fprintf (stderr, "Native console readiness failed: %lu\n",
	       GetLastError ());
      return 1;
    }
  /* Keep native console input active during the foreground Vim startup. */
  Sleep (4000);
  return 0;
}
