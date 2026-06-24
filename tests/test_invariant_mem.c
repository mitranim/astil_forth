#include <check.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <setjmp.h>

/* Include the production buffer API from clib/mem.c */
#include "clib/mem.h"

START_TEST(test_buf_reserve_no_null_deref)
{
    /* Invariant: After any buf operation, buf->dat must never be NULL
     * if data was successfully written, and the process must not crash
     * due to NULL dereference from a failed realloc going undetected. */

    /* Payloads: sizes to append/reserve — valid small, boundary, large */
    size_t payloads[] = {
        1,          /* valid small input */
        4096,       /* boundary: typical page size */
        1024 * 64,  /* larger allocation stress */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        Buf buf = {0};

        /* buf_append or buf_reserve must either succeed with valid dat,
         * or terminate the process — never silently store NULL */
        buf_reserve(&buf, (Ind)payloads[i]);

        /* If we reach here, realloc succeeded: dat must be non-NULL */
        ck_assert_msg(buf.dat != NULL,
            "buf.dat is NULL after buf_reserve — silent realloc failure");
        ck_assert_msg(buf.cap >= (Ind)payloads[i],
            "buf.cap is less than requested — capacity invariant violated");

        free(buf.dat);
        buf.dat = NULL;
        buf.cap = 0;
        buf.len = 0;
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buf_reserve_no_null_deref);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}