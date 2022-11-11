#ifndef TESTING_CHUNIT_H
#define TESTING_CHUNIT_H

#include <stdint.h>
#include <time.h>
#include "../core_src/data.h"

// NOTE :
// An assertion failure is not an error.
// It reveals a bug to the user, but it is not
// an error in the testing framework.
//
// However, a pipe error, or an error writing data...
// these are errors in the program.
// Not the fault of the user.
// So, these situations I think should be treated differently.

// A test result is what is interpreted upon completion of a test.
// Completion of a test implies there were no errors in the framework
// when running the test.
typedef enum {
    // This is for when a result was unable to be interpreted.
    CHUNIT_VOID = 0,

    // To be communicated back to parent 
    // when all goes as expected.
    CHUNIT_SUCCESS,

    // Single arg assertion fails.
    // Just tag will be sent.
    CHUNIT_ASSERT_TRUE_FAIL,
    CHUNIT_ASSERT_FALSE_FAIL,
    CHUNIT_ASSERT_NON_NULL_FAIL,

    // Double arg assertions fails.
    // Tag, expected, actual will be sent.
    CHUNIT_ASSERT_EQ_PTR_FAIL,
    CHUNIT_ASSERT_EQ_INT_FAIL,
    CHUNIT_ASSERT_EQ_UINT_FAIL,
    CHUNIT_ASSERT_EQ_CHAR_FAIL,

    // Tag, expected len, expected, 
    // actual len, actual will be sent.
    CHUNIT_ASSERT_EQ_STR_FAIL,

    // Memory Leak.
    CHUNIT_MEMORY_LEAK,

    // Test took too long.
    CHUNIT_TIMEOUT, 

    // Test had a fatal runtime error.
    CHUNIT_FATAL_ERROR,
    
} chunit_test_result;

// This exit code should be used when there is a 
// write error from the test to the parent.
#define CHUNIT_PIPE_ERROR_EXIT_CODE 3

// This enum is seeing whether or not an error
// occured in the framework.
typedef enum {
    CHUNIT_PIPE_ERROR = 0,
    CHUNIT_FORK_ERROR,
    CHUNIT_BAD_TEST_RESULT,
    CHUNIT_TERMINATION_ERROR,
} chunit_framework_error;

// 5 second timeout for all tests.
#define CHUNIT_TIMEOUT_S 5

// Parent process organization...

typedef struct {
    const char *name;
    const time_t timeout; // Length of test run (in seconds)

    // NOTE, this will be the runnable test itself.
    // It must be given a pipe file descriptor
    // to be able to communicate with the parent.
    void (*t)(int pipe_fd);
} chunit_test;

typedef struct {
    // The test which this run originates from.
    const chunit_test *test;

    // Pid of the child process. 
    // This is only useful if there was a termination
    // error.
    pid_t child;

    // A list of framework errors.
    slist *errors;

    // Only to be interpreted when there 
    // are no chunit errors.
    chunit_test_result result;

    // Finally, a data pointer to any other information
    // needed to interpret the test run...
    // i.e. assertions comparisons.
    void *data;
} chunit_test_run;

// Run a singular test.
chunit_test_run *chunit_run_test(const chunit_test *test);

// This is used for correctly deleting a test_run
// given its result type.
void chunit_delete_test_run(chunit_test_run *tr);

typedef struct {
    const char *name;
    const uint64_t tests_len;
    const chunit_test *tests[];  // An array of pointers ot tests.
} chunit_test_suite;

typedef struct {
    const chunit_test_suite *suite;

    // Slist of chunit_test_run *
    slist *test_runs; 
} chunit_test_suite_run;

chunit_test_suite_run *chunit_run_suite(const chunit_test_suite *suite);

// Delete a test suite run.
// WARNING, this will delete all test runs contained in tsr.
void chunit_delete_test_suite_run(chunit_test_suite_run *tsr);

// NOTE test module is a repeat of
// test suite to add just one more level of hierachy.

typedef struct {
    const char *name;
    const uint64_t suites_len;
    const chunit_test_suite *suites[];
} chunit_test_module; 

typedef struct {
    const chunit_test_module *mod;

    // Slist of chunit_test_suite_run *
    slist *test_suite_runs;
} chunit_test_module_run;

chunit_test_module_run *chunit_run_module(const chunit_test_module *mod);

void chunit_delete_test_module_run(chunit_test_module_run *tmr);

#endif
