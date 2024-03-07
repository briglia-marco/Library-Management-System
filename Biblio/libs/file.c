#include "aux_function.h"
#include "linked_list.h"
#include "queue.h"
#include "file.h"
#include "socket.h"
#include "thread.h"
#include "mutx.h"
#include "sig.h"
#include "socket.h"

FILE *myfopen(const char *path, const char *mode, int linea, char *file){
  FILE *f = fopen(path,mode);
  if(f==NULL){
    perror("Errore apertura file");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    exit(EXIT_FAILURE);
  }
  return f;
}

int myfclose(FILE *f, int linea, char *file){
  int e = fclose(f);
  if(e!=0){
    perror("Errore chiusura file");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    exit(EXIT_FAILURE);
  }
  return e;
}

int myflock(int fd, int operation, int linea, char *file){
  int e = flock(fd, operation);
  if(e!=0){
    perror("Errore lock file");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    exit(EXIT_FAILURE);
  }
  return e;
}

void myclose(int fd, int linea, char *file){
  int e = close(fd);
  if(e!=0){
    perror("Errore chiusura file descriptor");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    exit(EXIT_FAILURE);
  }
  return;
}
