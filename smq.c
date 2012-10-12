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

#include <sys/types.h>
#include <sys/queue.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "smq.h"

extern const size_t	MSG_MAX_SZ;
extern const time_t	LOCK_WAIT_S;
extern const long	LOCK_WAIT_US;


static int              acquire_lock(pthread_mutex_t, struct timespec *, int);

/*
 * create and initialise a new message queue. properly allocates all
 * resources, and returns the allocated and initialised msgqueue. if there
 * was a failure to allocate memory, returns NULL.
 */
s_msgqueuep
msgqueue_create()
{
	s_msgqueuep	msgq;
	int		error = -1;

	msgq = NULL;
	if (NULL == (msgq = calloc(1, sizeof(*msgq))))
		return msgq;
	if (NULL != (msgq->queue = calloc(1, sizeof(struct tq_msg)))) {
		TAILQ_INIT(msgq->queue);
		msgq->nmsg = 0;
		msgq->block.tv_sec = LOCK_WAIT_S;
		msgq->block.tv_nsec = LOCK_WAIT_NS;
		error = pthread_mutex_init(&msgq->mtx, NULL);
	}
	if (NULL != msgq && error) {
		free(msgq->queue);
		msgq->queue = NULL;
		free(msgq);
		msgq = NULL;
	}

	return msgq;
}


/*
 * msgqueue_push adds a new message to the queue.
 */
int
msgqueue_push(s_msgqueuep msgq, const char *msgdata)
{
	struct s_msg	*msg;
	size_t		cplen;
	int		error = -1;

        if (msgq == NULL || msgq->queue == NULL || msgdata == NULL)
                return error;
        else if (NULL == (msg = calloc(1, sizeof(struct s_msg))))
		return error;

	cplen = strlen(msgdata);
	cplen = (cplen + 1) > MSG_MAX_SZ ? MSG_MAX_SZ : cplen + 1;

	if (NULL == (msg->msg = calloc(cplen + 1, sizeof(char)))) {
                free(msg);
                msg = NULL;
                return error;
	}

        if (0 == (error = acquire_lock(msgq->mtx, &msgq->block, 0))) {
                memcpy(msg->msg, msgdata, cplen);
                msg->seq = ++msgq->lastseq;
                msg->msglen = cplen-1;
                TAILQ_INSERT_TAIL(msgq->queue, msg, msglst);
                msgq->nmsg++;
                error = pthread_mutex_unlock(&msgq->mtx);
        }
	return error;
}


/*
 * msgqueue_pop removes a message from the queue and returns it.
 * the caller should pass an empty *msg structure in; if it's NULL
 * after the call, an error occurred. Either the queue is empty (which
 * can be checked with msgq->nmsg), memory couldn't be allocated, or
 * a lock on the queue could not be obtained.
 */
struct s_message *
msgqueue_pop(s_msgqueuep msgq)
{
	struct s_msg            *msgh = NULL;
        struct s_message        *msg = NULL;
        int                     error = -1;

        if (NULL == msgq || NULL == msgq->queue)
                return msg;
	else if (NULL == (msg = calloc(1, sizeof(struct s_message))))
		return msg;
	else if (0 == (error = acquire_lock(msgq->mtx, &msgq->block, 0))) {
		msgh = TAILQ_FIRST(msgq->queue);
                if ((msgh != NULL) &&
                    (NULL != (msg->msg = calloc((msgh->msglen) + 1, 
                                                 sizeof(*msgh->msg))))) {
	                memcpy(msg->msg, msgh->msg, msgh->msglen);
                        msg->msglen = msgh->msglen;
                        msg->seq = msgh->seq;
                        free(msgh->msg);
                        msgh->msg = NULL;
                        TAILQ_REMOVE(msgq->queue, msgh, msglst);
                        msgq->nmsg--;
                        free(msgh);
                        msgh = NULL;
                } else {
                        free(msgh);
                        free(msg);
                        msgh = NULL;
                        msg = NULL;
                }
		error = pthread_mutex_unlock(&msgq->mtx);
	} else {
                free(msg);
                msg = NULL;
        }

	return msg;
}


/*
 * destroy a message queue, properly clearing the list and freeing all
 * allocated memory.
 */
int
msgqueue_destroy(s_msgqueuep msgq)
{
	struct s_msg	*msg;
	int		error = -1;

	if (0 == (error = acquire_lock(msgq->mtx, &msgq->block, 0))) {
		while ((msg = TAILQ_FIRST(msgq->queue))) {
			free(msg->msg);
			msg->msg = NULL;
			TAILQ_REMOVE(msgq->queue, msg, msglst);
			free(msg);
			msg = NULL;
		}
		free(msgq->queue);
		msgq->queue = NULL;
		error = (pthread_mutex_unlock(&msgq->mtx));
	}

	if (0 == error) {
		pthread_mutex_destroy(&msgq->mtx);
		free(msgq);
		msgq = NULL;
	}
	
	return error;
}


/*
 * acquire_lock attempts to lock the message queue, returning 1 if
 * successful, and 0 if not.
 */
static int
acquire_lock(pthread_mutex_t mtx, struct timespec *ts, int wait)
{
        int             error;
        struct timeval  timeo = { 0, 10000 };

        do {
                timeo.tv_sec = ts->tv_sec;
                timeo.tv_usec = ts->tv_nsec * 1000;
                error = pthread_mutex_trylock(&mtx);
                if (EINVAL == error)
                        break;
                else if (error && wait)
                        select(0, NULL, NULL, NULL, &timeo);
                        
        } while (wait && error != 0);

        return error;
}
