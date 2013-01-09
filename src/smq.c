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


static struct msgq_msg          *msgq_entry_to_msg(struct msgq_entry *);
static struct msgq_entry        *msg_to_msgq_entry(struct msgq_msg *);

/*
 * msgq_create initialises and returns a new, empty message queue.
 */
struct msqg
*msgq_create()
{

}


/*
 * msgq_enqueue adds a new message to the queue.
 */
int
msgq_enqueue(struct msgq_msg *queue)
{

}


/*
 * msgq_dequeue retrieves the next message from the queue.
 */
struct msgq_msg
*msgq_dequeue()
{

}


/*
 * msgq_destroy carries out the proper destruction of a message queue.
 */
int
msgq_destroy(struct msgq *queue)
{

}


/*
 * msgq_len returns the number of messages in the queue.
 */
size_t
msgq_len(struct msgq *queue)
{

}


/*
 * msg_create creates a new message structure for passing into a message queue.
 */
struct msgq_msg
*msg_create(void *message_data, size_t message_len)
{

}


/*
 * msg_destroy cleans up a message.
 */
int
msg_destroy(struct msgq_msg *message)
{

}


/*
 * msgq_entry_to_msg takes a msgq_entry and returns a msgq_msg from it.
 */
struct msgq_msg
*msgq_entry_to_msg(struct msgq_entry *entry)
{

}


/*
 * msg_to_msgq_entry converts a msgq_msg to a msgq_entry.
 */
struct msgq_entry
*msg_to_msgq_entry(struct msgq_msg *message)
{

}
