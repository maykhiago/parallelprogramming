/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "queue/zm_queue.h"
#define TEST_NTHREADS 3
#define TEST_NELEMTS  1000

typedef struct thread_data thread_data_t;
struct thread_data {
    int tid;
    zm_queue_t* queue;
};

zm_atomic_uint_t test_counter = 0;

static void* func(void *arg) {
#if defined(ZMTEST_ALLOC_QELEM)
    size_t *input;
#else
    size_t input = 1;
#endif
    int tid, nelem_enq, nelem_deq, producer_b;
    zm_queue_t* queue;
    thread_data_t *data = (thread_data_t*) arg;
    tid   = data->tid;
    queue = data->queue;

#if   defined(ZMTEST_MPMC)
    nelem_enq = TEST_NELEMTS;
    nelem_deq = (TEST_NTHREADS/2)*TEST_NELEMTS;
    producer_b = (tid % 2 == 0);
#elif defined(ZMTEST_MPSC)
    nelem_enq = TEST_NELEMTS;
    nelem_deq = (TEST_NTHREADS-1)*TEST_NELEMTS;
    producer_b = (tid != 0);
#elif defined(ZMTEST_SPMC)
    nelem_enq = (TEST_NTHREADS-1)*TEST_NELEMTS;
    nelem_deq = (TEST_NTHREADS-1)*TEST_NELEMTS;
    producer_b = (tid == 0);
#endif

    if(producer_b) { /* producer */
        size_t elem;
        for(elem=0; elem < nelem_enq; elem++) {
#if defined(ZMTEST_ALLOC_QELEM)
            input = malloc(sizeof *input);
            *input = 1;
#endif
            zm_queue_enqueue(queue, (void*) input);
        }
    } else {           /* consumer */
        while(zm_atomic_load(&test_counter, zm_memord_acquire) < nelem_deq) {
            void* elem;
            zm_queue_dequeue(queue, (void**)&elem);
            if (elem != NULL) {
#if defined(ZMTEST_ALLOC_QELEM)
                if (*(int*)elem == 1)
                    zm_atomic_fetch_add(&test_counter, 1, zm_memord_acq_rel);
                else
                    abort();
                free(elem);
#else
                if ((size_t)elem == 1)
                    zm_atomic_fetch_add(&test_counter, 1, zm_memord_acq_rel);
                else
                    abort();
#endif
            }
        }

    }
    return 0;
}

/*-------------------------------------------------------------------------
 * Function: run
 *
 * Purpose: Test the correctness of queue operations by counting the number
 *  of dequeued elements to the expected number
 *
 * Return: Success: 0
 *         Failure: 1
 *-------------------------------------------------------------------------
 */
static void run() {
    int nelem_deq;
    void *res;
    pthread_t threads[TEST_NTHREADS];
    thread_data_t data[TEST_NTHREADS];

    zm_queue_t queue;
    zm_queue_init(&queue);

    zm_atomic_store(&test_counter, 0, zm_memord_release);

    int th;
    for (th=0; th < TEST_NTHREADS; th++) {
        data[th].tid = th;
        data[th].queue = &queue;
        pthread_create(&threads[th], NULL, func, (void*) &data[th]);
    }
    for (th=0; th < TEST_NTHREADS; th++)
        pthread_join(threads[th], &res);

#if   defined(ZMTEST_MPMC)
    nelem_deq = (TEST_NTHREADS/2)*TEST_NELEMTS;
#elif defined(ZMTEST_MPSC)
    nelem_deq = (TEST_NTHREADS-1)*TEST_NELEMTS;
#elif defined(ZMTEST_SPMC)
    nelem_deq = (TEST_NTHREADS-1)*TEST_NELEMTS;
#endif
    if(test_counter != nelem_deq)
        printf("Failed: got counter %d instead of %d\n", test_counter, nelem_deq);
    else
        printf("Pass\n");

} /* end test_lock_thruput() */

int main(int argc, char **argv)
{
  run();
} /* end main() */

