#include "./virt.h"
#include "../virt.h"
#include "../../testing_src/assert.h"
#include "../../core_src/sys.h"
#include "../../core_src/io.h"
#include "../../core_src/thread.h"
#include "../../util_src/thread.h"
#include <string.h>

static void test_new_addr_table(chunit_test_context *tc) {
    addr_table *adt = new_addr_table(1, 5);
    delete_addr_table(adt);
}

static const chunit_test ADT_NEW_ADDR_TABLE = {
    .name = "New Address Table",
    .t = test_new_addr_table,
    .timeout = 5,
};

// Should only really check outward facing functionality...

static void test_adt_put_and_get(chunit_test_context *tc) {
    const uint64_t table_len = 4;
    addr_table *adt = new_addr_table(1, table_len);

    uint64_t ptable[table_len];

    uint64_t i;
    for (i = 0; i < table_len; i++) {
        addr_table_put_res put_res = adt_put(adt, ptable + i);
        
        // Consider adding not equal at some point...
        assert_eq_uint(tc, i == table_len - 1 ? ADT_NEWLY_FULL : ADT_SUCCESS, 
                put_res.code);
        assert_true(tc, put_res.index < table_len);

        void *ptr = adt_get_read(adt, put_res.index);
        adt_unlock(adt, put_res.index);

        // Looks good so far!
        assert_eq_ptr(tc, ptable + i, ptr);
    }

    // Final put here...
    addr_table_put_res put_err = adt_put(adt, NULL);
    assert_eq_uint(tc, ADT_NO_SPACE, put_err.code);

    delete_addr_table(adt);
}

static const chunit_test ADT_PUT_AND_GET = {
    .name = "Address Table Put and Get",
    .t = test_adt_put_and_get,
    .timeout = 5,
};

static void test_adt_free(chunit_test_context *tc) {
    const uint64_t table_len = 4;
    addr_table *adt = new_addr_table(1, table_len);

    uint64_t vaddrs[table_len];

    uint64_t i;
    for (i = 0; i < table_len; i++) {
        addr_table_put_res put_res = adt_put(adt, NULL);
        vaddrs[i] = put_res.index;        
    }

    const uint64_t freers_len = 3;
    uint64_t freers[freers_len] = {
        0, 3, 1
    };

    for (i = 0; i < freers_len; i++) {
        addr_table_code free_res = adt_free(adt, freers[i]);
        assert_eq_uint(tc, i == 0 ? ADT_NEWLY_FREE : ADT_SUCCESS, free_res);
    }

    for (i = 0; i < freers_len; i++) {
        addr_table_put_res put_res = adt_put(adt, NULL);
        assert_eq_uint(tc, i == freers_len - 1 ? ADT_NEWLY_FULL : ADT_SUCCESS, 
                put_res.code);

        assert_true(tc, put_res.index < table_len);
    }

    delete_addr_table(adt);
}

static const chunit_test ADT_FREE = {
    .name = "Address Table Free",
    .t = test_adt_free,
    .timeout = 5,
};

typedef struct {
    chunit_test_context *tc;

    uint64_t num_puts;
    addr_table *adt;

    pthread_mutex_t mut;
    uint8_t *index_vector;
} test_adt_context;

static test_adt_context *new_test_adt_context(uint8_t chnl, chunit_test_context *tc, 
        uint64_t num_puts, uint64_t cap) {
    
    test_adt_context *tac = safe_malloc(chnl, sizeof(test_adt_context));

    tac->tc = tc;

    tac->num_puts = num_puts;
    tac->adt = new_addr_table(chnl, cap);
    
    safe_mutex_init(&(tac->mut), NULL);
    tac->index_vector = safe_malloc(chnl, sizeof(uint8_t) * cap);

    uint64_t i;
    for (i = 0; i < cap; i++) {
        tac->index_vector[i] = 0;
    }

    return tac;
}

static void delete_test_adt_context(test_adt_context *tac) {
    delete_addr_table(tac->adt);

    safe_mutex_destroy(&(tac->mut));
    safe_free(tac->index_vector);

    safe_free(tac);
}

