#include "aux_function.h"
#include "linked_list.h"
#include "queue.h"
#include "file.h"
#include "socket.h"
#include "thread.h"
#include "mutx.h"
#include "sig.h"
#include "socket.h"

#define BUFFLEN 1024

void myperror(int en, char *msg){
  char buf[BUFFLEN];
  char *errmsg = strerror_r(en, buf, BUFFLEN);
  if(msg!=NULL)
    fprintf(stderr, "%s: %s\n", msg, errmsg);
  else
    fprintf(stderr, "%s\n", errmsg);
}

int mypthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*tfun) (void *), void *arg, int linea, char *file){
  int e = pthread_create(thread, attr, tfun, arg);
  if (e!=0) {
    myperror(e, "Errore pthread_create");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    pthread_exit(NULL);
  }
  return e;                       
}
             
int mypthread_join(pthread_t thread, void **retval, int linea, char *file){
  int e = pthread_join(thread, retval);
  if (e!=0) {
    myperror(e, "Errore pthread_join");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    pthread_exit(NULL);
  }
  return e;
}

int mypthread_detach(pthread_t thread, int linea, char *file){
  int e = pthread_detach(thread);
  if (e!=0) {
    myperror(e, "Errore pthread_detach");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    pthread_exit(NULL);
  }
  return e;
}

int mypthread_sigmask(int how, const sigset_t *set, sigset_t *oldset, int linea, char *file){
  int e = pthread_sigmask(how, set, oldset);
  if (e!=0) {
    myperror(e, "Errore pthread_sigmask");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    pthread_exit(NULL);
  }
  return e;
}
