/* Adapted from Steven, Unix Network Programming
   https://github.com/unpbook/unpv13e.git */

#include "af_unix_hdr.h"

ssize_t
write_fd (int fd, void *ptr, size_t nbytes, int sendfd)
{
  struct msghdr msg;
  struct iovec iov[1];

  union {
	struct cmsghdr cm;
	char control[CMSG_SPACE(sizeof(int))];
  } control_un;
  struct cmsghdr *cmptr;

  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof control_un.control;

  cmptr = CMSG_FIRSTHDR (&msg);
  cmptr->cmsg_len = CMSG_LEN (sizeof (int));
  cmptr->cmsg_level = SOL_SOCKET;
  cmptr->cmsg_type = SCM_RIGHTS;
  *((int *) CMSG_DATA (cmptr)) = sendfd;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;

  iov[0].iov_base = ptr;
  iov[0].iov_len = nbytes;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;

  return sendmsg (fd, &msg, 0);
}

ssize_t
Write_fd (int fd, void *ptr, size_t nbytes, int sendfd)
{
  ssize_t n;

  if ((n = write_fd (fd, ptr, nbytes, sendfd)) < 0)
	errExit ("write_fd");
  return n;
}
