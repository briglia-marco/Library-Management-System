#include "aux_function.h"
#include "linked_list.h"
#include "queue.h"
#include "file.h"
#include "socket.h"
#include "thread.h"
#include "mutx.h"
#include "sig.h"
#include "socket.h"

void mysigemptyset(sigset_t *set, int linea, char *file){
  int e = sigemptyset(set);
  if (e!=0) {
    myperror(e, "Errore sigemptyset");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    pthread_exit(NULL);
  }
}

void mysigaddset(sigset_t *set, int signum, int linea, char *file){
  int e = sigaddset(set, signum);
  if (e!=0) {
    myperror(e, "Errore sigaddset");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    pthread_exit(NULL);
  }
}
