#include "cs.h"
#include "../../core_src/io.h"
#include "../../testing_src/assert.h"
#include "../../core_src/sys.h"

#include "../../util_src/thread.h"

#include <stdint.h>
#include <string.h>
#include <sys/_pthread/_pthread_rwlock_t.h>
#include <time.h>

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
    uint64_t frees_total = 0;

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
            frees_total++;
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
    uint64_t freed = cs_collect_garbage(cs);
    assert_eq_uint(tc, frees_total, freed);

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
            assert_eq_uint(tc, i, da_u64[da_u64_i]);
        }

        cs_unlock(cs, vaddrs[i]);

        // Finally check that all references are valid.
        uint64_t refs_i;
        for (refs_i = 0; refs_i < refs_len; refs_i++) {
            if (!null_adb_addr(refs[refs_i])) {
                assert_true(tc, cs_allocated(cs, refs[refs_i]));
            }
        }

        safe_free(refs);
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

static const cs_test_blueprint TEST_CS_GC_3_BP = {
    .num_objs = 2,
    .instructions = {
        CS_R(0),

        CS_F(1),

        CS_T()
    },
};

static void test_cs_gc_3(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_3_BP);
}

static const chunit_test CS_GC_3 = {
    .name = "Collected Space Collect Garbage 3",
    .t = test_cs_gc_3,
    .timeout = 5,
};

static const cs_test_blueprint TEST_CS_GC_4_BP = {
    .num_objs = 3,
    .instructions = {
        CS_R(0),
        CS_E(0, 1),

        CS_F(2),

        CS_T()
    },
};

static void test_cs_gc_4(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_4_BP);
}

static const chunit_test CS_GC_4 = {
    .name = "Collected Space Collect Garbage 4",
    .t = test_cs_gc_4,
    .timeout = 5,
};

static const cs_test_blueprint TEST_CS_GC_5_BP = {
    .num_objs = 5,
    .instructions = {
        CS_R(0),
        CS_E(0, 1),
        CS_E(0, 2),

        CS_E(3, 4),
        CS_F(3), CS_F(4),

        CS_T()
    },
};

static void test_cs_gc_5(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_5_BP);
}

static const chunit_test CS_GC_5 = {
    .name = "Collected Space Collect Garbage 5",
    .t = test_cs_gc_5,
    .timeout = 5,
};

static const cs_test_blueprint TEST_CS_GC_6_BP = {
    .num_objs = 7,
    .instructions = {
        CS_R(0),
        CS_E(0, 1),

        CS_R(2), CS_E(2, 3), CS_E(2, 4),

        CS_E(5, 4), CS_E(5, 6),

        CS_F(5), CS_F(6),

        CS_T()
    },
};

static void test_cs_gc_6(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_6_BP);
}

static const chunit_test CS_GC_6 = {
    .name = "Collected Space Collect Garbage 6",
    .t = test_cs_gc_6,
    .timeout = 5,
};

// Just a kinda complex graph here.
static const cs_test_blueprint TEST_CS_GC_7_BP = {
    .num_objs = 8,
    .instructions = {
        CS_R(0), CS_R(1),

        CS_E(0, 2), CS_E(0, 3),
        CS_E(1, 3), CS_E(1, 4),
        
        CS_E(2, 5),

        CS_E(3, 5), CS_E(3, 6),

        CS_E(4, 6),

        CS_E(5, 7), CS_E(6, 7),

        CS_T()
    },
};

static void test_cs_gc_7(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_7_BP);
}

static const chunit_test CS_GC_7 = {
    .name = "Collected Space Collect Garbage 7",
    .t = test_cs_gc_7,
    .timeout = 5,
};

// Duplicate edges!
static const cs_test_blueprint TEST_CS_GC_8_BP = {
    .num_objs = 4,
    .instructions = {
        CS_R(0),
        CS_E(0, 2), CS_E(0, 2), CS_E(0, 3),
        CS_E(1, 2),

        CS_F(1),

        CS_T()
    },
};

static void test_cs_gc_8(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_8_BP);
}

static const chunit_test CS_GC_8 = {
    .name = "Collected Space Collect Garbage 8",
    .t = test_cs_gc_8,
    .timeout = 5,
};

