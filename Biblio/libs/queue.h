#ifndef QUEUE_H
#define QUEUE_H
#include "aux_function.h"

void initialize_queue(queue_t *);
void push(queue_t *, void *);
void *pop(queue_t *);
void free_queue(queue_t *);
int is_empty(queue_t *);

#endif  // QUEUE_H
// Path: libs/queue.c
