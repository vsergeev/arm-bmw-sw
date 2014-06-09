/* Test with
 *  $ gcc -Wall -DQUEUE_TEST queue.c -o queue_test
 */

#include <stdlib.h>
#include <stdbool.h>

#include "queue.h"

#ifndef QUEUE_TEST
#include "lock.h"
#else
#define system_lock()
#define system_unlock()
#endif

/*
    Circular buffer indexing semantics used below

    widx points to empty space
    ridx points to filled space
    (widx == ridx) && !empty  means queue is full
    (widx == ridx) && empty   means queue is empty

    [   |   |   |   |   ]   Occup = 0, Avail = 5
     WR                     empty = true

    [ X | X | X | X | X ]   Occup = 5, Avail = 0
     WR                     empty = false

    [ X |   |   |   |   ]   Occup = 1, Avail = 4
      R   W                 empty = false

    [ X | X |   |   |   ]   Occup = 1, Avail = 4
      R       W             empty = false

    [ X |   |   | X | X ]   Occup = 3, Avail = 2
          W       R         empty = false

    [ X | X |   | X | X ]   Occup = 4, Avail = 1
              W   R         empty = false

    occupied = (widx - ridx) % capacity
    available = capacity - used

    enqueue
        if (!filled)
            queue[widx] = object
            widx = (widx + 1) % capacity
            if (widx == ridx)
                empty = false
            return 0
        return -1;

    dequeue
        if (!empty)
            object = queue[ridx]
            ridx = (ridx + 1) % capacity
            if (ridx == widx)
                empty = true
            return object
        return NULL
*/

static bool _queue_is_empty(struct queue *q) {
    if (q->ridx == q->widx)
        return q->empty;
    return false;
}

static bool _queue_is_full(struct queue *q) {
    if (q->ridx == q->widx)
        return !q->empty;
    return false;
}

void queue_init(struct queue *q, int capacity) {
    q->cap = capacity;
    q->ridx = 0;
    q->widx = 0;
    q->empty = true;
}

int queue_enqueue(struct queue *q, void *object) {
    /* Lock queue here */
    system_lock();

    if (_queue_is_full(q)) {
        /* Unlock queue here */
        system_unlock();
        return -1;
    }

    q->objects[q->widx] = object;
    q->widx = (q->widx + 1) % q->cap;
    if (q->widx == q->ridx)
        q->empty = false;

    /* Unlock queue here */
    system_unlock();

    return 0;
}

void *queue_dequeue(struct queue *q) {
    void *object;

    /* Lock queue here */
    system_lock();

    if (_queue_is_empty(q)) {
        /* Unlock queue here */
        system_unlock();
        return NULL;
    }

    object = q->objects[q->ridx];
    q->ridx = (q->ridx + 1) % q->cap;
    if (q->ridx == q->widx)
        q->empty = true;

    /* Unlock queue here */
    system_unlock();

    return object;
}

/* Queue unit test */
#ifdef QUEUE_TEST
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

int main(void) {
    struct queue *q = (struct queue *)&(QUEUE(5)){ .cap = 5, .ridx = 0, .widx = 0, .empty = true };

    /* Check initialization */
    queue_init(q, 5);
    assert(_queue_is_empty(q));
    assert(!_queue_is_full(q));

    /* Check NULL dequeue on empty */
    q->objects[0] = (void *)-1;
    assert(queue_dequeue(q) == NULL);

    /* Check queuing to capacity */
    assert(queue_enqueue(q, (void *)1) == 0);
    assert(queue_enqueue(q, (void *)2) == 0);
    assert(queue_enqueue(q, (void *)3) == 0);
    assert(queue_enqueue(q, (void *)4) == 0);
    assert(queue_enqueue(q, (void *)5) == 0);
    /* Check capacity is reached */
    assert(queue_enqueue(q, (void *)6) == -1);
    assert(!_queue_is_empty(q));
    assert(_queue_is_full(q));

    /* Check mixed dequeue / enqueue*/
    assert(queue_dequeue(q) == (void *)1);
    assert(queue_enqueue(q, (void *)7) == 0);
    assert(queue_dequeue(q) == (void *)2);
    assert(queue_enqueue(q, (void *)8) == 0);
    assert(queue_dequeue(q) == (void *)3);
    assert(queue_enqueue(q, (void *)9) == 0);
    /* Check capacity is reached again */
    assert(!_queue_is_empty(q));
    assert(_queue_is_full(q));

    /* Check dequeue order */
    assert(queue_dequeue(q) == (void *)4);
    assert(queue_dequeue(q) == (void *)5);
    assert(queue_dequeue(q) == (void *)7);
    assert(queue_dequeue(q) == (void *)8);
    assert(queue_dequeue(q) == (void *)9);
    /* Check empty is reached again */
    assert(_queue_is_empty(q));
    assert(!_queue_is_full(q));

    /* Fuzzing test, tracking queue length and enqueued/dequeued data */
    unsigned int i, len;
    uintptr_t data_queue, data_dequeue;
    len = 0;
    data_queue = data_dequeue = 1;
    for (i = 0; i < 5000000; i++) {
        if (random() & 0x1) {
            if (len == q->cap) {
                assert(queue_enqueue(q, NULL) == -1);
            } else {
                assert(queue_enqueue(q, (void *)data_queue++) == 0);
                len++;
            }
        } else {
            if (len == 0) {
                assert(queue_dequeue(q) == NULL);
            } else {
                assert(queue_dequeue(q) == (void *)data_dequeue++);
                len--;
            }
        }
    }

    printf("All tests passed!\n");

    return 0;
}
#endif

