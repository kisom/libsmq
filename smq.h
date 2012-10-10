/*
 * Copyright (c) 2012 Kyle Isom <kyle@tyrfingr.is>
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above 
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE 
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 * ---------------------------------------------------------------------
 */

#ifndef __EFFIGY_MSG_H
#define __EFFIGY_MSG_H
#include <sys/queue.h>
#include <pthread.h>
#include <stdlib.h>


/*
 * global constants:
 *	MSG_MAX_SZ is the maximum length of a message.
 *	LOCK_WAIT_S is the maximum number of seconds to wait to acquire a
 *	    lock on the message queue.
 *	LOCK_WAIT_NS is the maximum number of nanoseconds to wait to acquire
 *	    a lock on the message queue.
 */
static const size_t	MSG_MAX_SZ = 4096;
static const time_t	LOCK_WAIT_S = 1;
static const long	LOCK_WAIT_NS = 0;


struct s_message {
	char			*msg;
	size_t			msglen;
	size_t			seq;
};

struct s_msg {
	char			*msg;
	size_t			msglen;
	size_t			seq;
	TAILQ_ENTRY(s_msg)	msglst;
};
TAILQ_HEAD(tq_msg, s_msg);
typedef struct tq_msg * tq_msgp;


struct s_msgqueue {
	tq_msgp		queue;
	pthread_mutex_t	mtx;
	size_t		nmsg;
	size_t		lastseq;
	struct timespec block;
};
typedef struct s_msgqueue * s_msgqueuep;


s_msgqueuep              msgqueue_create(void);
int	                 msgqueue_push(s_msgqueuep, const char *);
struct s_message        *msgqueue_pop(s_msgqueuep);
int	                 msgqueue_destroy(s_msgqueuep);

#endif
