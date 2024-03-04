#include "aux_function.h"
#include "linked_list.h"
#include "queue.h"
#include "file.h"
#include "socket.h"
#include "thread.h"
#include "mutx.h"
#include "sig.h"
#include "socket.h"

int mypthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr, int linea, char *file){
  int e = pthread_mutex_init(mutex, attr);
  if (e!=0) {
    myperror(e, "Errore pthread_mutex_init");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    pthread_exit(NULL);
  }  
  return e;
}

int mypthread_mutex_destroy(pthread_mutex_t *mutex, int linea, char *file){
  int e = pthread_mutex_destroy(mutex);
  if (e!=0) {
    myperror(e, "Errore pthread_mutex_destroy");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    pthread_exit(NULL);
  }
  return e;
}

int mypthread_mutex_lock(pthread_mutex_t *mutex, int linea, char *file){
  int e = pthread_mutex_lock(mutex);
  if (e!=0) {
    myperror(e, "Errore pthread_mutex_lock");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    pthread_exit(NULL);
  }
  return e;
}

int mypthread_mutex_unlock(pthread_mutex_t *mutex, int linea, char *file){
  int e = pthread_mutex_unlock(mutex);
  if (e!=0){
    myperror(e, "Errore pthread_mutex_unlock");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    pthread_exit(NULL);
  }
  return e;
}

int mypthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex, int linea, char *file){
  int e = pthread_cond_wait(cond, mutex);
  if (e!=0){
    myperror(e, "Errore pthread_cond_wait");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    pthread_exit(NULL);
  }
  return e;
}
