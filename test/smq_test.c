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

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "smq.h"

#define BUFSZ                   32
#define NMSG                    32


static void
test_simple_smq(void)
{
        struct smq      *msgq;
        struct smq_msg  *message;
        int              retval = 0;

        msgq = smq_create();
        CU_ASSERT(NULL != msgq);

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

/*
 * suite set up functions
 */
int initialise_smq_test()
{
    return 0;
}

int
cleanup_smq_test()
{
    return 0;
}

void destroy_test_registry()
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

    if (! CUE_SUCCESS == CU_initialize_registry()) {
        fprintf(stderr, "error initialising CUnit test registry!\n");
        return EXIT_FAILURE;
    }

    /* set up the suite */
    smq_suite = CU_add_suite("smq_tests", initialise_smq_test,
                                            cleanup_smq_test);
    if (NULL == smq_suite)
        destroy_test_registry();

    /* add tests */
    if (NULL == CU_add_test(smq_suite, "single thread test",
                            test_simple_smq))
        destroy_test_registry();

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    fails = CU_get_number_of_tests_failed();

    CU_cleanup_registry();
    return fails;
}
