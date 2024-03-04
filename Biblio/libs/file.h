#ifndef FILE_H
#define FILE_H
#include "aux_function.h"

FILE *myfopen(const char *path, const char *mode, int linea, char *file);
int myfclose(FILE *stream, int linea, char *file);
int myflock(int fd, int operation, int linea, char *file);
void myclose(int fd, int linea, char *file);


#endif // FILE_H
// Path: libs/file.c