static void *test_adt_T0(void *arg) {
    util_thread_spray_context *s_context = arg;
    test_adt_context *context = s_context->context;
    
    uint8_t repeat;
    uint64_t i; 
    for (i = 0; i < context->num_puts; i++) {
        addr_table_put_res p_res = adt_put(context->adt, NULL);

        // This confirms that there is space for every ptr 
        // put into the adt.
        assert_false(context->tc, p_res.code == ADT_NO_SPACE);

        // This confirms the returned index is valid.
        assert_true(context->tc, p_res.index < adt_get_cap(context->adt));

        // This confirms the returned index is not already in use.
        safe_mutex_lock(&(context->mut));
        repeat = context->index_vector[p_res.index];
        context->index_vector[p_res.index] = 1;
        safe_mutex_unlock(&(context->mut));

        assert_false(context->tc, repeat);

        // safe_printf("Thread %lu Given Index %lu\n", s_context->index, p_res.index);
    }

    return NULL;
}

static void test_adt_multi0(chunit_test_context *tc) {
    const uint64_t num_puts = 20;
    const uint64_t num_threads = 10;

    const uint64_t adt_cap = num_puts * num_threads;

    test_adt_context *tac = new_test_adt_context(1, tc, num_puts, num_puts * num_threads);

    util_thread_spray_info *spray = 
        util_thread_spray(1, num_threads, test_adt_T0, tac);

    util_thread_collect(spray);

    assert_eq_uint(tc, ADT_NO_SPACE, adt_put(tac->adt, NULL).code);

    delete_test_adt_context(tac);
}

static const chunit_test ADT_MULTI0 = {
    .name = "Address Table Multi-Threaded 0",
    .t = test_adt_multi0,
    .timeout = 5,
};

// This does a bunch of put the free pairs. 
static void *test_adt_T1(void *arg) {
    util_thread_spray_context *s_context = arg;
    test_adt_context *context = s_context->context; 

    // Dummy physical addresses.
    uint64_t *data = safe_malloc(1, sizeof(uint64_t) * context->num_puts);

    uint64_t i;
    for (i = 0; i < context->num_puts; i++) {
        addr_table_put_res p_res = adt_put(context->adt, data + i); 

        if (p_res.code == ADT_NO_SPACE) {
            continue;
        }

        uint8_t repeat;
        safe_mutex_lock(&(context->mut));
        repeat = context->index_vector[p_res.index];
        context->index_vector[p_res.index] = 1; // Mark as used.
        safe_mutex_unlock(&(context->mut));

        // Confirm there wasn't a repeat.
        assert_false(context->tc, repeat);

        // Get physical pointer stored in the ADT.
        uint64_t *paddr = adt_get_write(context->adt, p_res.index);
        adt_unlock(context->adt, p_res.index);
        
        // Confirm the correct pointer was stored.
        assert_eq_ptr(context->tc, data + i, paddr);

        safe_mutex_lock(&(context->mut));
        context->index_vector[p_res.index] = 0; // Mark as unused.
        safe_mutex_unlock(&(context->mut));

        adt_free(context->adt, p_res.index);
    }

    safe_free(data);

    return NULL;
}

static void test_adt_multi1(chunit_test_context *tc) {
    const uint64_t num_puts = 40;
    const uint64_t num_threads = 10;

    const uint64_t adt_cap = num_puts * num_threads;

    test_adt_context *tac = new_test_adt_context(1, tc, num_puts, num_puts * num_threads);

    util_thread_spray_info *spray = 
        util_thread_spray(1, num_threads, test_adt_T1, tac);

    util_thread_collect(spray);

    assert_eq_uint(tc, 0, adt_get_fill(tac->adt));

    delete_test_adt_context(tac);
}

static const chunit_test ADT_MULTI1 = {
    .name = "Address Table Multi-Threaded 1",
    .t = test_adt_multi1,
    .timeout = 5,
};

static void test_adt_move(chunit_test_context *tc) {
    addr_table *adt = new_addr_table(1, 5);

    const char *message = "Hello World My Friend";

    uint8_t buff[70];
    uint8_t *paddr1 = buff + sizeof(addr_book_vaddr);
    uint8_t *paddr2 = paddr1 + 10;

    addr_table_put_res res = adt_put(adt, paddr1);
    assert_false(tc, res.code == ADT_NO_SPACE);

    uint64_t cell_ind = res.index;

    char *paddr = adt_get_write(adt, cell_ind);
    assert_eq_ptr(tc, paddr1, paddr);

    strcpy(paddr, message); 

    adt_unlock(adt, cell_ind);

    // Do the move.
    adt_move_p(1, adt, cell_ind, paddr2, 
            strlen(message) + 1, 1, 0);

    paddr = adt_get_read(adt, cell_ind); 

    // Confirm physical address was correctly changed.
    assert_eq_ptr(tc, paddr2, paddr);

    // Confirm string was copied correctly.
    assert_eq_str(tc, message, paddr);

    adt_unlock(adt, cell_ind);

    delete_addr_table(adt);
}

