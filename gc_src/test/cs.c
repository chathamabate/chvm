#include "cs.h"
#include "../../core_src/io.h"
#include "../../testing_src/assert.h"

#include <cstdint>
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

    malloc_obj_res mor = cs_malloc_object_and_hold(cs, 5, sizeof(uint64_t));

    assert_false(tc, null_adb_addr(mor.vaddr));
    assert_non_null(tc, mor.i.h);

    obj_header *obj_h = mor.i.h;

    assert_eq_uint(tc, rt_len, obj_h->rt_len);
    assert_eq_uint(tc, da_size, obj_h->da_size);
    cs_unlock(cs, mor.vaddr);

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

// TODO : find a better way to craete colleccted spaces....
// How to represent a graph best in static constants?
//
// Each node has neighbors.
// There are those expected to be collected.
// and those not expected to be collected.
// Think of a better way soon.

typedef enum {
    CS_DIR_ROOT = 0,
    CS_DIR_EDGE,
    CS_DIR_EXPECT_ALLOCATED,
    CS_DIR_EXPECT_FREE,
    CS_DIR_TERMINATOR,
} cs_dir_code;

typedef struct {
    cs_dir_code type;

    uint64_t obj_ind_1;
    uint64_t obj_ind_2;
} cs_dir;

#define CS_R(root_ind) \
{.type = CS_DIR_ROOT, .obj_ind_1 = (root_ind), .obj_ind_2 = UINT64_MAX}

#define CS_E(from_ind, to_ind) \
{.type = CS_DIR_EDGE, .obj_ind_1 = (from_ind), .obj_ind_2 = (to_ind)}

#define CS_A(obj_ind) \
{.type = CS_DIR_EXPECT_ALLOCATED, .obj_ind_1 = (obj_ind), .obj_ind_2 = UINT64_MAX}

#define CS_F(obj_ind) \
{.type = CS_DIR_EXPECT_FREE, .obj_ind_1 = (obj_ind), .obj_ind_2 = UINT64_MAX}

#define CS_T() \
{.type = CS_DIR_TERMINATOR, .obj_ind_1 = UINT64_MAX, .obj_ind_2 = UINT64_MAX}

typedef struct {
    const uint64_t num_objs;

    // Length of this flexible array is not given.
    // Instead it is guaranteed to end with the terminator.
    const cs_dir instructions[];
} cs_test_blueprint;

static const cs_test_blueprint TEST_BP_0 = {
    .num_objs = 3,
    .instructions = {
        CS_F(0), CS_F(1), CS_F(2),

        CS_T()
    },
};

static void test_cs_gc_0(chunit_test_context *tc) {
    collected_space *cs = new_collected_space_seed(1, 1, 10, 1000);

    const uint64_t bogus_objs = 5;
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

static collected_space *new_standard_cs(addr_book_vaddr *vaddrs, uint64_t objs, 
        uint64_t rt_len, uint64_t da_size) {
    collected_space *cs = new_collected_space_seed(1, 1, 10, 1000); 
    
    uint64_t i;
    for (i = 0; i < objs; i++) {
        vaddrs[i] = cs_malloc_object(cs, rt_len, da_size);
    }

    return cs;
}

static void test_cs_gc_1(chunit_test_context *tc) {
    const uint64_t objs = 5;
    addr_book_vaddr vaddrs[objs];
    
    collected_space *cs = new_standard_cs(vaddrs, objs, 2, 0);

    // Test Case :
    //
    // Root(0);
    // 0 -> {1, 2};
    // 3 -> {4};
    // 
    // 3, 4 should be GC'd
    
    obj_index ind;

    cs_root_id root_id = cs_root(cs, vaddrs[0]);

    ind = cs_get_write_ind(cs, vaddrs[0]);
    ind.rt[0] = vaddrs[1];
    ind.rt[1] = vaddrs[2];
    cs_unlock(cs, vaddrs[0]);

    ind = cs_get_write_ind(cs, vaddrs[3]);
    ind.rt[0] = vaddrs[4];
    cs_unlock(cs, vaddrs[3]);

    cs_collect_garbage(cs);

    assert_eq_uint(tc, 3, cs_count(cs));

    // cs_print(cs);
    //
    delete_collected_space(cs);
}

static const chunit_test CS_GC_1 = {
    .name = "Collected Space Collect Garbage 1",
    .t = test_cs_gc_1,
    .timeout = 5,
};

static void test_cs_gc_2(chunit_test_context *tc) {
    const uint64_t objs = 6;
    addr_book_vaddr vaddrs[objs];
    collected_space *cs = new_standard_cs(vaddrs, objs, 2, 0);

    // Test Case :
    //
    // Root(0, 1);
    // 0 -> {2};
    // 1 -> {2};
    // 3 -> {4, 5};
    // 
    // 3, 4, 5 should be GC'd

    obj_index ind;

    delete_collected_space(cs);
}

const chunit_test_suite GC_TEST_SUITE_CS = {
    .name = "Collected Space Test Suite",
    .tests = {
        &CS_NEW,
        &CS_NEW_OBJ,
        &CS_NEW_ROOT,
        &CS_GC_0,
        &CS_GC_1,
    },
    .tests_len = 5,
};
