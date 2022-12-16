#include "./virt.h"
#include "../virt.h"
#include "../../testing_src/assert.h"
#include "../../core_src/sys.h"
#include "../../core_src/io.h"
#include "../../core_src/thread.h"
#include "../../util_src/thread.h"

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

    delete_test_adt_context(tac);
}

static const chunit_test ADT_MULTI1 = {
    .name = "Address Table Multi-Threaded 1",
    .t = test_adt_multi1,
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
    },
    .tests_len = 5
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
    const uint64_t table_cap = 2;

    uint64_t table, ind;

    addr_book *adb = new_addr_book(1, table_cap);
    
    void *paddrs[tables][table_cap];
    addr_book_vaddr vaddrs[tables][table_cap];

    for (table = 0; table < tables; table++) {
        for (ind = 0; ind < table_cap; ind++) {
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
    const uint64_t table_cap = 2;
    const uint64_t tables = 2;

    uint8_t repeat_vector[tables][table_cap] = {
        {0, 0}, {0, 0}
    };

    addr_book *adb = new_addr_book(1, table_cap);

    uint64_t i;
    for (i = 0; i < table_cap * tables; i++) {
        adb_put(adb, NULL);
    }

    // This is poor testing right here!
    const uint64_t vaddrs_len = 3;
    addr_book_vaddr vaddrs[vaddrs_len] = {
        {.table_index = 1, .cell_index = 0}, 
        {.table_index = 0, .cell_index = 1}, 
        {.table_index = 1, .cell_index = 1}, 
    };

    for (i = 0; i < vaddrs_len; i++) {
        adb_free(adb, vaddrs[i]);
    }

    // The new given addresses must all be valid
    // and unique. Beautiful.
    for (i = 0; i < vaddrs_len; i++) {
        addr_book_vaddr vaddr = adb_put(adb, NULL);
        assert_true(tc, vaddr.table_index < tables);
        assert_true(tc, vaddr.cell_index < table_cap);

        assert_false(tc, repeat_vector[vaddr.table_index][vaddr.cell_index]);
        repeat_vector[vaddr.table_index][vaddr.cell_index] = 1;
    }

    delete_addr_book(adb);
}

static const chunit_test ADB_FREE = {
    .name = "Address Book Free",
    .t = test_adb_free,
    .timeout = 5,
};

// TODO: multithreaded testing of the address book!

const chunit_test_suite GC_TEST_SUITE_ADB = {
    .name = "Address Book Test Suite",
    .tests = {
        &ADB_NEW_ADDR_BOOK,
        &ADB_PUT_AND_GET,
        &ADB_FREE,
    },
    .tests_len = 3
};

