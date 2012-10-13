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
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <smq.h>

#define BUFSZ                   32
#define NMSG                    32
static struct s_msgqueue       *mq;

static void     *load_data(void *);
static void     *read_data(void *);
static void      ms_sleep(short);
void     sthrd_test(void);

int
main(void)
{
        void            *thrdv;
        pthread_t        shover;
        pthread_t        pusher;

        printf("[+] start single threaded test\n");
        sthrd_test();

        ms_sleep(1000);
        printf("[+] start multithreaded test\n");
        mq = msgqueue_create();

        printf("[+] pusher robot starts\n");
        pthread_create(&pusher, NULL, read_data, NULL);
        ms_sleep(10);
        printf("[+] shover robot starts\n");
        pthread_create(&shover, NULL, load_data, NULL);

        pthread_join(pusher, &thrdv);
        printf("[+] pusher robot finishes.\n");
        pthread_join(shover, &thrdv);
        printf("[+] shover robot finishes.\n");

        printf("[+] destroying message queue\n");
        return msgqueue_destroy(mq);
}


static void
ms_sleep(short ms)
{
        struct timespec  timeo;
        struct timespec  unslept;
        int              sec = 0;

        if (ms >= 1000) {
                sec = ms / 1000;
        }

        timeo.tv_sec = sec;
        timeo.tv_nsec = (ms % 1000) * 1000;
        while (0 != nanosleep((const struct timespec *)&timeo, &unslept)) {
                timeo = unslept;
        }
}

void *
load_data(void *junk)
{
        char     buf[BUFSZ+1];
        size_t   nsent = 0;
        ssize_t  rdsz;
        size_t   i = 0;
        int      error = 1;
        FILE    *dr;

        junk = NULL;
        dr = fopen("/dev/urandom", "r");
        assert(NULL != dr);

        while (nsent < NMSG) {
                rdsz = fread(buf, sizeof(char), BUFSZ, dr);
                if (rdsz < BUFSZ)
                        printf("<load data> only read %lu bytes\n",
                            (long unsigned)rdsz);
                for (i = 0; i < BUFSZ; i++) {
                        if (buf[i] == '\0')
                                buf[i] = ' ';
                }
                error = 1;
                while (error) {
                        error = msgqueue_push(mq, (uint8_t *)buf, BUFSZ);
                        if (!error) {
                                printf("[+] <load_data> push %d bytes\n",
                                    BUFSZ);
                        } else {
                                printf("[!] <load_data> failed to push "
                                       "%d bytes\n", BUFSZ);
                                ms_sleep(25);
                        }
                }
                nsent++;
        }

        printf("[+] <shover> sent %lu messages\n", (long unsigned)nsent);
        fclose(dr);
        printf("[+] shover robot is almost finished...\n");
        pthread_exit(NULL);
        printf("[+] shover robot is terminating its thread!\n");
        return junk;
}


void *
read_data(void *junk)
{
        struct s_message *msg;
        size_t  cnt = 0;
        size_t  last = 0;

        while (cnt < NMSG) {
                msg = msgqueue_pop(mq);
                if (NULL == msg) {
                        ms_sleep(10);
                        continue;
                }
                printf("[+] (%4lu) received %lu bytes (msg seq: %lu)\n",
                    (long unsigned)cnt,
                    (long unsigned)strlen((char *)msg->msg),
                    (long unsigned)msg->seq);

                if (last != msg->seq-1) {
                        printf("[-] <read_data> sequence miss\n");
                } else if (msg->msglen != BUFSZ) {
                        printf("[-] <read_data> invald read size:\n");
                        printf("\tmsg data: '%s'\n", msg->msg);
                }
                cnt++;
                last = msg->seq;
                free(msg->msg);
                msg->msg = NULL;
                free(msg);
                msg = NULL;
        }
        printf("pusher robot is almost finished...\n");
        pthread_exit(NULL);
        return junk;
}


void
sthrd_test()
{
        struct s_msgqueue       *msq;
        struct s_message        *msg = NULL;

        msq = msgqueue_create();
        if (0 != msgqueue_push(msq, (uint8_t *)"foo", strlen("foo")))
                printf("[!] push 1 failed\n");
        if (0 != msgqueue_push(msq, (uint8_t *)"bar", strlen("bar")))
                printf("[!] push 2 failed\n");
        if (NULL == (msg = msgqueue_pop(msq)))
                printf("[!] pop failed\n");
        else {
                printf("[+] first message: %s (seq=%lu)\n", msg->msg,
                    (long unsigned)msg->seq);
        }

        if (0 != msgqueue_destroy(msq))
                printf("[!] failed to destroy message queue\n");
        printf("[+] finishing single threaded test\n");
}
