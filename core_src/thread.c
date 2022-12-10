#include "./thread.h"
#include <pthread.h>
#include "./sys.h"

void safe_mutex_init(pthread_mutex_t *mut, pthread_mutexattr_t *attr) {
    if (pthread_mutex_init(mut, attr)) {
        core_logf(1, "Process failed to init mutex.");
        safe_exit(1);
    }
}

void safe_mutex_lock(pthread_mutex_t *mut) {
    if (pthread_mutex_lock(mut)) {
        core_logf(1, "Process failed to acquire mutex.");
        safe_exit(1);
    }
}

void safe_mutex_unlock(pthread_mutex_t *mut) {
    if (pthread_mutex_unlock(mut)) {
        core_logf(1, "Process failed to release mutex.");
        safe_exit(1); 
    }
}

void safe_mutex_destroy(pthread_mutex_t *mut) {
    if (pthread_mutex_destroy(mut)) {
        core_logf(1, "Process failed to destroy mutex.");
        safe_exit(1); 
    }
}

void safe_rwlock_init(pthread_rwlock_t *rwlock, pthread_rwlockattr_t *attr) {
    if (pthread_rwlock_init(rwlock, attr)) {
        core_logf(1, "Process failed to init rwlock.");
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
        core_logf(1, "Process failed while releasing rwlock.");
        safe_exit(1);
    }
}

void safe_rwlock_destroy(pthread_rwlock_t *rwlock) {
    if (pthread_rwlock_destroy(rwlock)) {
        core_logf(1, "Process failed while destroying rwlock.");
        safe_exit(1);
    }
}
