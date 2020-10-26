/* Adapted from https://www.oreilly.com/library/view/linux-system-programming/0596009585/ch04.html */

#include "scatter_gather.h"

int main ( )
{
  struct iovec iov[3];
  ssize_t nr;
  int sfd;

  char *buf[] = {
    "The term buccaneer comes from the word boucan.\n",
    "A boucan is a wooden frame used for cooking meat.\n",
    "Buccaneer is the West Indies name for a pirate.\n"
  };

  if ((sfd = unixConnect (SV_SOCK_PATH, SOCK_STREAM)) < 0)
    errExit ("unixConnect");

  for (int i = 0; i < 3; i++)
    {
      iov[i].iov_base = buf[i];
      iov[i].iov_len = strlen (buf[i]) + 1;
    }

  nr = writev (sfd, iov, 3);
  if (nr < 0)
    errExit ("writev");
  printf ("wrote %zd bytes\n", nr);
  if (close (sfd) < 0)
    errExit ("close");
}
