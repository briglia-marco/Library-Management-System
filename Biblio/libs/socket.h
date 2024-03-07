#ifndef SOCKET_H
#define SOCKET_H
#include "aux_function.h"

int mysocket(int domain, int type, int protocol, int linea, char *file);
int mybind(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int linea, char *file);
int mylisten(int sockfd, int backlog, int linea, char *file);
int myaccept(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int linea, char *file);
int myconnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int linea, char *file);

#endif  // SOCKET_H
// Path: libs/socket.c
