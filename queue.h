#ifndef _QUEUE_H
#define _QUEUE_H

#include <stdbool.h>

/* Flexible array queue structure */
struct queue {
    unsigned int cap;
    unsigned int ridx;
    unsigned int widx;
    bool empty;
    /* lock goes here */
    void *objects[];
};

/* Static queue allocator */
#define QUEUE(N) \
    struct { \
        unsigned int cap; \
        unsigned int ridx; \
        unsigned int widx; \
        bool empty; \
        /* lock goes here */ \
        void *objects[N]; \
    }
/* Instantiate with
 *  QUEUE(5) q;
 *  queue_init((struct queue *)&q, 5);
 */

void queue_init(struct queue *q, int capacity);
int queue_enqueue(struct queue *q, void *object);
void *queue_dequeue(struct queue *q);

#endif

