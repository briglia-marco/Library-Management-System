#ifndef QUEUE_H
#define QUEUE_H
#include "aux_function.h"

typedef struct node_queue{
    void *data;
    struct node_queue *next;
} node_queue_t;

typedef struct{
    node_queue_t *head;
    pthread_mutex_t mutex;
} queue_t;

void initialize_queue(queue_t *);
void push(queue_t *, void *);
void *pop(queue_t *);
void free_queue(queue_t *);
int is_empty(queue_t *);

#endif  // QUEUE_H
// Path: libs/queue.c
