#include "waitall.h"

#define BUF_SIZE 100

int
main ()
{
    int sfd;
    ssize_t nread;
    char buf[BUF_SIZE];

    if ((sfd = unixConnect (SV_SOCK_PATH, SOCK_STREAM)) < 0)
      errExit ("unixConnect");

    /* Copy stdin to socket. */
    while ((nread = read (STDIN_FILENO, buf, BUF_SIZE)) > 0)
      if (write (sfd, buf, nread) != nread)
	errExit ("partial/failed write");

    if (nread < 0)
      errExit ("read");
}
