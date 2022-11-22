#ifndef CORE_THREADS_H
#define CORE_THREADS_H

#include <pthread.h>

// Wrappers for common pthread functions.
// TODO... fill in more as needed.

void safe_pt_rwl_rdlock(pthread_rwlock_t *rwl);
void safe_pt_rwl_wrlock(pthread_rwlock_t *rwl);
void safe_pt_rwl_unlock(pthread_rwlock_t *rwl);

#endif