// Root pointing to a root.
static const cs_test_blueprint TEST_CS_GC_9_BP = {
    .num_objs = 3,
    .instructions = {
        CS_R(0), CS_R(1),

        CS_E(0, 1), CS_E(0, 2),
        CS_E(1, 2),

        CS_T()
    },
};

static void test_cs_gc_9(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_9_BP);
}

static const chunit_test CS_GC_9 = {
    .name = "Collected Space Collect Garbage 9",
    .t = test_cs_gc_9,
    .timeout = 5,
};

// Direct Cycle.
static const cs_test_blueprint TEST_CS_GC_10_BP = {
    .num_objs = 2,
    .instructions = {
        CS_R(0),
        CS_E(0, 1), CS_E(1, 0), 
        CS_T()
    },
};

static void test_cs_gc_10(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_10_BP);
}

static const chunit_test CS_GC_10 = {
    .name = "Collected Space Collect Garbage 10",
    .t = test_cs_gc_10,
    .timeout = 5,
};

// Big Cycle with extra root.
static const cs_test_blueprint TEST_CS_GC_11_BP = {
    .num_objs = 5,
    .instructions = {
        CS_R(0), CS_R(1),

        CS_E(0, 2),
        CS_E(2, 4),
        CS_E(4, 3),
        CS_E(3, 0),

        CS_E(1, 3),

        CS_T()
    },
};

static void test_cs_gc_11(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_11_BP);
}

static const chunit_test CS_GC_11 = {
    .name = "Collected Space Collect Garbage 11",
    .t = test_cs_gc_11,
    .timeout = 5,
};

// Big loop which contains multiple roots.
static const cs_test_blueprint TEST_CS_GC_12_BP = {
    .num_objs = 4,
    .instructions = {
        CS_R(0), CS_R(3),

        CS_E(0, 1), CS_E(1, 3),
        CS_E(3, 2), CS_E(2, 0),

        CS_T()
    },
};

static void test_cs_gc_12(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_12_BP);
}

static const chunit_test CS_GC_12 = {
    .name = "Collected Space Collect Garbage 12",
    .t = test_cs_gc_12,
    .timeout = 5,
};

// Big Graph 1.
static const cs_test_blueprint TEST_CS_GC_13_BP = {
    .num_objs = 11,
    .instructions = {
        CS_E(0, 2), CS_E(0, 3), CS_F(0),

        CS_E(1, 0), CS_F(1),

        CS_R(2), CS_E(2, 4), CS_E(2, 5),

        CS_E(3, 5), CS_E(3, 6), CS_E(3, 1), CS_F(3),

        CS_E(4, 7),

        CS_E(5, 7), CS_E(5, 8),

        CS_E(6, 5), CS_F(6),

        CS_E(7, 2),

        CS_R(8), CS_E(8, 7), CS_E(8, 9), CS_E(8, 10), CS_E(8, 7),

        CS_T()
    },
};

static void test_cs_gc_13(chunit_test_context *tc) {
    run_cs_test(tc, &TEST_CS_GC_13_BP);
}

static const chunit_test CS_GC_13 = {
    .name = "Collected Space Collect Garbage 13",
    .t = test_cs_gc_13,
    .timeout = 5,
};

// Time for some sort of parallel testing...

static const gc_worker_spec CONSTANT_GC = {
    .delay = NULL, // No delay.
    
    .shift = 1,
    .shift_trigger = 1,
};

typedef struct {
    chunit_test_context * const tc;
    collected_space * const cs;
} cs_worker_arg;

static void cs_gc_multi_template(chunit_test_context *tc, uint8_t chnl,
        uint64_t num_threads, void *(*worker)(void *)) {
    collected_space *cs = new_collected_space_seed(chnl, 1, 10, 1000);

    cs_start_gc(cs, &CONSTANT_GC);   
    
    cs_worker_arg worker_arg = {
        .tc = tc,
        .cs = cs,
    };
    
    util_thread_spray_info *spray = util_thread_spray(1, num_threads, 
           worker, &worker_arg);

    util_thread_collect(spray);

    assert_false(tc, cs_stop_gc(cs));

    delete_collected_space(cs);
}

