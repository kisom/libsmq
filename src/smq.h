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


#ifndef __LIBSMQ_SMQ_H
#define __LIBSMQ_SMQ_H
#include <sys/types.h>
#include <sys/queue.h>
#include <pthread.h>
#include <stdlib.h>


struct msgq_msg {
        void    *data;
        size_t   data_len;
};

struct msgq_entry {
        TAILQ_ENTRY(msgq_msg);
        void  *data;
        size_t  data_len;
};
TAILQ_HEAD(tq_msg, msgq_entry);

struct msgq {
        struct tq_msg   *queue;
        size_t           queue_len;
        pthread_mutex_t  mtx;
};


struct msqg     *msgq_create(void);
int              msgq_enqueue(struct msgq_msg *);
struct msgq_msg *msgq_dequeue(void);
int              msgq_destroy(struct msgq *);
size_t           msgq_len(struct msgq *);

struct msgq_msg *msg_create(void *, size_t);
int              msg_destroy(struct msgq_msg *);
#endif
