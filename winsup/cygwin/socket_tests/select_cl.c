#include "select_test.h"

int
main ()
{
  int sfd, flags;
  fd_set writefds;
  size_t nwritten = 0;
  ssize_t nw;
  char buf[BUF_SIZE];

  if ((sfd = unixConnect (SV_SOCK_PATH, SOCK_STREAM)) < 0)
    errExit ("unixConnect");
  flags = fcntl (sfd, F_GETFL);
  if (fcntl (sfd, F_SETFL, flags | O_NONBLOCK) < 0)
    errExit ("fcntl");

  printf ("waiting for socket to be ready for write...\n");
  FD_ZERO (&writefds);
  FD_SET (sfd, &writefds);
  if (select (sfd + 1, NULL, &writefds, NULL, NULL) < 0)
    errExit ("select");
  if (FD_ISSET (sfd, &writefds))
    printf ("ready for write, writing until buffer full\n");
  else
    errExit ("something's wrong");
  while (1)
    {
      nw = write (sfd, buf, BUF_SIZE);
      if (nw < 0)
	{
	  if (errno == EAGAIN)
	    {
	      printf ("buffer full\n");
	      break;
	    }
	  else
	    errExit ("write");
	}
      nwritten += nw;
    }
  printf ("wrote %zu bytes\n", nwritten);
  printf ("waiting for write ready again...\n");
  FD_ZERO (&writefds);
  FD_SET (sfd, &writefds);
  if (select (sfd + 1, NULL, &writefds, NULL, NULL) < 0)
    errExit ("select");
  if (FD_ISSET (sfd, &writefds))
    printf ("ready for write, writing once more\n");
  if ((nw = write (sfd, buf, BUF_SIZE)) < 0)
    errExit ("write");
  nwritten += nw;
  printf ("wrote %zd more bytes for a total of %zu\n", nw, nwritten);
}
