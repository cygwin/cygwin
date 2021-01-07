#include "af_unix_hdr.h"
#include "pty_master.h"
#define BUF_SIZE 100

int
main (int argc, char *argv[])
{
  int lfd, connfd, ptyfd, junk;

  if (remove (SOCK_PATH) == -1 && errno != ENOENT)
    errExit ("remove-%s", SOCK_PATH);

  lfd = unixBind (SOCK_PATH, SOCK_STREAM);
  if (lfd < 0)
    errExit ("unixBind");
  printf ("Waiting for sender to connect and send descriptor...\n");
  if (listen (lfd, 5) < 0)
    errExit ("listen");
  connfd = accept (lfd, NULL, NULL);
  if (connfd < 0)
    errExit ("accept");
  ptyfd = recvfd (connfd);
  if (ptyfd < 0)
    errExit ("recvfd");
  printf ("Received descriptor %d.\n", ptyfd);
  printf ("Writing \"ps\" to that descriptor.\n"
	  "This should appear in the other terminal\n"
	  "and be executed by the shell running there.\n");
  if (write (ptyfd, "ps\n", 3) != 3)
    errExit ("write");
  printf ("Waiting for sender to finish...\n");
  if (read (connfd, &junk, sizeof junk) < 0)
    errExit ("read");
  if (close (ptyfd) < 0)
    errMsg ("close");
  if (close (lfd) < 0)
    errMsg ("close");
  if (close (connfd) < 0)
    errMsg ("close");
}
