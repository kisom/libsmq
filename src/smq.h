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

#define SMQ_CONTAINER_ONLY      0
#define SMQ_DESTROY_ALL         1


struct smq_msg {
        void    *data;
        size_t   data_len;
};

struct smq_entry {
        void                    *data;
        size_t                   data_len;
        TAILQ_ENTRY(smq_entry)   entries;
};
TAILQ_HEAD(tq_msg, smq_entry);

struct smq {
        struct tq_msg   *queue;
        size_t           queue_len;
        pthread_mutex_t  mtx;
};


struct smq      *smq_create(void);
int              smq_enqueue(struct smq *, struct smq_msg *);
struct smq_msg  *smq_dequeue(struct smq *);
int              smq_destroy(struct smq *);
size_t           smq_len(struct smq *);

struct smq_msg  *msg_create(void *, size_t);
int              msg_destroy(struct smq_msg *, int);
#endif
