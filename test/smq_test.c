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


#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "smq.h"


#define SMQ_TEST_DATA           "a quick brown fox jumps over the lazy dog"
#define SMQ_TEST_RUNS           32


void    *pusher_run(void *);
void    *puller_run(void *);


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


static void
test_simple_smq(void)
{
	struct smq *msgq;
	struct smq_msg *message;
	char *msg_data;
	int retval = 0;

	msgq = smq_create();
	CU_ASSERT(NULL != msgq);

	msg_data = (char *)malloc(sizeof(char) * strlen(SMQ_TEST_DATA) + 1);
	CU_ASSERT(NULL != msg_data);
	strncpy(msg_data, SMQ_TEST_DATA, strlen(SMQ_TEST_DATA));
	message = smq_msg_create(msg_data, strlen(msg_data));
	CU_ASSERT(NULL != message);

	CU_ASSERT(0 == smq_enqueue(msgq, message));

	message = smq_dequeue(msgq);
	CU_ASSERT(NULL != message);
	CU_ASSERT(message->data_len == strlen(SMQ_TEST_DATA));
	CU_ASSERT(0 == (strncmp(message->data, SMQ_TEST_DATA, 
                                strlen(SMQ_TEST_DATA))));

	CU_ASSERT(0 == smq_msg_destroy(message, SMQ_DESTROY_ALL));

	retval = smq_destroy(msgq);
	switch (retval) {
	case 0:
		break;
	case EBUSY:
		fprintf(stderr, "[!] mutex is already locked.\n");
		break;
	case EINVAL:
		fprintf(stderr, "[!] invalid mutex.\n");
		break;
	default:
		fprintf(stderr, "[!] invalid state: error = %d\n", retval);
		abort();
	}
	CU_ASSERT(0 == retval);
}


static void
test_queue_ordering(void)
{
	struct smq *msgq;
	struct smq_msg *message;
	int origin;
	int *data;

	msgq = smq_create();
	data = (int *)malloc(sizeof(int));
	CU_ASSERT(NULL != data);
	origin = 2;
	memcpy(data, &origin, sizeof(origin));
	message = smq_msg_create(data, sizeof(origin));
	CU_ASSERT(0 == smq_enqueue(msgq, message));

	data = (int *)malloc(sizeof(int));
	CU_ASSERT(NULL != data);
	origin = 1;
	memcpy(data, &origin, sizeof(origin));
	message = smq_msg_create(data, sizeof(origin));
	CU_ASSERT(0 == smq_enqueue(msgq, message));

	message = smq_dequeue(msgq);
	CU_ASSERT(NULL != message);
	CU_ASSERT(message->data_len == sizeof(origin));
	CU_ASSERT(2 == *((int *)message->data));
	CU_ASSERT(0 == smq_msg_destroy(message, SMQ_DESTROY_ALL));

	message = smq_dequeue(msgq);
	CU_ASSERT(NULL != message);
	CU_ASSERT(message->data_len == sizeof(origin));
	CU_ASSERT(1 == *((int *)message->data));
	CU_ASSERT(0 == smq_msg_destroy(message, SMQ_DESTROY_ALL));

	CU_ASSERT(0 == smq_destroy(msgq));
}


/*
static void
test_million_allocs(void)
{
	int i;

	for (i = 0; i < 1000000; i++)
		test_simple_smq();
}
*/


static void
test_threaded_smq(void)
{
        struct smq      *msgq;
        pthread_t        pusher_thd;
        pthread_t        puller_thd;
        void            *thread_res;
        int              status;

        msgq = smq_create();
        CU_ASSERT(NULL != msgq);

        status = pthread_create(&pusher_thd, NULL, pusher_run, (void *)msgq);
        CU_ASSERT(0 == status);
        status = pthread_create(&puller_thd, NULL, puller_run, (void *)msgq);
        CU_ASSERT(0 == status);

        status = pthread_join(pusher_thd, &thread_res);
        perror("pusher_thd");
        CU_ASSERT(0 == status);
        CU_ASSERT(NULL != thread_res);
        CU_ASSERT(*(int *)thread_res == SMQ_TEST_RUNS);

        status = pthread_join(puller_thd, &thread_res);
        perror("puller_thd");
        CU_ASSERT(0 == status);
        CU_ASSERT(NULL != thread_res);
        CU_ASSERT(*(int *)thread_res == SMQ_TEST_RUNS);

        printf("%u messages on the queue\n", (unsigned int)smq_len(msgq));
        printf("destroying message queue\n");
        CU_ASSERT(0 == smq_destroy(msgq));
}


