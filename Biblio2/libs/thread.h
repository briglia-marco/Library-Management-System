#ifndef THREAD_H
#define THREAD_H
#include "aux_function.h"

void myperror(int en, char *msg);
int mypthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*tfun) (void *), void *arg, int linea, char *file);
int mypthread_join(pthread_t thread, void **retval, int linea, char *file);
int mypthread_detach(pthread_t thread, int linea, char *file);
int mypthread_sigmask(int how, const sigset_t *set, sigset_t *oldset, int linea, char *file);

#endif // THREAD_H
// Path: libs/thread.c
