/* mqueue.h: POSIX message queue interface

   This file is part of Cygwin.

   This software is a copyrighted work licensed under the terms of the
   Cygwin license.  Please consult the file "CYGWIN_LICENSE" for
   details. */

#include <time.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/cdefs.h>
#include <cygwin/bits.h>

#ifndef _MQUEUE_H
#define _MQUEUE_H

__BEGIN_DECLS

typedef intptr_t mqd_t;

struct mq_attr {
  long  mq_flags;	/* Message queue flags */
  long  mq_maxmsg;	/* Max number of messages in queue */
  long  mq_msgsize;	/* Max message size */
  long  mq_curmsgs;	/* Current number of messages in queue */
};

int     mq_close (mqd_t);
int     mq_getattr (mqd_t, struct mq_attr *);
int     mq_notify (mqd_t, const struct sigevent *);
mqd_t   mq_open (const char *, int, ...);
ssize_t mq_receive (mqd_t, char *, size_t, unsigned int *);
int     mq_send (mqd_t, const char *, size_t, unsigned int);
int     mq_setattr (mqd_t, const struct mq_attr *, struct mq_attr *);
ssize_t mq_timedreceive (mqd_t, char *, size_t, unsigned int *,
			 const struct timespec *);
int     mq_timedsend (mqd_t, const char *, size_t, unsigned int,
		      const struct timespec *);
int     mq_unlink (const char *name);

#ifdef __INSIDE_CYGWIN__
enum
{
  _MQ_ALLOW_PARTIAL	= _BIT ( 0), /* allow partial reads */
  _MQ_PEEK	        = _BIT ( 1), /* Peek into the packet, return data,
					but don't touch the packet at all
					(MSG_PEEK) */
  _MQ_PEEK_NONBLOCK     = _BIT ( 2), /* Peek into the packet, return data,
					but don't touch the packet at all,
					and don't block (grab_admin_pkt) */
  _MQ_HOLD_LOCK         = _BIT ( 3), /* Don't unlock mutex after reading. */
};

ssize_t _mq_recv (mqd_t, char *, size_t, int);
ssize_t _mq_timedrecv (mqd_t, char *, size_t, const struct timespec *, int);
ssize_t _mq_peek (mqd_t, char *, size_t, bool);
int     _mq_unlock (mqd_t);
#endif

__END_DECLS

#endif /* _MQUEUE_H */
