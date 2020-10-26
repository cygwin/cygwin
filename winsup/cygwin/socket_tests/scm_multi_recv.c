/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Supplementary program for Chapter 61 */

/* scm_multi_recv.c

   Used in conjunction with scm_multi_send.c to demonstrate passing of
   ancillary data containing multiple 'msghdr' structures on a UNIX
   domain socket.

   Usage is as shown in the usageErr() call below.

   This program uses stream sockets by default; the "-d" command-line option
   specifies that datagram sockets should be used instead.

   This program is Linux-specific.
*/
#define _GNU_SOURCE
#include "scm_multi.h"

#define BUF_SIZE 100

int
main(int argc, char *argv[])
{
    int data, lfd, sfd, opt, optval, j;
    ssize_t NumReceived;
    Boolean useDatagramSocket;
    int optControlMsgSize;
    struct msghdr msgh;
    struct iovec iov;
    char *controlMsg;           /* Ancillary data (control message) */
    size_t controlMsgSize;      /* Size of ancillary data */
    struct cmsghdr *cmsgp;      /* Pointer used to iterate through
                                   headers in ancillary data */
    struct ucred *ucredp;       /* Pointer to data area of a 'cmsghdr' that
                                   contains credentials */
    int *fdList;                /* Pointer to data area of a 'cmsghdr' that
                                   contains a list of file descriptors */
    int fdCnt;                  /* Number of FDs in ancillary data */

    /* Allocate a buffer of suitable size to hold the ancillary data.
       This buffer is in reality treated as a 'struct cmsghdr',
       and so needs to be suitably aligned: malloc() provides a block
       with suitable alignment. */

    controlMsgSize = CMSG_SPACE(sizeof(int[MAX_FDS])) +
                     CMSG_SPACE(sizeof(struct ucred));
    controlMsg = malloc(controlMsgSize);
    if (controlMsg == NULL)
        errExit("malloc");

    /* Parse command-line options */

    useDatagramSocket = FALSE;
    optControlMsgSize = -1;

    while ((opt = getopt(argc, argv, "dn:")) != -1) {
        switch (opt) {
        case 'd':
            useDatagramSocket = TRUE;
            break;

        case 'n':
            optControlMsgSize = atoi(optarg);
            break;

        default:
            usageErr("%s [-d]\n"
                     "        -d           use datagram socket\n"
                     "        -n nbytes    limit on size of received "
                                          "ancillary data\n", argv[0]);
        }
    }

    /* Create socket bound to a well-known address. In the case where
       we are using stream sockets, also make the socket a listening
       socket and accept a connection on the socket. */

    if (remove(SOCK_PATH) == -1 && errno != ENOENT)
        errExit("remove-%s", SOCK_PATH);

    if (useDatagramSocket) {
        sfd = unixBind(SOCK_PATH, SOCK_DGRAM);
        if (sfd == -1)
            errExit("unixBind");

    } else {
        lfd = unixBind(SOCK_PATH, SOCK_STREAM);
        if (lfd == -1)
            errExit("unixBind");

        if (listen(lfd, 5) == -1)
            errExit("listen");

        sfd = accept(lfd, NULL, NULL);
        if (sfd == -1)
            errExit("accept");
    }

    /* We must set the SO_PASSCRED socket option in order to receive
       credentials */

    optval = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)) == -1)
        errExit("setsockopt");

    /* The 'msg_name' field can be set to point to a buffer where the
       kernel will place the address of the peer socket. However, we don't
       need the address of the peer, so we set this field to NULL. */

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;

    /* Set fields of 'msgh' to point to a buffer used to receive
       the (real) data read by recvmsg() */

    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    iov.iov_base = &data;
    iov.iov_len = sizeof(int);

    /* Set 'msgh' fields to describe the ancillary data buffer.

       The 'optControlMsgSize' value (specified as a command-line option)
       can be used to artifically limit the size of the received ancillary
       data. This can be used to demonstrate that when the buffer size is
       too small, the list of received file descriptors is truncated, and
       the excess file descriptors are automatically closed. */

    msgh.msg_control = controlMsg;
    msgh.msg_controllen = (optControlMsgSize == -1) ?
                          controlMsgSize : optControlMsgSize;

    /* Receive real plus ancillary data */

    NumReceived = recvmsg(sfd, &msgh, 0);
    if (NumReceived == -1)
        errExit("recvmsg");

    printf("recvmsg() returned %ld\n", (long) NumReceived);

    if (NumReceived > 0)
        printf("Received data = %d\n", data);

    if (optControlMsgSize != -1) {
        char cbuf[1000];

        /* Display this process's set of open file descriptors via
           /proc/PID/fd */

        printf("=================================\n");
        snprintf(cbuf, sizeof(cbuf), "ls -l /proc/%ld/fd", (long) getpid());
        system(cbuf);
        printf("=================================\n");
    }

    /* Check to see if the ancillary data was truncated */

    if (msgh.msg_flags & MSG_CTRUNC)
        printf("********** Ancillary data was truncated!!! **********\n");

    /* Walk through the series of headers in the ancillary data */

    for (cmsgp = CMSG_FIRSTHDR(&msgh);
             cmsgp != NULL;
             cmsgp = CMSG_NXTHDR(&msgh, cmsgp)) {

        printf("=================================\n");
        printf("cmsg_len: %ld\n", (long) cmsgp->cmsg_len);

        /* Check that 'cmsg_level' is as expected */

        if (cmsgp->cmsg_level != SOL_SOCKET)
            fatal("cmsg_level != SOL_SOCKET");

        switch (cmsgp->cmsg_type) {

        case SCM_RIGHTS:        /* Header containing file descriptors */

            /* The number of file descriptors is the size of the control
               message block minus the size that would be allocated for
               a zero-length data block (i.e., the size of the 'cmsghdr'
               structure plus padding), divided by the size of a file
               descriptor */

            fdCnt = (cmsgp->cmsg_len - CMSG_LEN(0)) / sizeof(int);
            printf("SCM_RIGHTS: received %d file descriptors\n", fdCnt);

            /* Set 'fdList' to point to the first descriptor in the
               control message data */

            fdList = ((int *) CMSG_DATA(cmsgp));

            /* For each of the received file descriptors, display the file
               descriptor number and read and display the file content */

            for (j = 0; j < fdCnt; j++) {
                printf("--- [%d] Received FD %d\n", j, fdList[j]);

                for (;;) {
                    char buf[BUF_SIZE];
                    ssize_t numRead;

                    numRead = read(fdList[j], buf, BUF_SIZE);
                    if (numRead == -1)
                        errExit("read");

                    if (numRead == 0)
                        break;

                    write(STDOUT_FILENO, buf, numRead);
                }

                if (close(fdList[j]) == -1)
                    errExit("close");
            }
            break;

        case SCM_CREDENTIALS:   /* Header containing credentials */

            /* Check validity of the 'cmsghdr' */

            if (cmsgp->cmsg_len != CMSG_LEN(sizeof(struct ucred)))
                fatal("cmsg data has incorrect size");

            /* The data in this control message block is a 'struct ucred' */

            ucredp = (struct ucred *) CMSG_DATA(cmsgp);
            printf("SCM_CREDENTIALS: pid=%ld, uid=%ld, gid=%ld\n",
                        (long) ucredp->pid, (long) ucredp->uid,
                        (long) ucredp->gid);
            break;

        default:
            fatal("Bad cmsg_type (%d)", cmsgp->cmsg_type);
        }
    }

    exit(EXIT_SUCCESS);
}
