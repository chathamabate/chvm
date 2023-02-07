#include "cs.h"
#include "../../core_src/io.h"
#include "../../testing_src/assert.h"

#include <stdint.h>
#include <string.h>
#include <sys/_pthread/_pthread_rwlock_t.h>

static void test_new_collected_space(chunit_test_context *tc) {
    collected_space *cs = new_collected_space_seed(1, 1, 10, 1000);

    assert_eq_uint(tc, 0, cs_count(cs));
    
    delete_collected_space(cs);
}

static const chunit_test CS_NEW = {
    .name = "Collected Space New",
    .t = test_new_collected_space,
    .timeout = 5,
};

static void test_cs_new_obj(chunit_test_context *tc) {
    collected_space *cs = new_collected_space_seed(1, 1, 10, 1000);

    const uint64_t rt_len = 5;
    const uint64_t da_size = sizeof(uint64_t);

    malloc_res res = cs_malloc_object_and_hold(cs, 5, sizeof(uint64_t));

    assert_false(tc, null_adb_addr(res.vaddr));
    assert_non_null(tc, res.paddr);

    obj_header *obj_h = res.paddr;

    assert_eq_uint(tc, rt_len, obj_h->rt_len);
    assert_eq_uint(tc, da_size, obj_h->da_size);
    cs_unlock(cs, res.vaddr);

    delete_collected_space(cs);
}

static const chunit_test CS_NEW_OBJ = {
    .name = "Collected Space New Obj",
    .t = test_cs_new_obj,
    .timeout = 5,
};

static void test_cs_new_root(chunit_test_context *tc) {
    collected_space *cs = new_collected_space_seed(1, 1, 10, 1000);

    const uint64_t roots = 8;
    cs_root_id root_ids[roots];

    uint64_t i;
    for (i = 0; i < roots; i++) {
        root_ids[i] = cs_malloc_root(cs, 1, 0);

        assert_false(tc, 
                null_adb_addr(cs_get_root_vaddr(cs, root_ids[i]).root_vaddr));
    }

    assert_eq_uint(tc, roots, cs_count(cs));

    cs_root_status_code deroot_res;
    for (i = 0; i < roots; i += 2) {
        deroot_res = cs_deroot(cs, root_ids[i]);
        root_ids[i] = UINT64_MAX;
        
        assert_eq_uint(tc, CS_SUCCESS, deroot_res);
    }

    assert_eq_uint(tc, roots, cs_count(cs));

    // cs_print(cs);

    delete_collected_space(cs);
}

static const chunit_test CS_NEW_ROOT = {
    .name = "Collected Space New Root",
    .t = test_cs_new_root,
    .timeout = 5,
};

static void test_cs_gc_0(chunit_test_context *tc) {
    collected_space *cs = new_collected_space_seed(1, 1, 10, 1000);

    const uint64_t bogus_objs = 5;

    // Make a bunch of objects, all to be GC'd.

    uint64_t i;
    for (i = 0; i < bogus_objs; i++) {
        cs_malloc_object(cs, 1, 0);
    }

    assert_eq_uint(tc, bogus_objs, cs_count(cs));

    cs_collect_garbage(cs);

    delete_collected_space(cs);
}

static const chunit_test CS_GC_0 = {
    .name = "Collected Space Collect Garbage 0",
    .t = test_cs_gc_0,
    .timeout = 5,
};

static void test_cs_gc_1(chunit_test_context *tc) {
    collected_space *cs = new_collected_space_seed(1, 1, 10, 1000); 

    // Simple Tree here.
    const uint64_t objs = 5;
    const uint64_t rt_len = 3;

    addr_book_vaddr vaddrs[objs];

    obj_header *obj_h;
    addr_book_vaddr *rt;
    uint64_t *da;

    uint64_t i;
    for (i = 0; i < objs; i++) {
        malloc_res res = cs_malloc_object_and_hold(cs, rt_len, sizeof(uint64_t));

        obj_h = res.paddr;
        rt = (addr_book_vaddr *)(obj_h + 1);
        da = (uint64_t *)(rt + rt_len);

        *da = i;

        cs_unlock(cs, res.vaddr);
    }

    // TODO finish this up.
    // Test comment for git.

    delete_collected_space(cs);
}

const chunit_test_suite GC_TEST_SUITE_CS = {
    .name = "Collected Space Test Suite",
    .tests = {
        &CS_NEW,
        &CS_NEW_OBJ,
        &CS_NEW_ROOT,
        &CS_GC_0,
    },
    .tests_len = 4,
};
