#include "select_test.h"

int
main ()
{
  int sfd, cfd, flags;
  fd_set readfds;
  size_t nread = 0;
  char buf[BUF_SIZE];

  if (remove (SV_SOCK_PATH) < 0 && errno != ENOENT)
    errExit ("remove");

  if ((sfd = unixBind (SV_SOCK_PATH, SOCK_STREAM)) < 0)
    errExit ("unixBind");

  if (listen (sfd, BACKLOG) < 0)
    errExit ("listen");

  printf ("waiting for connection request...\n");
  FD_ZERO (&readfds);
  FD_SET (sfd, &readfds);
  if (select (sfd + 1, &readfds, NULL, NULL, NULL) < 0)
    errExit ("select");
  if (FD_ISSET (sfd, &readfds))
    printf ("connection request received; accepting\n");
  else
    errExit ("something's wrong");
  cfd = accept (sfd, NULL, NULL);
  if (cfd < 0)
    errExit ("accept");

  flags = fcntl (cfd, F_GETFL);
  if (fcntl (cfd, F_SETFL, flags | O_NONBLOCK) < 0)
    errExit ("fcntl");

  printf ("slowly reading from socket...\n");
  while (1)
    {
      FD_ZERO (&readfds);
      FD_SET (cfd, &readfds);
      if (select (cfd + 1, &readfds, NULL, NULL, NULL) < 0)
	errExit ("select");
      if (!FD_ISSET (cfd, &readfds))
	errExit ("something's wrong");
      ssize_t nr = read (cfd, buf, 10);
      if (nr < 0)
	{
	  if (errno == EPIPE)
	    break;
	  else
	    errExit ("read");
	}
      else if (nr == 0)
	break;
      nread += nr;
    }
  printf ("read %zu bytes\n", nread);
}
