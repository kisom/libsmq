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
#include <sys/time.h>
#include <semaphore.h>
#include <stdlib.h>

#include "smq.h"


struct smq_entry {
	void                    *data;
	size_t                   data_len;
        TAILQ_ENTRY(smq_entry)   entries;
};
TAILQ_HEAD(tq_msg, smq_entry);

struct _smq {
	struct tq_msg           *queue;
	size_t                   queue_len;
	sem_t                   *sem;
        struct timeval           timeo;
        size_t                   refs;
};


static int lock_queue(smq);
static int unlock_queue(smq);
static int queue_locked(smq);
static struct smq_msg *smq_entry_to_msg(struct smq_entry *);
static struct smq_entry *msg_to_smq_entry(struct smq_msg *);


/*
 * smq_create initialises and returns a new, empty message queue.
 */
smq
smq_create()
{
	smq queue;
	int sem_error = -1;

	queue = NULL;
	queue = (smq)malloc(sizeof(struct _smq));
	if (NULL == queue)
		return NULL;
        if (NULL == (queue->sem = (sem_t *)malloc(sizeof(sem_t))))
                return NULL;
	queue->queue = (struct tq_msg *)malloc(sizeof(struct tq_msg));
	if (NULL != queue->queue) {
		TAILQ_INIT(queue->queue);
		queue->queue_len = 0;
                queue->refs = 1;
                queue->timeo.tv_sec = 0;
                queue->timeo.tv_usec = 10 * 1000;
		sem_error = sem_init(queue->sem, 0, 0);
                if (0 == sem_error) {
                        sem_error = unlock_queue(queue);
                }
	}

	if (sem_error) {
		free(queue->queue);
		free(queue);
		return NULL;
	}

	return queue;
}


/*
 * smq_send adds a new message to the queue. The message structure will
 * be freed if the message was successfully added to the queue.
 */
int
smq_send(smq queue, struct smq_msg *message)
{
	struct smq_entry        *entry;
        int                      retval;

	if (NULL == queue)
		return -1;

	entry = msg_to_smq_entry(message);
	if (entry == NULL) {
		return -1;
        } else if (entry->data == NULL) {
                return -1;
        }

        retval = lock_queue(queue);
	if (0 == retval) {
		TAILQ_INSERT_TAIL(queue->queue, entry, entries);
		queue->queue_len++;
		free(message);
		return unlock_queue(queue);
	}
	return -1;
}


/*
 * smq_receive retrieves the next message from the queue.
 */
struct smq_msg *
smq_receive(smq queue)
{
	struct smq_msg *message = NULL;
	struct smq_entry *entry = NULL;

        if (NULL == queue)
                return NULL;
	if (0 == lock_queue(queue)) {
		entry = TAILQ_FIRST(queue->queue);
		if (NULL != entry) {
			message = smq_entry_to_msg(entry);
			TAILQ_REMOVE(queue->queue, entry, entries);
			free(entry);
			unlock_queue(queue);
                        queue->queue_len--;
		}
	}
        if (queue_locked(queue))
                unlock_queue(queue);
	return message;
}


/*
 * smq_destroy carries out the proper destruction of a message queue. All
 * messages are fully destroyed.
 */
int
smq_destroy(smq queue)
{
	struct smq_entry *entry;
	int retval;

        if (NULL == queue)
                return 0;

	if (0 != (retval = (lock_queue(queue))))
		return retval;

        queue->refs--;

        if (queue->refs > 0)
                return unlock_queue(queue);

	while (NULL != (entry = TAILQ_FIRST(queue->queue))) {
		free(entry->data);
		TAILQ_REMOVE(queue->queue, entry, entries);
		free(entry);
	}

	free(queue->queue);
        while (unlock_queue(queue)) ;
        retval = sem_destroy(queue->sem);
        free(queue->sem);
	free(queue);
	return retval;
}


/*
 * smq_len returns the number of messages in the queue.
 */
size_t
smq_len(smq queue)
{
	if (NULL == queue) {
		return 0;
	} else {
		return queue->queue_len;
	}
}


/*
 * smq_settimeout sets the queue's timeout to the new timeval.
 */
void
smq_settimeout(smq queue, struct timeval *timeo)
{
        if (NULL == timeo)
                return;
        queue->timeo.tv_sec = timeo->tv_sec;
        queue->timeo.tv_usec = timeo->tv_usec;
        return;
}


/*
 * duplicate a message queue for use in another thread.
 */
int
smq_dup(smq queue)
{
        if (NULL == queue)
                return -1;
        if (lock_queue(queue)) {
                return -1;
        }
        queue->refs++;
        while (queue_locked(queue))
                unlock_queue(queue);
        return 0;
}


/*
 * msg_create creates a new message structure for passing into a message queue.
 */
struct smq_msg *
smq_msg_create(void *message_data, size_t message_len)
{
	struct smq_msg *message;

        if (NULL == message_data || message_len == 0)
                return NULL;

	message = (struct smq_msg *)malloc(sizeof(struct smq_msg));
	if (NULL == message) {
		return NULL;
	}
	message->data = message_data;
	message->data_len = message_len;
	return message;
}


/*
 * msg_destroy cleans up a message. If the integer parameter != 0, it will
 * free the data stored in the message.
 */
int
smq_msg_destroy(struct smq_msg *message, int opts)
{
	if (NULL != message && SMQ_CONTAINER_ONLY != opts)
		free(message->data);
	free(message);
	return 0;
}


/*
 * smq_entry_to_msg takes a smq_entry and returns a struct smq_msg *from it.
 */
struct smq_msg *
smq_entry_to_msg(struct smq_entry *entry)
{
	struct smq_msg *message;

        if (NULL == entry)
                return NULL;
	message = (struct smq_msg *)malloc(sizeof(struct smq_msg));
	if (NULL == message)
		return message;
	message->data = entry->data;
	message->data_len = entry->data_len;
	return message;
}


/*
 * msg_to_smq_entry converts a struct smq_msg *to a smq_entry.
 */
struct smq_entry *
msg_to_smq_entry(struct smq_msg *message)
{
	struct smq_entry *entry;

        if (NULL == message)
                return NULL;
        if (NULL == message->data || 0 == message->data_len)
                return NULL;
	entry = (struct smq_entry *)malloc(sizeof(struct smq_entry));
	if (NULL == entry)
		return entry;
	entry->data = message->data;
	entry->data_len = message->data_len;
	return entry;
}


/*
 * attempt to acquire the lock for the message queue. If doblock is set
 * to 1, lock_queue will block until the mutex is locked.
 */
int
lock_queue(smq queue)
{
        struct timeval   ts;
        int              retval;

        ts.tv_sec = queue->timeo.tv_sec;
        ts.tv_usec = queue->timeo.tv_usec;

        retval = sem_trywait(queue->sem);
        if (-1 == retval) {
                select(0, NULL, NULL, NULL, &ts);
                return sem_trywait(queue->sem);
        }
        return retval;
}


/*
 * release the lock on a queue. only unlocks if queue is locked.
 */
int
unlock_queue(smq queue)
{
        if (queue_locked(queue))
	        return sem_post(queue->sem);
        return 0;
}


/*
 * returns 1 if the queue is locked, and 0 if it is unlocked.
 */
static int
queue_locked(smq queue)
{
        int     retval;
        int     semval;

        retval = sem_getvalue(queue->sem, &semval);
        if (-1 == retval)
                return retval;
        return semval == 0;
}
