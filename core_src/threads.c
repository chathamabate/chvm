#include "./threads.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// I suspect I may need to use this at somepoint later...
//
void safe_pt_rwl_rdlock(pthread_rwlock_t *rwl) {
    if (pthread_rwlock_rdlock(rwl)) {
        printf("[Safe PT RWL] Error aquiring read lock.\n");
        exit(1);
    }
}

void safe_pt_rwl_wrlock(pthread_rwlock_t *rwl) {
    if (pthread_rwlock_wrlock(rwl)) {
        printf("[Safe PT RWL] Error aquiring write lock.\n");
        exit(1);
    }
}

void safe_pt_rwl_unlock(pthread_rwlock_t *rwl) {
    if (pthread_rwlock_unlock(rwl)) {
        printf("[Safe PT RWL] Error releasing lock.\n");
        exit(1);
    }
}
