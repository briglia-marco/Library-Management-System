#include "aux_function.h"
#include "linked_list.h"
#include "queue.h"
#include "file.h"
#include "socket.h"
#include "thread.h"
#include "mutx.h"
#include "sig.h"
#include "socket.h"

void handle_null_error(void *ptr, char *msg){
  if (ptr == NULL){
    fprintf(stderr, "linked list: %s\n", msg);
    exit(1);
  }
}

void safe_free(void *ptr){
  if (ptr != NULL){
    free(ptr);
    ptr = NULL;
  }
}

void initialize_list(linked_list_t *list){
  list->head = NULL;
  mypthread_mutex_init(&list->lock, NULL, __LINE__, __FILE__);
  list->size = 0;
}

void add_node(linked_list_t *list, void *libro){
  node_t *new_node = (node_t *)malloc(sizeof(node_t));
  if (new_node == NULL){
    perror("Errore allocazione memoria");
    exit(1);
  }
  new_node->data = libro;
  mypthread_mutex_lock(&list->lock, __LINE__, __FILE__);
  new_node->next = list->head;
  list->head = new_node;
  list->size = list->size + 1;
  mypthread_mutex_unlock(&list->lock, __LINE__, __FILE__);
}

void remove_node(linked_list_t *list, void *data){
  mypthread_mutex_lock(&list->lock, __LINE__, __FILE__);
  node_t *current = list->head;
  node_t *previous = NULL;
  while (current != NULL){
    if (current->data == data){
      if (previous == NULL){
        list->head = current->next;
      }
      else{
        previous->next = current->next;
      }
      safe_free(current);
      list->size = list->size - 1;
      break;
    }
    else{
      previous = current;
      current = current->next;
    }
  }
  if (current == NULL){
    dprintf(2, "linked list: remove_node: data not found\n");
    exit(EXIT_FAILURE);
  }
  mypthread_mutex_unlock(&list->lock, __LINE__, __FILE__);
}

void *get_nth_element(linked_list_t *list, int n){
  if (n < 0 || n >= list->size){
    return NULL;
  }
  mypthread_mutex_lock(&list->lock, __LINE__, __FILE__);
  node_t *current = list->head;
  for (int i = 0; i < n; i++){
    current = current->next;
  }
  void *data = current->data;
  mypthread_mutex_unlock(&list->lock, __LINE__, __FILE__);
  return data;
}

void free_list(linked_list_t *list){
  mypthread_mutex_lock(&list->lock, __LINE__, __FILE__);
  node_t *current = list->head;
  while (current != NULL){
    node_t *next = current->next;
    safe_free(current->data);
    safe_free(current);
    current = next;
  }
  list->head = NULL;
  list->size = 0;
  mypthread_mutex_unlock(&list->lock, __LINE__, __FILE__);
}
