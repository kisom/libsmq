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
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "smq.h"


static int lock_queue(struct smq *, int);
static int unlock_queue(struct smq *);
static struct smq_msg *smq_entry_to_msg(struct smq_entry *);
static struct smq_entry *msg_to_smq_entry(struct smq_msg *);


/*
 * smq_create initialises and returns a new, empty message queue.
 */
struct smq *
smq_create()
{
	struct smq *queue;
	int sem_error = -1;

	queue = NULL;
	queue = (struct smq *)malloc(sizeof(struct smq));
	if (NULL == queue)
		return NULL;
	queue->queue = (struct tq_msg *)malloc(sizeof(struct tq_msg));
	if (NULL != queue->queue) {
		TAILQ_INIT(queue->queue);
		queue->queue_len = 0;
                queue->refs++;
                queue->timeo.tv_sec = 30;
		sem_error = sem_init(&queue->sem, 0, 0);
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
 * smq_enqueue adds a new message to the queue. The message structure will
 * be freed if the message was successfully added to the queue.
 */
int
smq_enqueue(struct smq *queue, struct smq_msg *message)
{
	struct smq_entry        *entry;
        int                      retval;

	if (NULL == queue)
		return -1;

	entry = msg_to_smq_entry(message);
	if (entry == NULL) {
                printf("[!] NULL entry!\n");
		return -1;
        }

        retval = lock_queue(queue, SMQ_BLOCKING);
	if (0 == retval) {
		TAILQ_INSERT_TAIL(queue->queue, entry, entries);
		queue->queue_len++;
		free(message);
		return unlock_queue(queue);
	}
        printf("[!] couldn't lock queue: %d\n", retval);
        perror("lock_queue");
	return -1;
}


/*
 * smq_dequeue retrieves the next message from the queue.
 */
struct smq_msg *
smq_dequeue(struct smq *queue)
{
	struct smq_msg *message = NULL;
	struct smq_entry *entry = NULL;

        if (NULL == queue)
                return NULL;
	if (0 == lock_queue(queue, SMQ_BLOCKING)) {
		entry = TAILQ_FIRST(queue->queue);
		if (NULL != entry) {
			message = smq_entry_to_msg(entry);
			TAILQ_REMOVE(queue->queue, entry, entries);
			free(entry);
			unlock_queue(queue);
		}
	}
	return message;
}


/*
 * smq_destroy carries out the proper destruction of a message queue. All
 * messages are fully destroyed.
 */
int
smq_destroy(struct smq *queue)
{
	struct smq_entry *entry;
	int retval;

        if (NULL == queue)
                return 0;

	if (0 != (retval = (lock_queue(queue, SMQ_BLOCKING)))) {
		return retval;
	}
        queue->refs--;

        if (queue->refs) {
                return unlock_queue(queue);
        }

	while (NULL != (entry = TAILQ_FIRST(queue->queue))) {
		free(entry->data);
		TAILQ_REMOVE(queue->queue, entry, entries);
		free(entry);
	}

	free(queue->queue);
	if (0 != (retval = (unlock_queue(queue))))
		return retval;
        retval = sem_destroy(&queue->sem);
	free(queue);
	return 0;
}


/*
 * smq_len returns the number of messages in the queue.
 */
size_t
smq_len(struct smq * queue)
{
	if (NULL == queue) {
		return 0;
	} else {
		return queue->queue_len;
	}
}


/*
 * msg_create creates a new message structure for passing into a message queue.
 */
struct smq_msg
*
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
 * smq_entry_to_msg takes a smq_entry and returns a smq_msg from it.
 */
struct smq_msg
*
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
 * msg_to_smq_entry converts a smq_msg to a smq_entry.
 */
struct smq_entry *
msg_to_smq_entry(struct smq_msg *message)
{
	struct smq_entry *entry;

        if (NULL == message)
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
lock_queue(struct smq *queue, int doblock)
{
        struct timeval   ts;
        int              retval;

        ts.tv_sec = queue->timeo.tv_sec;
        ts.tv_usec = queue->timeo.tv_usec;

        retval = sem_trywait(&queue->sem);
        if ((-1 == retval) && (SMQ_BLOCKING == doblock)) {
                select(0, NULL, NULL, NULL, &ts);
                return sem_trywait(&queue->sem);
        }
        return retval;
}


/*
 * release the lock on a queue. only unlocks if queue is locked.
 */
int
unlock_queue(struct smq *queue)
{
        int      retval = 0;
        int      sval = 0;       /* current semaphore value */

        if (0 != (retval = sem_getvalue(&queue->sem, &sval)))
                return retval;
        if (sval > 0)
                return 0;
	return sem_post(&queue->sem);
}
