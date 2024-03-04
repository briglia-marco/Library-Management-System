#ifndef MUTX_H
#define MUTX_H
#include "aux_function.h"

int mypthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr, int linea, char *file);
int mypthread_mutex_destroy(pthread_mutex_t *mutex, int linea, char *file);
int mypthread_mutex_lock(pthread_mutex_t *mutex, int linea, char *file);
int mypthread_mutex_unlock(pthread_mutex_t *mutex, int linea, char *file);
int mypthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex, int linea, char *file);

#endif  // MUTX_H
// Path: libs/mutx.c
