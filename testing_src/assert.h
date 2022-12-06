#ifndef TESTING_ASSERT_H
#define TESTING_ASSERT_H

#include <stdint.h>
#include "./chunit.h"

void chunit_child_process(int fds[2], const chunit_test *test);

// Below are the only chunit calls which do not
// come with chunit prefix.
void assert_true(chunit_test_context *tc, int actual);
void assert_false(chunit_test_context *tc, int actual);
void assert_non_null(chunit_test_context *tc, void *ptr);

void assert_eq_ptr(chunit_test_context *tc, void *expected, void *actual);
void assert_eq_int(chunit_test_context *tc, int64_t expected, int64_t actual);
void assert_eq_uint(chunit_test_context *tc, uint64_t expected, uint64_t actual);
void assert_eq_char(chunit_test_context *tc, char expected, char actual);
void assert_eq_str(chunit_test_context *tc, char *expected, char *actual);

#endif
