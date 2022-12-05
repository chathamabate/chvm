#ifndef CORE_THREAD_H
#define CORE_THREAD_H

#include <pthread.h>

// Safe wrappers for commonly used pthread functions.

void safe_rwlock_init(pthread_rwlock_t *rwlock, pthread_rwlockattr_t *attr);
void safe_rdlock(pthread_rwlock_t *rwlock);
void safe_wrlock(pthread_rwlock_t *rwlock);
void safe_rwlock_unlock(pthread_rwlock_t *rwlock);

#endif
