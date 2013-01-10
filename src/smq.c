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
#include <pthread.h>
#include <stdlib.h>

#include "smq.h"


static int                       lock_queue(struct smq *);
static int                       unlock_queue(struct smq *);
static struct smq_msg          *smq_entry_to_msg(struct smq_entry *);
static struct smq_entry        *msg_to_smq_entry(struct smq_msg *);

/*
 * smq_create initialises and returns a new, empty message queue.
 */
struct smq *
smq_create()
{
        struct smq     *queue;
        int              mutex_error = -1;

        queue = NULL;
        queue = (struct smq *)malloc(sizeof(struct smq));
        if (NULL == queue)
                return NULL;
        queue->queue = (struct tq_msg *)malloc(sizeof(struct tq_msg));
        if (NULL != queue->queue) {
                TAILQ_INIT(queue->queue);
                queue->queue_len = 0;
                mutex_error = pthread_mutex_init(&queue->mtx, NULL);
        }

        if (mutex_error) {
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
        struct smq_entry       *entry;

        if (NULL == queue)
                return -1;

        entry = msg_to_smq_entry(message);
        if (entry == NULL)
                return -1;

        if (0 == lock_queue(queue)) {
                TAILQ_INSERT_TAIL(queue->queue, entry, entries);
                queue->queue_len++;
                free(message);
                return unlock_queue(queue);
        }
        return -1;
}


/*
 * smq_dequeue retrieves the next message from the queue.
 */
struct smq_msg *
smq_dequeue(struct smq *queue)
{
        struct smq_msg         *message;
        struct smq_entry       *entry;

        if (0 == lock_queue(queue)) {
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
 * smq_destroy carries out the proper destruction of a message queue.
 */
int
smq_destroy(struct smq *queue)
{
        struct smq_entry        *entry;
        int                      retval;

        if (0 != (retval = (lock_queue(queue)))) {
                return retval;
        }

        while (NULL != (entry = TAILQ_FIRST(queue->queue))) {
                free(entry->data);
                TAILQ_REMOVE(queue->queue, entry, entries);
                free(entry);
        }

        free(queue->queue);
        if (0 != (retval = (unlock_queue(queue))))
                return retval;
        pthread_mutex_destroy(&queue->mtx);
        free(queue);
        return 0;
}


/*
 * smq_len returns the number of messages in the queue.
 */
size_t
smq_len(struct smq *queue)
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
*msg_create(void *message_data, size_t message_len)
{
        struct smq_msg *message;

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
msg_destroy(struct smq_msg *message, int opts)
{
        if (SMQ_CONTAINER_ONLY != opts)
                free(message->data);
        free(message);
}


/*
 * smq_entry_to_msg takes a smq_entry and returns a smq_msg from it.
 */
struct smq_msg
*smq_entry_to_msg(struct smq_entry *entry)
{
        struct smq_msg *message;

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
        struct smq_entry       *entry;

        entry = (struct smq_entry *)malloc(sizeof(struct smq_entry));
        if (NULL == entry)
                return entry;
        entry->data = message->data;
        entry->data_len = message->data_len;
        return entry;
}


/*
 * attempt to acquire the lock for the message queue.
 */
int
lock_queue(struct smq *queue)
{
        return pthread_mutex_trylock(&queue->mtx);
}


/*
 * release the lock on a queue.
 */
int
unlock_queue(struct smq *queue)
{
        return pthread_mutex_unlock(&queue->mtx);
}