const chunit_test ADT_MOVE = {
    .name = "Address Table Move",
    .t = test_adt_move,
    .timeout = 5,
};

static void test_adt_bad_index(chunit_test_context *tc) {
    addr_table *adt = new_addr_table(1, 1);

    // Out of bounds index here.
    adt_free(adt, 1);

    delete_addr_table(adt);
}

const chunit_test ADT_BAD_INDEX = {
    .name = "Address Table Bad Index",
    .t = test_adt_bad_index,
    .timeout = 5,
    .should_fail = 1,
};

static void test_adt_unallocated_index(chunit_test_context *tc) {
    addr_table *adt = new_addr_table(1, 1);

    // Freeing an unallocated index.
    adt_free(adt, 0);

    delete_addr_table(adt);
}

const chunit_test ADT_UNALLOCATED_INDEX = {
    .name = "Address Table Unallocated Index",
    .t = test_adt_unallocated_index,
    .timeout = 5,
    .should_fail = 1,
};

static void test_adt_try_lock(chunit_test_context *tc) {
    addr_table *adt = new_addr_table(1, 1);

    addr_table_put_res res = adt_put(adt, (void *)1);
    assert_false(tc, ADT_NO_SPACE == res.code);

    adt_get_write(adt, res.index);

    // We should not be able to get our lock here.
    assert_eq_ptr(tc, NULL, adt_try_get_write(adt, res.index));

    adt_unlock(adt, res.index);

    delete_addr_table(adt);
}

const chunit_test ADT_TRY_LOCK = {
    .name = "Address Table Try Lock",
    .t = test_adt_try_lock,
    .timeout = 5
};

static void test_adt_put_and_hold(chunit_test_context *tc) {
    addr_table *adt = new_addr_table(1, 5);    
    uint64_t i = 0;

    addr_table_put_res res = adt_put_and_hold(adt, &i);
    assert_false(tc, ADT_NO_SPACE == res.code);
    assert_eq_ptr(tc, NULL, adt_try_get_read(adt, res.index));

    adt_unlock(adt, res.index);

    delete_addr_table(adt);
}

const chunit_test ADT_PUT_AND_HOLD = {
    .name = "Address Table Put and Hold",
    .t = test_adt_put_and_hold,
    .timeout = 5,
};

const chunit_test_suite GC_TEST_SUITE_ADT = {
    .name = "Address Table Test Suite",
    .tests = {
        &ADT_NEW_ADDR_TABLE,
        &ADT_PUT_AND_GET,
        &ADT_FREE,
        &ADT_MULTI0,
        &ADT_MULTI1,

        &ADT_MOVE,
        &ADT_BAD_INDEX,
        &ADT_UNALLOCATED_INDEX,
        &ADT_TRY_LOCK,
        &ADT_PUT_AND_HOLD,
    },
    .tests_len = 10,
};

// Address Book Suite Below.

static void test_new_addr_book(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 10);
    delete_addr_book(adb);
}

static const chunit_test ADB_NEW_ADDR_BOOK = {
    .name = "New Address Book",
    .t = test_new_addr_book,
    .timeout = 5
};

static void test_adb_put_and_get(chunit_test_context *tc) {
    const uint64_t tables = 16; // Should test a good amount of
                                // expansions.
    const uint64_t table_cap = 4;

    uint64_t table, ind;

    addr_book *adb = new_addr_book(1, table_cap);
    
    void *paddrs[tables][table_cap];
    addr_book_vaddr vaddrs[tables][table_cap];

    for (table = 0; table < tables; table++) {
        for (ind = 0; ind < table_cap; ind++) {
            // By having a unique physical address for each put
            // we are confirming all puts return a unique address.
            paddrs[table][ind] = (void *)((table * table_cap) + ind);
            vaddrs[table][ind] = adb_put(adb, paddrs[table][ind]);
        }        
    }

    for (table = 0; table < tables; table++) {
        for (ind = 0; ind < table_cap; ind++) {
            assert_eq_ptr(tc, paddrs[table][ind], 
                    adb_get_read(adb, vaddrs[table][ind]));
            adb_unlock(adb, vaddrs[table][ind]);
        }        
    }

    delete_addr_book(adb);
}

