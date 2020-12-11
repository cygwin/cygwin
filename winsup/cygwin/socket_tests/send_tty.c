/*
  Fork a subprocess and send it the stdin file descriptor over an
  AF_UNIX socket.  Invoke with --debug to allow time to attach gdb to
  the child.

  Usage: ./send_tty, then enter a line at the terminal.
*/

#include "af_unix_hdr.h"
#include "read_line.h"          /* Declaration of readLine() */
#define BUF_SIZE 100

int
main(int argc, char *argv[])
{
  Boolean debug = FALSE;
  pid_t pid;
  int pipefd[2];
  int pfd;			/* parent's end */
  int cfd;			/* child's end */

  if (argc > 1 && strcmp (argv[1], "--debug") == 0)
    debug = TRUE;

  if (socketpair (AF_UNIX, SOCK_STREAM, 0, pipefd) < 0)
    errExit ("socketpair");
  pfd = pipefd[1];
  cfd = pipefd[0];

  if ((pid = fork ()) < 0)
    errExit ("fork");
  else if (pid > 0)		/* parent */
    {
      int junk;

      if (close (cfd) < 0)
	errExit ("close");
      if (debug)
	{
	  printf ("parent pid %d, child pid %d, sleeping...\n", getpid (), pid);
	  sleep (30);
	}

      /* Send stdin descriptor to child. */
      if (sendfd (pfd, STDIN_FILENO) < 0)
	errExit ("sendfd");
      /* Wait for child. */
      if (read (pfd, &junk, sizeof junk) != sizeof junk)
	errMsg ("read");
      if (close (pfd) < 0)
	errMsg ("close");
    }
    else			/* child */
      {
	int fd, junk;
	ssize_t nr;
	char buf[BUF_SIZE];

	if (close (pfd) < 0)
	  errExit ("close");
	if (debug)
	  sleep (30);

	/* Read fd from parent. */
	fd = recvfd (cfd);
	if (fd < 0)
	  errExit ("recvfd");

        /* Read a line from fd. */
        if ((nr = readLine (fd, buf, BUF_SIZE)) < 0)
	  {
            close (fd);
	    errExit ("readLine");
	  }

	/* Kill newline. */
	  buf[nr - 1] = '\0';
	printf ("Read %zd bytes from fd %d (including newline): %s\n", nr,
		fd, buf);
        if (close (fd) == -1)           /* Close connection */
	  errMsg ("close");
	/* Tell parent we're done. */
	if (write (cfd, &junk, sizeof junk) != sizeof junk)
	  errMsg ("write");
	if (close (cfd) < 0)
	  errExit ("close");
      }
}
