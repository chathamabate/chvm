#include "cs.h"
#include "../../core_src/io.h"
#include "../../testing_src/assert.h"
#include "../../core_src/sys.h"

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

typedef enum {
    CS_DIR_ROOT = 0,
    CS_DIR_EDGE,

    // All objects which are not expected to be freed
    // are automatically expected to be allocated.
    CS_DIR_EXPECT_FREED,

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

#define CS_F(obj_ind) \
{.type = CS_DIR_EXPECT_FREED, .obj_ind_1 = (obj_ind), .obj_ind_2 = UINT64_MAX}

#define CS_T() \
{.type = CS_DIR_TERMINATOR, .obj_ind_1 = UINT64_MAX, .obj_ind_2 = UINT64_MAX}

typedef struct {
    const uint64_t num_objs;

    // Length of this flexible array is not given.
    // Instead it is guaranteed to end with the terminator.
    const cs_dir instructions[];
} cs_test_blueprint;

static const uint64_t CS_TEST_SIZE_MOD = 4; 

static void run_cs_test(chunit_test_context *tc, const cs_test_blueprint *bp) {
    collected_space *cs = new_collected_space_seed(1, 1, 10, 1000);

    uint64_t *edges = safe_malloc(1, sizeof(uint64_t) * bp->num_objs);
    uint8_t *frees = safe_malloc(1, sizeof(uint8_t) * bp->num_objs);

    // Maybe I should make a safe calloc someday...
    uint64_t i;
    for (i = 0; i < bp->num_objs; i++) {
        edges[i] = 0;
        frees[i] = 0;
    }

    // In the first pass, we store which objects are expected to be freed,
    // as well as how many edges come out of each object.

    const cs_dir *iter;
    for (iter = bp->instructions; iter->type != CS_DIR_TERMINATOR; iter++) {
        if (iter->obj_ind_1 >= bp->num_objs && iter->obj_ind_1 != UINT64_MAX) {
            error_logf(1, 1, "run_cs_test: bad object 1 index given");
        }

        if (iter->obj_ind_2 >= bp->num_objs && iter->obj_ind_2 != UINT64_MAX) {
            error_logf(1, 1, "run_cs_test: bad object 2 index given");
        }

        if (iter->type == CS_DIR_EXPECT_FREED) {
            frees[iter->obj_ind_1] = 1;
        } else if (iter->type == CS_DIR_EDGE) {
            edges[iter->obj_ind_1]++;
        }

        // Terminator and Root directives have no effect at this
        // phase.
    }

    // Now we have enough information to actually make the graph.
    // First we must create all the objects.
    
    uint64_t da_u64_cells;
    addr_book_vaddr *vaddrs = safe_malloc(1, sizeof(addr_book_vaddr) * bp->num_objs);

    for (i = 0; i < bp->num_objs; i++) {
        da_u64_cells = (i % CS_TEST_SIZE_MOD) + 1;

        malloc_obj_res obj_res = cs_malloc_object_and_hold(cs, edges[i], 
                da_u64_cells * sizeof(uint64_t));

        uint64_t *da_u64 = (uint64_t *)(obj_res.i.da);

        // Here we store the objects number identifier in every u64 
        // cell of its data array.
        uint64_t j;        
        for (j = 0; j < da_u64_cells; j++) {
            da_u64[j] = i;
        }

        cs_unlock(cs, obj_res.vaddr);
        
        vaddrs[i] = obj_res.vaddr; 
    }

    // Now we pass over the instructions one final time to create
    // roots and edges! 
    for (iter = bp->instructions; iter->type != CS_DIR_TERMINATOR; iter++) {
        // We have already confirmed all instructions are valid!
        // Don't need to check again.

        if (iter->type == CS_DIR_ROOT) {
            cs_root(cs, vaddrs[iter->obj_ind_1]);
        } else if (iter->type == CS_DIR_EDGE) {
            obj_index i = cs_get_write_ind(cs, vaddrs[iter->obj_ind_1]);
            // Little hacky, but makes sense.
            i.rt[--(edges[iter->obj_ind_1])] = vaddrs[iter->obj_ind_2];
            cs_unlock(cs, vaddrs[iter->obj_ind_1]);
        }
    }

    // Finally, we can run GC!
    cs_collect_garbage(cs);

    // Now we run tests on each object!
    for (i = 0; i < bp->num_objs; i++) {
        if (frees[i]) {
            assert_false(tc, cs_allocated(cs, vaddrs[i]));
            continue;
        }  

        assert_true(tc, cs_allocated(cs, vaddrs[i]));

        addr_book_vaddr *refs;
        uint64_t refs_len;
    
        obj_index ind = cs_get_read_ind(cs, vaddrs[i]);

        refs_len = ind.h->rt_len;

        // Copy references for post checking.
        refs = safe_malloc(1, sizeof(addr_book_vaddr) * refs_len);
        memcpy(refs, ind.rt, sizeof(addr_book_vaddr) * refs_len);

        // Check data array.
        
        uint64_t *da_u64 = (uint64_t *)(ind.da); 
        uint64_t da_u64_cells = (i % CS_TEST_SIZE_MOD) + 1;

        uint64_t da_u64_i;
        for (da_u64_i = 0; da_u64_i < da_u64_cells; da_u64_i++) {
            assert_eq_uint(tc, i, da_u64[i]);
        }

        cs_unlock(cs, vaddrs[i]);

        // Finally check that all references are valid.
        uint64_t refs_i;
        for (refs_i = 0; refs_i < refs_len; refs_i++) {
            if (!null_adb_addr(refs[refs_i])) {
                assert_true(tc, cs_allocated(cs, refs[refs_i]));
            }
        }
    }
    
    safe_free(vaddrs);
    safe_free(edges);
    safe_free(frees);

    delete_collected_space(cs);
}

// No objects at all in the space.
static const cs_test_blueprint TEST_CS_GC_0_BP = {
    .num_objs = 0,
    .instructions = {
        CS_T()
    },
};


static void test_cs_gc_0(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_0_BP);
}

static const chunit_test CS_GC_0 = {
    .name = "Collected Space Collect Garbage 0",
    .t = test_cs_gc_0,
    .timeout = 5,
};

// One non-root object.
static const cs_test_blueprint TEST_CS_GC_1_BP = {
    .num_objs = 1,
    .instructions = {
        CS_F(0),

        CS_T()
    },
};

static void test_cs_gc_1(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_1_BP);
}

static const chunit_test CS_GC_1 = {
    .name = "Collected Space Collect Garbage 1",
    .t = test_cs_gc_1,
    .timeout = 5,
};

// Three non root objects.
static const cs_test_blueprint TEST_CS_GC_2_BP = {
    .num_objs = 3,
    .instructions = {
        CS_F(0), CS_F(1), CS_F(2),

        CS_T()
    },
};

static void test_cs_gc_2(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_2_BP);
}

static const chunit_test CS_GC_2 = {
    .name = "Collected Space Collect Garbage 2",
    .t = test_cs_gc_2,
    .timeout = 5,
};

const chunit_test_suite GC_TEST_SUITE_CS = {
    .name = "Collected Space Test Suite",
    .tests = {
        &CS_NEW,
        &CS_NEW_OBJ,
        &CS_NEW_ROOT,
        &CS_GC_0,
        &CS_GC_1,

        &CS_GC_2,
    },
    .tests_len = 6,
};
