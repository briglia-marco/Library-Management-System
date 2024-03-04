#include "aux_function.h"
#include "linked_list.h"
#include "queue.h"
#include "file.h"
#include "socket.h"
#include "thread.h"
#include "mutx.h"
#include "sig.h"
#include "socket.h"

int mysocket(int domain, int type, int protocol, int linea, char *file){
  int s = socket(domain, type, protocol);
  if(s==-1){
    perror("Errore socket");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    exit(1);
  }
  return s;
}

int mybind(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int linea, char *file){
  int e = bind(sockfd, addr, addrlen);
  if(e!=0){
    perror("Errore bind");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    exit(1);
  }
  return e;
}

int mylisten(int sockfd, int backlog, int linea, char *file){
  int e = listen(sockfd, backlog);
  if(e!=0){
    perror("Errore listen");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    exit(1);
  }
  return e;
}

int myaccept(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int linea, char *file){
  int s = accept(sockfd, addr, addrlen);
  if(s==-1){
    perror("Errore accept");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    exit(1);
  }
  return s;
}

int myconnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int linea, char *file){
  int e = connect(sockfd, addr, addrlen);
  if(e!=0){
    perror("Errore connect");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    exit(1);
  }
  return e;
}

int mysend(int sockfd, const void *buf, size_t len, int flags, int linea, char *file){
  int e = send(sockfd, buf, len, flags);
  if(e==-1){
    perror("Errore send");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    exit(1);
  }
  return e;
}

int myrecv(int sockfd, void *buf, size_t len, int flags, int linea, char *file){
  int e = recv(sockfd, buf, len, flags);
  if(e==-1){
    perror("Errore recv");
    fprintf(stderr, "== %d == Linea: %d, File: %s\n", getpid(), linea, file);
    exit(1);
  }
  return e;
} 
