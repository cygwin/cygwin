/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* is_seqnum_v3_sv.c (KB)

   A simple Internet stream socket server. Our service is to provide unique
   sequence numbers to the client.

   This program is the same as is_seqnum_v2.c, except that it forks a
   subprocess to do the work, and it sends the connection fd to the
   child over an AF_UNIX socket.  Invoke it with --debug to allow time
   to attach gdb to the child.  (KB)

   Usage:  is_seqnum_sv [init-seq-num]  (default = 0)

   See also is_seqnum_v2_cl.c.
*/
#include "af_unix_hdr.h"
#include "is_seqnum_v2.h"

int
main(int argc, char *argv[])
{
  uint32_t seqNum;
  char reqLenStr[INT_LEN];            /* Length of requested sequence */
  char seqNumStr[INT_LEN];            /* Start of granted sequence */
  struct sockaddr *claddr;
  int lfd, reqLen;
  socklen_t addrlen, alen;
  char addrStr[IS_ADDR_STR_LEN];
  Boolean debug = FALSE;
  pid_t pid;
  int pipefd[2];
  int pfd;			/* parent's end */
  int cfd;			/* child's end */

  if (argc > 1 && strcmp (argv[1], "--help") == 0)
    usageErr ("%s [--debug] [init-seq-num]\n", argv[0]);
  if (argc > 1 && strcmp (argv[1], "--debug") == 0)
    debug = TRUE;

  if (!debug)
    seqNum = (argc > 1) ? getInt (argv[1], 0, "init-seq-num") : 0;
  else
    seqNum = (argc > 2) ? getInt (argv[2], 0, "init-seq-num") : 0;

  /* Ignore the SIGPIPE signal, so that we find out about broken connection
     errors via a failure from write(). */

  if (signal (SIGPIPE, SIG_IGN) == SIG_ERR)
    errExit("signal");

  lfd = inetListen (PORT_NUM_STR, 5, &addrlen);
  if (lfd == -1)
    fatal ("inetListen() failed");

  /* Allocate a buffer large enough to hold the client's socket address */

  claddr = malloc (addrlen);
  if (claddr == NULL)
    errExit ("malloc");

  /* Fork a child to handle client request. */

  if (socketpair (AF_UNIX, SOCK_STREAM, 0, pipefd) < 0)
    errExit ("socketpair");
  pfd = pipefd[1];
  cfd = pipefd[0];

  if ((pid = fork ()) < 0)
    errExit ("fork");
  else if (pid > 0)		/* parent */
    {
      int connfd, junk;

      if (close (cfd) < 0)
	errExit ("close");
      if (debug)
	{
	  printf ("parent pid %d, child pid %d, sleeping...\n", getpid (), pid);
	  sleep (30);
	}

      /* Accept a client connection, obtaining client's address */

      alen = addrlen;
      connfd = accept (lfd, (struct sockaddr *) claddr, &alen);
      if (connfd == -1)
	errExit ("accept");
      printf ("Connection from %s\n", inetAddressStr (claddr, alen, addrStr,
						      IS_ADDR_STR_LEN));
      printf ("Sending fd %d to child\n", connfd);
      if (sendfd (pfd, connfd) < 0)
	errExit ("sendfd");
      if (close (connfd) < 0)
	errExit ("close");
      /* Wait for child. */
      if (read (pfd, &junk, sizeof junk) != sizeof junk)
	errMsg ("read");
      if (close (pfd) < 0)
	errMsg ("close");
      if (close (lfd) < 0)
	errExit ("close");
    }
    else			/* child */
      {
	int connfd, junk;

	if (close (pfd) < 0)
	  errExit ("close");
	if (close (lfd) < 0)
	  errExit ("close");
	if (debug)
	  sleep (30);

	/* Get connection fd from parent. */

	connfd = recvfd (cfd);
	if (connfd < 0)
	  errExit ("recvfd");

        /* Read client request, send sequence number back */

        if (readLine (connfd, reqLenStr, INT_LEN) <= 0)
	  {
            close (connfd);
	    errExit ("readLine");
	  }

        reqLen = atoi (reqLenStr);
        if (reqLen <= 0)
	  {
            close(cfd);
	    errExit ("Bad request");
	  }

        snprintf (seqNumStr, INT_LEN, "%d\n", seqNum);
        if (write (connfd, seqNumStr, strlen (seqNumStr)) != strlen(seqNumStr))
	  errExit ("write");

        seqNum += reqLen;               /* Update sequence number */

        if (close (connfd) == -1)           /* Close connection */
	  errMsg ("close");
	/* Tell parent we're done. */
	if (write (cfd, &junk, sizeof junk) != sizeof junk)
	  errMsg ("write");
	if (close (cfd) < 0)
	  errExit ("close");
      }
}
