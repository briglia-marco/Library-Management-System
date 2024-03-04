#include "aux_function.h"
#include "linked_list.h"
#include "queue.h"
#include "file.h"
#include "socket.h"
#include "thread.h"
#include "mutx.h"
#include "sig.h"
#include "socket.h"

// Queue

void initialize_queue(queue_t *queue){ // inizializza la coda
  queue->head = NULL;
  mypthread_mutex_init(&queue->mutex, NULL, __LINE__, __FILE__);
}

void push(queue_t *queue, void *data){ // aggiunge un elemento in coda 
  node_t *new_node = (node_t *)malloc(sizeof(node_t));
  if (new_node == NULL){
    perror("Errore allocazione memoria");
    exit(EXIT_FAILURE);
  }
  new_node->data = data;
  new_node->next = NULL;
  mypthread_mutex_lock(&queue->mutex, __LINE__, __FILE__);
  if (queue->head == NULL){
    queue->head = new_node;
  }
  else{
    node_t *current = queue->head;
    while (current->next != NULL){
      current = current->next;
    }
    current->next = new_node;
  }
  mypthread_mutex_unlock(&queue->mutex, __LINE__, __FILE__);
}

void *pop(queue_t *queue){ // rimuove un elemento dalla coda
  mypthread_mutex_lock(&queue->mutex, __LINE__, __FILE__);
  if(queue->head == NULL){
    mypthread_mutex_unlock(&queue->mutex, __LINE__, __FILE__);
    return NULL;
  }
  node_t *current = queue->head;
  queue->head = current->next;
  void *data = current->data;
  // Non liberiamo il nodo corrente qui, ma restituiamo solo i dati.
  // La liberazione del nodo sarà responsabilità del chiamante.
  mypthread_mutex_unlock(&queue->mutex, __LINE__, __FILE__);
  return data;
}

void free_queue(queue_t *queue){ // libera la coda
  mypthread_mutex_lock(&queue->mutex, __LINE__, __FILE__);
  node_t *current = queue->head;
  while (current != NULL){
    node_t *next = current->next;
    safe_free(current->data);
    safe_free(current);
    current = next;
  }
  queue->head = NULL;
  mypthread_mutex_unlock(&queue->mutex, __LINE__, __FILE__);
}

int is_empty(queue_t *queue){ // controlla se la coda è vuota
  mypthread_mutex_lock(&queue->mutex, __LINE__, __FILE__);
  int empty = queue->head == NULL;
  mypthread_mutex_unlock(&queue->mutex, __LINE__, __FILE__);
  return empty;
}
