#ifndef LINKED_LIST_H
#define LINKED_LIST_H
#include "aux_function.h"

typedef struct node{
    void *data;
    struct node *next;
} node_t;

typedef struct{
    node_t *head;
    pthread_mutex_t lock;
    int size;
} linked_list_t;

void handle_null_error(void *, char *);
void safe_free(void *);
void initialize_list(linked_list_t *);
void add_node(linked_list_t *, void *);
void remove_node(linked_list_t *, void *);
void free_list(linked_list_t *);
void *get_nth_element(linked_list_t *, int );

#endif  // LINKED_LIST_H    
// Path: libs/linked_list.c