static const chunit_test ADB_PUT_AND_GET = {
    .name = "Address Book Put and Get",
    .t = test_adb_put_and_get,
    .timeout = 5
};

static void test_adb_free(chunit_test_context *tc) {
    const uint64_t table_cap = 5;
    const uint64_t tables = 5;
    const uint64_t num_puts = table_cap * tables;
    const uint64_t frees = 10; 

    addr_book *adb = new_addr_book(1, table_cap);

    addr_book_vaddr vaddrs[num_puts];

    uint64_t i;
    for (i = 0; i < num_puts; i++) {
        vaddrs[i] = adb_put(adb, NULL);
    }

    // Confirm all puts executed correctly.
    assert_eq_uint(tc, num_puts, adb_get_fill(adb));

    for (i = 0; i < frees; i++) {
        adb_free(adb, vaddrs[i]);
    }

    // Assert frees happened successfully.
    assert_eq_uint(tc, num_puts - frees, adb_get_fill(adb));

    // Now put for each free.
    for (i = 0; i < frees; i++) {
        addr_book_vaddr vaddr = adb_put(adb, (void *)i);

        // Reuse vaddrs here.
        vaddrs[i] = vaddr;
    }

    assert_eq_uint(tc, num_puts, adb_get_fill(adb));

    // Confirm puts occured successfully after frees.
    for (i = 0; i < frees; i++) {
        void *paddr = adb_get_write(adb, vaddrs[i]); 
        adb_unlock(adb, vaddrs[i]);

        assert_eq_uint(tc, i, (uint64_t)paddr);
    }

    delete_addr_book(adb);
}

static const chunit_test ADB_FREE = {
    .name = "Address Book Free",
    .t = test_adb_free,
    .timeout = 5,
};

// NOTE: multithreaded testing of the address book!
// We will do two multi threaded tests, one with just puts,
// and one with frees.
//
// This will not use a repeat vector like the ADT tests since
// I've begun to think this is a bit redundant.
// Maybe I can change the ADT test code at somepoint.

typedef struct {
    chunit_test_context *tc;

    addr_book *adb;
    uint64_t num_puts;

    // Whether or not the test should execute
    // frees as well as puts.
    uint8_t with_free;
} test_adb_context;

// This will be the just puts try.
static void *test_adb_T(void *arg) {
    util_thread_spray_context *s_context = arg;
    test_adb_context *context = s_context->context;

    chunit_test_context *tc = context->tc;
    addr_book *adb = context->adb;
    const uint64_t num_puts = context->num_puts;

    // Dummy data array.
    uint8_t *data = safe_malloc(1, sizeof(uint8_t) * num_puts);

    uint64_t i;
    for (i = 0; i < num_puts; i++) {
        addr_book_vaddr vaddr = adb_put(adb, data + i); 

        void *paddr = adb_get_read(adb, vaddr);
        adb_unlock(adb, vaddr);

        assert_eq_ptr(tc, data + i, paddr);

        if (context->with_free) {
            adb_free(adb, vaddr);
        }
    }

    safe_free(data);

    return NULL;
}

static void test_adb_multi0(chunit_test_context *tc) {
    const uint64_t num_threads = 4;
    const uint64_t table_cap = 5;
    const uint64_t num_puts = 3;

    addr_book *adb = new_addr_book(1, table_cap);

    test_adb_context ctx = {
        .tc = tc,

        .adb = adb,
        .num_puts = num_puts,

        .with_free = 0,
    };

    util_thread_spray_info *spray = 
        util_thread_spray(1, num_threads, test_adb_T, &ctx);
    util_thread_collect(spray);

    assert_eq_uint(tc, num_puts * num_threads, adb_get_fill(adb));

    delete_addr_book(adb);
}

static const chunit_test ADB_MULTI0 = {
    .name = "Address Book Multi-Threaded 0",
    .t = test_adb_multi0,
    .timeout = 5,
};

static void test_adb_multi1(chunit_test_context *tc) {
    const uint64_t num_threads = 10;
    const uint64_t table_cap = 6;
    const uint64_t num_puts = 120;

    addr_book *adb = new_addr_book(1, table_cap);

    test_adb_context ctx = {
        .tc = tc,

        .adb = adb,
        .num_puts = num_puts,

        .with_free = 1,
    };

    util_thread_spray_info *spray = 
        util_thread_spray(1, num_threads, test_adb_T, &ctx);
    util_thread_collect(spray);

    assert_eq_uint(tc, 0, adb_get_fill(adb));

    delete_addr_book(adb);
}

