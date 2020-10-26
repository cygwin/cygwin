#include "msg_peek.h"

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

    cfd = accept (sfd, NULL, NULL);
    if (cfd < 0)
      errExit ("accept");

    printf ("peeking...\n");
    if ((nread = recv (cfd, buf, BUF_SIZE - 1, MSG_PEEK)) < 0)
      errExit ("recv");
    buf[nread] = '\0';
    printf ("reading would yield %zd bytes: %s\n", nread, buf);
    sleep (1);
    printf ("reading...\n");
    if ((nread = read (cfd, buf, BUF_SIZE)) < 0)
      errExit ("read");
    buf[nread] = '\0';
    printf ("read %zd bytes: %s\n", nread, buf);
    if (close (cfd) < 0)
      errExit ("close");
}
