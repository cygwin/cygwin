#include "waitall.h"

#define BUF_SIZE 10

int
main ()
{
    int sfd, cfd;
    ssize_t nread;
    char buf[BUF_SIZE];

    if (remove (SV_SOCK_PATH) < 0 && errno != ENOENT)
      errExit ("remove");

    if ((sfd = unixBind (SV_SOCK_PATH, SOCK_STREAM)) < 0)
      errExit ("unixBind");

    if (listen (sfd, BACKLOG) < 0)
      errExit ("listen");

    while (1)
      {
        cfd = accept (sfd, NULL, NULL);
        if (cfd < 0)
	  errExit ("accept");

        /* Transfer data from connected socket to stdout until EOF. */
        while ((nread = recv (cfd, buf, BUF_SIZE, MSG_WAITALL)) > 0)
	  if (write (STDOUT_FILENO, buf, nread) != nread)
	    errExit ("partial/failed write");

        if (nread < 0)
	  errExit ("read");

        if (close (cfd) < 0)
	  errExit ("close");
      }
}
