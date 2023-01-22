#ifndef TESTING_MISC_H
#define TESTING_MISC_H

#include "./chunit.h"

void write_test_bytes(uint8_t *ptr, uint64_t len, uint8_t b);

void check_test_bytes(chunit_test_context *tc, 
        uint8_t *ptr, uint64_t len, uint8_t e_b);

#endif