static void *cs_root_worker(void *arg) {
    const uint64_t roots = 20;

    util_thread_spray_context *s_ctx = arg;
    cs_worker_arg *worker_arg = s_ctx->context;

    addr_book_vaddr *vaddrs = safe_malloc(get_chnl(worker_arg->cs), 
            sizeof(addr_book_vaddr) * roots);

    uint64_t i;
    for (i = 0; i < roots; i++) {
        // safe_printf("Starting Iter %llu\n", i);
        cs_root_id root_id = cs_malloc_root(worker_arg->cs, 0, 10);
        cs_get_root_res root_res = cs_get_root_vaddr(worker_arg->cs, root_id);

        assert_eq_uint(worker_arg->tc, CS_SUCCESS, root_res.status_code);
        assert_false(worker_arg->tc, null_adb_addr(root_res.root_vaddr));

        vaddrs[i] = root_res.root_vaddr;
    }

    for (i = 0; i < roots; i++) {
        assert_true(worker_arg->tc, cs_allocated(worker_arg->cs, vaddrs[i]));
    }

    safe_free(vaddrs);

    return NULL;
}

// This first multi tests makes sure that adding roots randomly
// and concurrently behaves correctly!
static void test_cs_gc_multi_0(chunit_test_context *tc) {
    cs_gc_multi_template(tc, 1, 10, cs_root_worker);
}

static const chunit_test CS_GC_MULTI_0 = {
    .name = "Collected Space Collect Garbage Multi 0",
    .t = test_cs_gc_multi_0,
    .timeout = 5,
};

static void *cs_obj_worker(void *arg) {
    // HMMM, what will this do...

    util_thread_spray_context *s_ctx = arg;
    cs_worker_arg *worker_arg = s_ctx->context; 

    chunit_test_context *tc = worker_arg->tc;
    collected_space *cs = worker_arg->cs;

    const uint64_t objs = 5;

    // NOTE : Lying below is some GC Deadlock issue.
    // Only arises when the GC Thread is on, so probs nothing
    // to do with MS.

    // 0 -> {1, 2}
    // 1 -> {3}
    // 2 -> {4}
    addr_book_vaddr vaddrs[objs];
    safe_printf("\nStarting\n");

    malloc_obj_res root_res = cs_malloc_object_and_hold(cs, 2, 0);
    vaddrs[0] = root_res.vaddr;

    safe_printf("Creating Child Object\n");
    vaddrs[1] = cs_malloc_object(cs, 2, 0);
    // vaddrs[2] = cs_malloc_object(cs, 2, 0);

    root_res.i.rt[0] = vaddrs[1];
    root_res.i.rt[1] = vaddrs[2]; 

    cs_unlock(cs, vaddrs[0]);

    safe_printf("\nDone with creation\n");

    return NULL;

    obj_index ind; 

    ind = cs_get_write_ind(cs, vaddrs[1]);
    vaddrs[3] = cs_malloc_object(cs, 0, 1);
    ind.rt[0] = vaddrs[3];
    cs_unlock(cs, vaddrs[1]);

    ind = cs_get_write_ind(cs, vaddrs[2]);
    vaddrs[4] = cs_malloc_object(cs, 0, 1);
    ind.rt[0] = vaddrs[4];
    cs_unlock(cs, vaddrs[2]);

    uint64_t i;
    for (i = 0; i < objs; i++) {
        assert_true(tc, cs_allocated(cs, vaddrs[i]));
    }

    return NULL;
}

static void test_cs_gc_multi_1(chunit_test_context *tc) {
    cs_gc_multi_template(tc, 1, 1, cs_obj_worker);
}

static const chunit_test CS_GC_MULTI_1 = {
    .name = "Collected Space Collect Garbage Multi 1",
    .t = test_cs_gc_multi_1,
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
        &CS_GC_3,
        &CS_GC_4,
        &CS_GC_5,
        &CS_GC_6,

        &CS_GC_7,
        &CS_GC_8,
        &CS_GC_9,
        &CS_GC_10,
        &CS_GC_11,

        &CS_GC_12,
        &CS_GC_13,
        &CS_GC_MULTI_0,
        &CS_GC_MULTI_1,
    },
    .tests_len = 19,
};
