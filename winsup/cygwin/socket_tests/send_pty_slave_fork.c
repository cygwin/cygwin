/*
  Fork a subprocess, open a pty pair, and send the pty slave file
  descriptor to the subprocess over an AF_UNIX socket.  Invoke with
  --debug to allow time to attach gdb to the child.
*/

#include "af_unix_hdr.h"
#include "pty_master_open.h"
#include "read_line.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define BUF_SIZE 100
#define MAX_SNAME 100                  /* Maximum size for pty slave name */

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
      int mfd, sfd, junk;
      char slname[MAX_SNAME];

      if (close (cfd) < 0)
	errExit ("close");
      if ((mfd = ptyMasterOpen (slname, MAX_SNAME)) < 0)
	errExit ("ptyMasterOpen");
      if ((sfd = open (slname, O_RDWR | O_NOCTTY)) < 0)
	errExit ("open");
      if (debug)
	{
	  printf ("parent pid %d, child pid %d, sleeping...\n", getpid (), pid);
	  sleep (30);
	}

      printf ("parent sending descriptor %d for %s to child\n", sfd, slname);
      if (sendfd (pfd, sfd) < 0)
	errExit ("sendfd");
      if (close (sfd) < 0)
	errMsg ("close");
      if (write (mfd, "hello\n", 6) < 0)
	errExit ("write");
      /* Wait for child. */
      if (read (pfd, &junk, sizeof junk) != sizeof junk)
	errMsg ("read");
      if (close (pfd) < 0)
	errMsg ("close");
      if (close (mfd) < 0)
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
      printf ("child read %zd bytes (including newline) from fd %d: %s\n", nr,
	      fd, buf);
      if (close (fd) == -1)
	errMsg ("close");
      /* Tell parent we're done. */
      if (write (cfd, &junk, sizeof junk) != sizeof junk)
	errMsg ("write");
      if (close (cfd) < 0)
	errExit ("close");
    }
}
