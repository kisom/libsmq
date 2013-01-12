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
#define SMQ_TEST_RUNS           1000000


void    *pusher_run(void *);
void    *puller_run(void *);


static void
test_simple_smq(void)
{
	smq msgq;
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

	CU_ASSERT(0 == smq_send(msgq, message));

	message = smq_receive(msgq);
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
	smq msgq;
	struct smq_msg *message;
	int origin;
	int *data;

	msgq = smq_create();
	data = (int *)malloc(sizeof(int));
	CU_ASSERT(NULL != data);
	origin = 2;
	memcpy(data, &origin, sizeof(origin));
	message = smq_msg_create(data, sizeof(origin));
	CU_ASSERT(0 == smq_send(msgq, message));

	data = (int *)malloc(sizeof(int));
	CU_ASSERT(NULL != data);
	origin = 1;
	memcpy(data, &origin, sizeof(origin));
	message = smq_msg_create(data, sizeof(origin));
	CU_ASSERT(0 == smq_send(msgq, message));

	message = smq_receive(msgq);
	CU_ASSERT(NULL != message);
	CU_ASSERT(message->data_len == sizeof(origin));
	CU_ASSERT(2 == *((int *)message->data));
	CU_ASSERT(0 == smq_msg_destroy(message, SMQ_DESTROY_ALL));

	message = smq_receive(msgq);
	CU_ASSERT(NULL != message);
	CU_ASSERT(message->data_len == sizeof(origin));
	CU_ASSERT(1 == *((int *)message->data));
	CU_ASSERT(0 == smq_msg_destroy(message, SMQ_DESTROY_ALL));

	CU_ASSERT(0 == smq_destroy(msgq));
}


static void
test_million_allocs(void)
{
	int i;

	for (i = 0; i < 100000; i++)
		test_simple_smq();
}


static void
test_threaded_smq(void)
{
        smq              msgq;
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
        CU_ASSERT(0 == status);
        CU_ASSERT(NULL != thread_res);
        CU_ASSERT(*(int *)thread_res == SMQ_TEST_RUNS);
        free(thread_res);

        status = pthread_join(puller_thd, &thread_res);
        CU_ASSERT(0 == status);
        CU_ASSERT(NULL != thread_res && *(int *)thread_res == SMQ_TEST_RUNS);
        free(thread_res);

        CU_ASSERT(0 == smq_len(msgq));
        CU_ASSERT(0 == (status = smq_destroy(msgq)));
        if (0 != status)
                fprintf(stderr, "[!] error destroying smq (%d)\n");
}


void *
pusher_run(void *args)
{
        smq              msgq;
        struct smq_msg  *message;
        int             *data;
        int             *i;     /* int pointer so we can return this    */
                                /*      from thread                     */

        i = (int *)malloc(sizeof(*i));
        if (NULL == i) {
                pthread_exit(NULL);
        }

        msgq = (smq)args;
        if (smq_dup(msgq))
                pthread_exit(NULL);

        for (*i = 0; *i < SMQ_TEST_RUNS; (*i)++) {
                data = (int *)malloc(sizeof(i));
                if (NULL == data) {
                        smq_destroy(msgq);
                        pthread_exit(i);
                }

                *data = *i;
                message = smq_msg_create((void *)data, sizeof(*data));
                if (NULL == message) {
                        smq_destroy(msgq);
                        free(data);
                        pthread_exit(i);
                } else {
                        while (0 != smq_send(msgq, message)) ;
                }
        }
        smq_destroy(msgq);
        pthread_exit(i);
        return NULL;
}


void *
puller_run(void *args)
{
        smq              msgq;
        struct smq_msg  *message;
        int             *msg_count;

        msg_count = (int *)malloc(sizeof(*msg_count));
        if (NULL == msg_count) {
                pthread_exit(NULL);
        }

        *msg_count = 0;
        msgq = (smq)args;
        if (smq_dup(msgq))
                pthread_exit(NULL);

        while (1) {
                message = smq_receive(msgq);
                if (NULL == message)
                        continue;
                else if (message->data == NULL)
                        continue;
                else if (*(int *)message->data != *msg_count)
                        printf("?%d,%d\n", *(int *)message->data, *msg_count);
                else
                        (*msg_count)++;

                if (0 != smq_msg_destroy(message, SMQ_DESTROY_ALL)) {
                        smq_destroy(msgq);
                        pthread_exit(msg_count);
                }

                if (SMQ_TEST_RUNS == *msg_count)
                        break;
        }
        smq_destroy(msgq);
        pthread_exit(msg_count);
        return NULL;
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
main(int argc, char *argv[])
{
	CU_pSuite smq_suite = NULL;
	unsigned int fails = 0;
        int c;
        int long_test = 1;

        while (-1 != (c = getopt(argc, argv, "hm"))) {
                switch(c) {
                case 'm':
                        long_test = 0;
                        break;
                case 'h':
                        fprintf(stderr, "usage: %s [-hm]\n", argv[0]);
                        fprintf(stderr, "options:\n");
                        fprintf(stderr, "\t-h\tdisplay this help message\n");
                        fprintf(stderr, "\t-m\tdon't run the million alloc");
                        fprintf(stderr, "\t\t test\n");
                        return 0;
                default:
                        fprintf(stderr, "invalid option\n");
                        return EXIT_FAILURE;
                }
        }

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

	if (long_test && NULL == CU_add_test(smq_suite, "one million allocs",
                                             test_million_allocs))
		destroy_test_registry();

	if (NULL == CU_add_test(smq_suite, "threaded smq test",
				test_threaded_smq))
		destroy_test_registry();

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	fails = CU_get_number_of_tests_failed();

	CU_cleanup_registry();
	return fails;
}
