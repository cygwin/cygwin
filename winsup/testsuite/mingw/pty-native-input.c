#include <stdio.h>
#include <windows.h>

int
main (void)
{
  HANDLE input = CreateFileA ("CONIN$", GENERIC_READ,
			     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			     OPEN_EXISTING, 0, NULL);
  DWORD mode, written;
  if (input == INVALID_HANDLE_VALUE || !GetConsoleMode (input, &mode)
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
