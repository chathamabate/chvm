#ifndef GC_TEST_MB_H
#define GC_TEST_MB_H

#include "../../testing_src/chunit.h"
#include "../virt.h"

void fill_unique(addr_book *adb, addr_book_vaddr vaddr, 
        uint64_t min_size);

void check_unique_vaddr_body(chunit_test_context *tc, 
        addr_book *adb, addr_book_vaddr vaddr, uint64_t min_size);

extern const chunit_test_suite GC_TEST_SUITE_MB;

#endif
