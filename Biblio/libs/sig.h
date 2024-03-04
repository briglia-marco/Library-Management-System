#ifndef SIGNAL_H
#define SIGNAL_H
#include "aux_function.h"

void mysigemptyset(sigset_t *set, int linea, char *file);
void mysigaddset(sigset_t *set, int signum, int linea, char *file);

#endif
// Path: libs/signal.c
