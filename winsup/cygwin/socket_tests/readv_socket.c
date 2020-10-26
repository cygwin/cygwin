/* Adapted from https://www.oreilly.com/library/view/linux-system-programming/0596009585/ch04.html */

#include "scatter_gather.h"

int main ()
{
  char foo[48], bar[51], baz[49];
  struct iovec iov[3];
  ssize_t nr;
  int sfd, cfd;

  if (remove (SV_SOCK_PATH) < 0 && errno != ENOENT)
    errExit ("remove");

  if ((sfd = unixBind (SV_SOCK_PATH, SOCK_STREAM)) < 0)
    errExit ("unixBind");

  if (listen (sfd, BACKLOG) < 0)
    errExit ("listen");

  cfd = accept (sfd, NULL, NULL);
  if (cfd < 0)
    errExit ("accept");

  iov[0].iov_base = foo;
  iov[0].iov_len = sizeof (foo);
  iov[1].iov_base = bar;
  iov[1].iov_len = sizeof (bar);
  iov[2].iov_base = baz;
  iov[2].iov_len = sizeof (baz);

  nr = readv (cfd, iov, 3);
  if (nr < 0)
    errExit ("readv");
  printf ("read %zd bytes\n", nr);
  for (int i = 0; i < 3; i++)
    printf ("%d: %s", i, (char *) iov[i].iov_base);

  if (close (cfd) < 0)
    errExit ("close");
}
