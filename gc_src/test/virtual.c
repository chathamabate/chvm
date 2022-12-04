
#include "./virtual.h"
#include "../virtual.h"
#include "../../testing_src/assert.h"

static void test_new_addr_table(int pipe_fd) {
    addr_table *adt = new_addr_table(1, 5);
    delete_addr_table(adt);
}

static const chunit_test ADT_NEW_ADDR_TABLE = {
    .name = "New Address Table",
    .t = test_new_addr_table,
    .timeout = 5,
};

static void test_adt_put_and_get(int pipe_fd) {
    const uint64_t table_len = 4;
    addr_table *adt = new_addr_table(1, table_len);

    uint64_t ptable[table_len];

    uint64_t i;
    for (i = 0; i < table_len; i++) {
        assert_eq_uint(pipe_fd, i, adt_put(adt, ptable + i));
        assert_eq_ptr(pipe_fd, ptable + i, adt_get(adt, i));
    }

    assert_true(pipe_fd, adt_is_full(adt));

    delete_addr_table(adt);
}

static const chunit_test ADT_PUT_AND_GET = {
    .name = "Address Table Put and Get",
    .t = test_adt_put_and_get,
    .timeout = 5,
};

static void test_adt_set(int pipe_fd) {
    const uint64_t table_len = 4;
    addr_table *adt = new_addr_table(1, table_len);

    uint64_t ptable[table_len];

    uint64_t i;
    for (i = 0; i < table_len; i++) {
        adt_put(adt, ptable + i);
    }

    adt_set(adt, 0, ptable + 1);
    assert_eq_ptr(pipe_fd, ptable + 1, adt_get(adt, 0));

    adt_set(adt, 3, ptable);
    assert_eq_ptr(pipe_fd, ptable, adt_get(adt, 3));
    
    delete_addr_table(adt);
}

static const chunit_test ADT_SET = {
    .name = "Address Table Set",
    .t = test_adt_set,
    .timeout = 5,
};

static void test_adt_free(int pipe_fd) {
    const uint64_t table_len = 4;
    addr_table *adt = new_addr_table(1, table_len);

    uint64_t i;
    for (i = 0; i < table_len; i++) {
        adt_put(adt, NULL);
    }

    adt_free(adt, 3);
    adt_free(adt, 0);
    adt_free(adt, 1);

    assert_eq_int(pipe_fd, 1, adt_put(adt, NULL));
    assert_eq_int(pipe_fd, 0, adt_put(adt, NULL));
    assert_eq_int(pipe_fd, 3, adt_put(adt, NULL));

    assert_false(pipe_fd, adt_has_next(adt));

    delete_addr_table(adt);
}

static const chunit_test ADT_FREE = {
    .name = "Address Table Free",
    .t = test_adt_free,
    .timeout = 5,
};

const chunit_test_suite GC_TEST_SUITE_ADT = {
    .name = "Address Table Test Suite",
    .tests = {
        &ADT_NEW_ADDR_TABLE,
        &ADT_PUT_AND_GET,
        &ADT_SET,
        &ADT_FREE,
    },
    .tests_len = 4
};

// Address Book Suite Below.

static void test_new_addr_book(int pipe_fd) {
    addr_book *adb = new_addr_book(1, 10, 10);
    delete_addr_book(adb);
}

static const chunit_test ADB_NEW_ADDR_BOOK = {
    .name = "New Address Book",
    .t = test_new_addr_book,
    .timeout = 5
};

static void test_adb_put_and_get(int pipe_fd) {
    const uint64_t init_cap = 2;
    const uint64_t table_cap = 2;

    addr_book *adb = new_addr_book(1, init_cap, table_cap);

    // Here we are pseudo testing auto expansion of the 
    // address book.
    
    uint64_t table, ind;
    void *paddr;

    for (table = 0; table < 3; table++) {
        for (ind = 0; ind < table_cap; ind++) {
            void *paddr = (void *)((table * table_cap) + ind);

            addr_book_lookup vaddr = adb_put(adb, paddr);
            
            // NOTE: 
            // This is somewhat questionable testing.
            // We are assuming we know the exact inner workings
            // of the adb (that slots will be given out consecutively)
            // This is not a requirement of the data structure.
            //
            // However, to ensure the specifics of the code I wrote 
            // work, I am OK with this.
            
            assert_eq_uint(pipe_fd, table, vaddr.table);
            assert_eq_uint(pipe_fd, ind, vaddr.index);

            assert_eq_ptr(pipe_fd, paddr, adb_get(adb, vaddr));
        }        
    }

    delete_addr_book(adb);
}

static const chunit_test ADB_PUT_AND_GET = {
    .name = "Address Book Put and Get",
    .t = test_adb_put_and_get,
    .timeout = 5
};

static void test_adb_set(int pipe_fd) {
    const uint64_t init_cap = 2;
    const uint64_t table_cap = 2;

    addr_book *adb = new_addr_book(1, init_cap, table_cap);

    void *paddrs[] = {
        (void *)54, (void *)23, (void *)10 // Dummy addresses.
    };

    adb_put(adb, paddrs[0]);
    addr_book_lookup vaddr = adb_put(adb, paddrs[1]);
    adb_put(adb, paddrs[2]);

    assert_eq_ptr(pipe_fd, paddrs[1], adb_get(adb, vaddr));

    adb_set(adb, vaddr, paddrs[0]);
    assert_eq_ptr(pipe_fd, paddrs[0], adb_get(adb, vaddr));

    delete_addr_book(adb);
}

static const chunit_test ADB_SET = {
    .name = "Address Book Set",
    .t = test_adb_set,
    .timeout = 5,
};

static void test_adb_free(int pipe_fd) {
    const uint64_t init_cap = 2;
    const uint64_t table_cap = 2;

    addr_book *adb = new_addr_book(1, init_cap, table_cap);

    uint64_t i;
    for (i = 0; i < 4; i++) {
        adb_put(adb, NULL);
    }

    const uint64_t vaddrs_len = 3;
    addr_book_lookup vaddrs[vaddrs_len] = {
        {.table = 1, .index = 0}, 
        {.table = 0, .index = 1}, 
        {.table = 1, .index = 1}, 
    };

    for (i = 0; i < vaddrs_len; i++) {
        adb_free(adb, vaddrs[i]);
    }

    // Again, these tests take advantage of the fact
    // that I know exactly how slots are freed.
    
    addr_book_lookup new_vaddr0 = adb_put(adb, NULL);
    assert_eq_uint(pipe_fd, 0, new_vaddr0.table);
    assert_eq_uint(pipe_fd, 1, new_vaddr0.index);

    addr_book_lookup new_vaddr1 = adb_put(adb, NULL);
    assert_eq_uint(pipe_fd, 1, new_vaddr1.table);
    assert_eq_uint(pipe_fd, 1, new_vaddr1.index);

    addr_book_lookup new_vaddr2 = adb_put(adb, NULL);
    assert_eq_uint(pipe_fd, 1, new_vaddr2.table);
    assert_eq_uint(pipe_fd, 0, new_vaddr2.index);

    delete_addr_book(adb);
}

static const chunit_test ADB_FREE = {
    .name = "Address Book Free",
    .t = test_adb_free,
    .timeout = 5,
};

const chunit_test_suite GC_TEST_SUITE_ADB = {
    .name = "Address Book Test Suite",
    .tests = {
        &ADB_NEW_ADDR_BOOK,
        &ADB_PUT_AND_GET,
        &ADB_SET,
        &ADB_FREE,
    },
    .tests_len = 4
};

