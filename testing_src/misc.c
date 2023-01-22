#include "./misc.h"
#include "assert.h"

void write_test_bytes(uint8_t *ptr, uint64_t len, uint8_t b) {
    uint8_t *iter = ptr;
    uint8_t *end = ptr + len;

    for (; iter < end; iter++) {
        *iter = b;
    }
}

void check_test_bytes(chunit_test_context *tc, 
        uint8_t *ptr, uint64_t len, uint8_t e_b) {
    uint8_t *iter = ptr;
    uint8_t *end = ptr + len;

    for (; iter < end; iter++) {
        assert_eq_char(tc, e_b, *iter);
    }
}