void *
pusher_run(void *args)
{
        void            *junk = NULL;
        struct smq      *msgq;
        struct smq_msg  *message;
        int             *data;
        int             *i;     /* int pointer so we can return this    */
                                /*      from thread                     */

        printf("[-] pusher running\n");
        i = (int *)malloc(sizeof(*i));
        if (NULL == i)
                pthread_exit(NULL);

        msgq = (struct smq *)args;
        printf("^");
        for (*i = 0; *i < SMQ_TEST_RUNS; (*i)++) {
                printf("1.");
                fflush(stdout);
                data = (int *)malloc(sizeof(i));
                if (NULL == data) {
                        printf("1!a");
                        fflush(stdout);
                        pthread_exit(i);
                }
                *data = *i;
                message = smq_msg_create((void *)data, sizeof(*data));
                if (NULL == message) {
                        printf("1!m");
                        fflush(stdout);
                        free(data);
                        pthread_exit(i);
                } else {
                        while (0 != smq_enqueue(msgq, message)) {
                                printf("1!e");
                                fflush(stdout);
                                ms_sleep(25);
                        }
                }
                printf("1+");
                fflush(stdout);
        }
        pthread_exit(i);
        return junk;
}


void *
puller_run(void *args)
{
        struct smq      *msgq;
        struct smq_msg  *message;
        void            *junk;
        int             *msg_count;

        printf("[-] puller running\n");
        msg_count = (int *)malloc(sizeof(*msg_count));
        if (NULL == msg_count)
                pthread_exit(NULL);

        *msg_count = 0;
        msgq = (struct smq *)args;
        while (1) {
                message = smq_dequeue(msgq);
                if (NULL == message) {
                        break;
                }  else if (message->data == NULL) {
                        printf("2!");
                        fflush(stdout);
                        break;
                }
                if (*(int *)message->data != *msg_count)
                        printf("2?");
                if (0 != smq_msg_destroy(message, SMQ_DESTROY_ALL)) {
                        printf("2!");
                        fflush(stdout);
                        pthread_exit(msg_count);
                }
                (*msg_count)++;
                if (SMQ_TEST_RUNS == *msg_count)
                        break;
        }
        pthread_exit(msg_count);
        return junk;
}


/*
 * suite set up functions
 */
int
initialise_smq_test()
{
	return 0;
}

int
cleanup_smq_test()
{
	return 0;
}


void
destroy_test_registry()
{
	CU_cleanup_registry();
	exit(CU_get_error());
}


int
main(void)
{
	CU_pSuite smq_suite = NULL;
	unsigned int fails = 0;

	printf("starting tests for smq...\n");

	if (!CUE_SUCCESS == CU_initialize_registry()) {
		fprintf(stderr, "error initialising CUnit test registry!\n");
		return EXIT_FAILURE;
	}

	/*
	 * set up the suite
	 */
	smq_suite = CU_add_suite("smq_tests", initialise_smq_test,
				 cleanup_smq_test);
	if (NULL == smq_suite)
		destroy_test_registry();

	/*
	 * add tests
	 */
	if (NULL ==
	    CU_add_test(smq_suite, "single thread test", test_simple_smq))
		destroy_test_registry();

	if (NULL == CU_add_test(smq_suite, "queue ordering test",
				test_queue_ordering))
		destroy_test_registry();

/*
 *      commenting out code for sem debug
	if (NULL == CU_add_test(smq_suite, "one million allocs",
				test_million_allocs))
		destroy_test_registry();
 */
	if (NULL == CU_add_test(smq_suite, "threaded smq test",
				test_threaded_smq))
		destroy_test_registry();

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	fails = CU_get_number_of_tests_failed();

	CU_cleanup_registry();
	return fails;
}
