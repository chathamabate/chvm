#ifndef CORE_THREAD_H
#define CORE_THREAD_H

#include <pthread.h>
#include <stdint.h>

void safe_pthread_create(pthread_t *thrd, 
        const pthread_attr_t *attr, 
        void *(*start_routine)(void *), 
        void *ag);

void safe_pthread_join(pthread_t thrd, void **retval);

void safe_mutex_init(pthread_mutex_t *mut, pthread_mutexattr_t *attr);
void safe_mutex_lock(pthread_mutex_t *mut);
void safe_mutex_unlock(pthread_mutex_t *mut);
void safe_mutex_destroy(pthread_mutex_t *mut);

void safe_rwlock_init(pthread_rwlock_t *rwlock, pthread_rwlockattr_t *attr);
void safe_rdlock(pthread_rwlock_t *rwlock);
void safe_wrlock(pthread_rwlock_t *rwlock);

// These return 0 if the lock was acquired.
// 1 if the lock was busy.
uint8_t safe_try_rdlock(pthread_rwlock_t *rwlock);
uint8_t safe_try_wrlock(pthread_rwlock_t *rwlock);

void safe_rwlock_unlock(pthread_rwlock_t *rwlock);
void safe_rwlock_destroy(pthread_rwlock_t *rwlock);

#endif
