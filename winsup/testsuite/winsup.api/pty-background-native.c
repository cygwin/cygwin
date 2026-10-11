#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <errno.h>
#include <poll.h>
#include <pty.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static const struct
{
  const char *query, *reply;
} replies[] = {
  { "\033[6n", "\033[1;1R" }
};

/* Emulate MinTTY answering terminal queries, not rendering a screen. */
static int
terminal (int master, pid_t child)
{
  char window[256];
  size_t used = 0;
  int status;
  ULONGLONG deadline = GetTickCount64 () + 10000;
  for (;;)
    {
      assert (GetTickCount64 () < deadline);
      struct pollfd fd = { master, POLLIN, 0 };
      int ready = poll (&fd, 1, 100);
      assert (ready >= 0 || errno == EINTR);
      if (ready > 0)
	{
	  char buf[128];
	  ssize_t n = read (master, buf, sizeof buf);
	  assert (n >= 0 || errno == EINTR || errno == EIO);
	  for (ssize_t i = 0; i < n; i++)
	    {
	      if (used == sizeof window)
		memmove (window, window + 1, --used);
	      window[used++] = buf[i];
	      for (size_t j = 0; j < sizeof replies / sizeof *replies; j++)
		{
		  size_t len = strlen (replies[j].query);
		  assert (len && len <= sizeof window);
		  if (used >= len
		      && !memcmp (window + used - len, replies[j].query, len))
		    assert (write (master, replies[j].reply,
				  strlen (replies[j].reply))
			   == (ssize_t) strlen (replies[j].reply));
		}
	    }
	}
      pid_t done = waitpid (child, &status, WNOHANG);
      assert (done >= 0 || errno == EINTR);
      if (done == child)
	break;
    }
  close (master);
  if (!WIFEXITED (status) || WEXITSTATUS (status))
    {
      fwrite (window, 1, used, stderr);
      return 1;
    }
  return 0;
}

/* Emulate Vim entering raw mode and querying before its first input read. */
static int
worker (pid_t native)
{
  struct termios raw;
  assert (tcgetpgrp (0) == getpgrp () && getpgid (native) == native
	 && native != getpgrp ());
  assert (!tcgetattr (0, &raw));
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  assert (!tcsetattr (0, TCSANOW, &raw));
  assert (write (1, replies[0].query, strlen (replies[0].query))
	 == (ssize_t) strlen (replies[0].query));
  /* Emulate Vim's startup work while replies arrive before the first read. */
  usleep (100000);
  char buf[64];
  size_t used = 0;
  for (int i = 0; i < 30 && used < sizeof buf; i++)
    {
      ssize_t n = read (0, buf + used, sizeof buf - used);
      assert (n >= 0 || errno == EINTR || errno == EAGAIN);
      if (n > 0)
	used += n;
      if (memchr (buf, 'R', used))
	break;
    }
  int result = used != strlen (replies[0].reply)
	       || memcmp (buf, replies[0].reply, used);
  /* Misrouted reports can become keystrokes that edit Vim's first line. */
  if (result)
    fprintf (stderr, "Bad terminal reply (%zu bytes)\n", used);
  int status;
  assert (waitpid (native, &status, 0) == native
	 && WIFEXITED (status) && !WEXITSTATUS (status));
  return result ? 1 : 0;
}

int
main (int argc, char **argv)
{
  if (argc == 3 && !strcmp (argv[1], "worker"))
    {
      int pid;
      char extra;
      assert (sscanf (argv[2], "%d%c", &pid, &extra) == 1 && pid > 0);
      return worker (pid);
    }
  assert (argc == 1);
  if (!GetProcAddress (GetModuleHandleW (L"kernel32.dll"),
		      "CreatePseudoConsole"))
    {
      fprintf (stderr, "ConPTY unavailable\n");
      return 77;
    }
  assert (!setenv ("TERM", "xterm", 1));
  struct winsize size = { 24, 80, 0, 0 };
  int master;
  pid_t child = forkpty (&master, NULL, NULL, &size);
  assert (child >= 0);
  if (child)
    return terminal (master, child);

  int ready[2];
  assert (!pipe (ready));
  pid_t native = fork ();
  assert (native >= 0);
  if (!native)
    {
      close (ready[0]);
      assert (dup2 (ready[1], 1) == 1);
      close (ready[1]);
      assert (execl ("/bin/pty-native-input.exe", "pty-native-input",
		    (char *) NULL) != -1);
    }
  close (ready[1]);
  char byte;
  assert (read (ready[0], &byte, 1) == 1 && byte == 'R');
  close (ready[0]);
  /* Emulate a native job remaining alive while foreground Vim starts. */
  assert (!setpgid (native, native));
  char pid[24];
  snprintf (pid, sizeof pid, "%ld", (long) native);
  assert (execl (argv[0], argv[0], "worker", pid, (char *) NULL) != -1);
}