static const chunit_test ADB_MULTI1 = {
    .name = "Address Book Multi-Threaded 1",
    .t = test_adb_multi1,
    .timeout = 5,
};

static void test_adb_move(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 5);

    // The main functionality is tested in the 
    // adt version of this test.
    
    uint8_t buff[5] = {
        1, 2, 3, 4, 5
    };

    uint8_t *paddr1 = buff + 2; // 3, 4, 5
    uint8_t *paddr2 = buff;

    addr_book_vaddr vaddr = adb_put(adb, paddr1);
    adb_move(adb, vaddr, paddr2, 3);

    uint8_t *paddr = adb_get_read(adb, vaddr);
    assert_eq_ptr(tc, paddr2, paddr);

    // Pretty elementary test that bytes were copied 
    // correctly.
    assert_eq_uint(tc, 3, paddr[0]);
    assert_eq_uint(tc, 4, paddr[1]);
    assert_eq_uint(tc, 5, paddr[2]);

    adb_unlock(adb, vaddr);

    delete_addr_book(adb);
}

static const chunit_test ADB_MOVE = {
    .name = "Address Book Move Test",
    .t = test_adb_move,
    .timeout = 5,
};

static void test_adb_misc_0(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 3);

    const uint64_t free_mod = 2;
    const uint64_t num_puts = 15;

    addr_book_vaddr vaddrs[num_puts];

    uint64_t i;
    for (i = 0; i < num_puts; i++) {
        vaddrs[i] = adb_put(adb, (void *)i);
    }

    for (i = 0; i < num_puts; i += free_mod) {
        adb_free(adb, vaddrs[i]);
    }

    for (i = 0; i < num_puts; i += free_mod) {
        vaddrs[i] = adb_put(adb, (void *)i);
    }

    delete_addr_book(adb);
}

static const chunit_test ADB_MISC_0 = {
    .name = "Address Book Misc Test 0",
    .t = test_adb_misc_0,
    .timeout = 5,
};

static void test_adb_bad_index(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 5);
    adb_put(adb, NULL);

    addr_book_vaddr v = {
        .cell_index = 0,
        .table_index = 1,
    };

    // There should be no index 1 table.
    adb_get_read(adb, v);

    delete_addr_book(adb);
}

static const chunit_test ADB_BAD_INDEX = {
    .name = "Address Book Bad Index",
    .t = test_adb_bad_index,
    .timeout = 5,
    .should_fail = 1,
};

static void test_adb_try_lock(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 5);

    addr_book_vaddr v = adb_put(adb, (void *)1);

    void *paddr = adb_get_write(adb, v);
    assert_eq_ptr(tc, (void *)1, paddr);

    assert_eq_ptr(tc, NULL, adb_try_get_read(adb, v));

    adb_unlock(adb, v);

    delete_addr_book(adb);
}

static const chunit_test ADB_TRY_LOCK = {
    .name = "Address Book Try Lock",
    .t = test_adb_try_lock,
    .timeout = 5,
};

static void test_adb_put_and_hold(chunit_test_context *tc) {
    addr_book *adb = new_addr_book(1, 5);
    int i = 0;

    addr_book_vaddr vaddr = adb_put_and_hold(adb, &i);
    assert_eq_ptr(tc, NULL, adb_try_get_read(adb, vaddr));

    adb_unlock(adb, vaddr);

    delete_addr_book(adb);
}

static const chunit_test ADB_PUT_AND_HOLD = {
    .name = "Address Book Put and Hold",
    .t = test_adb_put_and_hold,
    .timeout = 5,
};

const chunit_test_suite GC_TEST_SUITE_ADB = {
    .name = "Address Book Test Suite",
    .tests = {
        &ADB_NEW_ADDR_BOOK,
        &ADB_PUT_AND_GET,
        &ADB_FREE,
        &ADB_MULTI0,
        &ADB_MULTI1,

        &ADB_MOVE,
        &ADB_MISC_0,
        &ADB_BAD_INDEX,
        &ADB_TRY_LOCK,
        &ADB_PUT_AND_HOLD,
    },
    .tests_len = 10
};

