#include "./thread.h"
#include "../thread.h"

#include "../../testing_src/assert.h"
#include "../../core_src//thread.h"
#include <inttypes.h>


// We need the testing context to test!!!!
static const uint64_t const_num = 14;

typedef struct {
    chunit_test_context *tc;

    uint64_t num;    
} thread_test_context_T0;

static void *test_util_thread_T0(void *arg) {
    util_thread_spray_context *s_context = arg;
    thread_test_context_T0 *context = s_context->context;

    assert_eq_uint(context->tc, const_num, context->num);

    return NULL;
}

static void test_util_thread_spray_basic(chunit_test_context *tc) {
    thread_test_context_T0 context = {
        .tc = tc,
        .num = const_num
    };

    util_thread_spray_info *spray = 
        util_thread_spray(1, 10, test_util_thread_T0, &context); 

    util_thread_collect(spray);
}

static const chunit_test UTIL_TEST_THREAD_SPRAY_BASIC = {
    .name = "Thread Spray Test Basic",
    .t = test_util_thread_spray_basic,
    .timeout = 5,
};

typedef struct {
    chunit_test_context *tc;

    pthread_mutex_t *mut;
    uint64_t num;    
} thread_test_context_T1;

static void *test_util_thread_T1(void *arg) {
    util_thread_spray_context *s_context = arg;
    thread_test_context_T1 *context = s_context->context;

    safe_mutex_lock(context->mut);
    context->num++;
    safe_mutex_unlock(context->mut);

    return NULL;
}

static void test_util_thread_spray_mutex(chunit_test_context *tc) {
    const uint64_t threads = 10;

    pthread_mutex_t mut;
    safe_mutex_init(&mut, NULL);

    thread_test_context_T1 context = {
        .tc = tc,

        .mut = &mut,
        .num = 0
    };

    util_thread_spray_info *spray = 
        util_thread_spray(1, threads, test_util_thread_T1, &context); 

    util_thread_collect(spray);
    safe_mutex_destroy(&mut);

    assert_eq_uint(tc, threads, context.num);
}

static const chunit_test UTIL_TEST_THREAD_SPRAY_MUTEX = {
    .name = "Thread Spray Test Mutex",
    .t = test_util_thread_spray_mutex,
    .timeout = 5,
};





const chunit_test_suite UTIL_TEST_SUITE_THREAD = {
    .name = "Threading Suite",
    .tests = {
        &UTIL_TEST_THREAD_SPRAY_BASIC,
        &UTIL_TEST_THREAD_SPRAY_MUTEX,
    },
    .tests_len = 2
};
