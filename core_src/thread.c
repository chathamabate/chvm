#include "./thread.h"
#include <pthread.h>
#include "./sys.h"

void safe_rwlock_init(pthread_rwlock_t *rwlock, pthread_rwlockattr_t *attr) {
    if (pthread_rwlock_init(rwlock, attr)) {
        core_logf(1, "Process failed to init lock.");
        safe_exit(1);
    }
}

void safe_rdlock(pthread_rwlock_t *rwlock) {
    if (pthread_rwlock_rdlock(rwlock)) {
        core_logf(1, "Process failed while acquiring read lock.");
        safe_exit(1);
    }
}

void safe_wrlock(pthread_rwlock_t *rwlock) {
    if (pthread_rwlock_wrlock(rwlock)) {
        core_logf(1, "Process failed while acquiring write lock.");
        safe_exit(1);
    }
}

void safe_rwlock_unlock(pthread_rwlock_t *rwlock) {
    if (pthread_rwlock_unlock(rwlock)) {
        core_logf(1, "Process failed while releasing lock.");
        safe_exit(1);
    }
}
