#ifndef TESTING_OUTPUT_H
#define TESTING_OUTPUT_H

#include "./chunit.h"

extern const char *CHUNIT_TR_NAMES[CHUNIT_FATAL_ERROR + 1];
extern const char *CHUNIT_FE_NAMES[CHUNIT_TERMINATION_ERROR + 1];

void chunit_print_test_run(chunit_test_run *tr);
void chunit_print_test_suite_run(chunit_test_suite_run *tsr);
void chunit_print_test_module_run(chunit_test_module_run *tmr);

#endif
