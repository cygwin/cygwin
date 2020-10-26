/* Adapted from the code for debug/backlog.c in Stevens, Unix Network
   Programming. */

#include "af_unix_hdr.h"

int pipefd[2];
#define pfd pipefd[1]   /* parent's end */
#define cfd pipefd[0]   /* child's end */

/* function prototypes */
void do_parent(void);
void do_child(void);

int
main (int argc, char **argv)
{
  pid_t pid;

  if (socketpair (AF_UNIX, SOCK_STREAM, 0, pipefd) < 0)
    errExit ("socketpair");
  if ((pid = fork ()) < 0)
    errExit ("fork");
  else if (pid == 0)
    do_child();
  else
    do_parent();
}

void
do_parent (void)
{
  int count, junk;

  if (close (cfd) < 0)
    errExit ("close");

  for (count = 0; count < 10; count++)
    {
      printf ("count = %d\n", count);
      /* tell child value */
      if (write (pfd, &count, sizeof (int)) != sizeof (int))
	errExit ("write");
      /* wait for child */
      if (read (pfd, &junk, sizeof (int)) < 0)
	errExit ("read");
      sleep (1);
    }
  count = -1;       /* tell child we're all done */
  if (write (pfd, &count, sizeof (int)) != sizeof (int))
    errExit ("write");
}

void
do_child(void)
{
  int count, junk;

  if (close (pfd) < 0)
    errExit ("close");
  /* wait for parent */
  if (read (cfd, &count, sizeof (int)) < 0)
    errExit ("read");
  while (count >= 0)
    {
      /* tell parent */
      if (write (cfd, &junk, sizeof (int)) != sizeof (int))
	errExit ("write");
      /* wait for parent */
      if (read (cfd, &count, sizeof (int)) < 0)
	errExit ("read");
    }
}
