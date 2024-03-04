#ifndef AUX_FUNCTION_H
#define AUX_FUNCTION_H
#define _GNU_SOURCE   
#include <stdio.h>
#include <stdlib.h>   
#include <string.h>   
#include <errno.h>   
#include <unistd.h>   
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/un.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <limits.h>
#include <ctype.h>
#include <signal.h>
#include <netinet/in.h>


//struct
#define BUFF_SIZE 1024
#define IP "127.0.0.1"
#define PORT 0
#define MSG_QUERY 'Q'
#define MSG_LOAN 'L'
#define MSG_RECORD 'R'
#define MSG_NO 'N'
#define MSG_ERROR 'E'

typedef struct node{
    void *data;
    struct node *next;
} node_t;

typedef struct{
    node_t *head;
    pthread_mutex_t lock;
    int size;
} linked_list_t;

typedef struct{
    char *autore;
    char *titolo; 
    char *editore;
    int anno;
    char *nota;
    char *collocazione;
    char *luogo_pubblicazione; 
    char *descrizione_fisica; 
    char *prestito; 
    char *volume;
    char *scaffale;
} Libro_t; 

typedef struct{
    char *etichetta;
    char *valore;
} richiesta_client_t;

typedef struct{
    node_t *head;
    pthread_mutex_t mutex;
} queue_t;

typedef struct{
    queue_t *coda;
    linked_list_t *biblioteca;
    pthread_mutex_t lock;
    fd_set *clients;
    FILE *log;
} param_worker_t;

typedef struct{
    char type;
    unsigned int length;
    char data[BUFF_SIZE];
} msg_client_t;

// termina programma
void termina(const char *s, int linea, char *file);
void termina_thread(const char *messaggio, int linea, char *file);

// funzioni ausiliarie
void free_libro(Libro_t *libro);
void riempi_scheda_libro(Libro_t *libro, char *line);
void remove_spaces(char* str);
void stampa_libro(Libro_t *libro);
int compara_libri(Libro_t *libro1, Libro_t *libro2);
int is_in_biblioteca(linked_list_t *, Libro_t *);
void remove_all_spaces(char *str);
void remove_duplicates(linked_list_t *);
void print_biblioteca(linked_list_t *);
void inizializza_libro(Libro_t *libro);
int parsing_client(int arg, char *argv[], linked_list_t *lista_arg);
void remove_dashes(char *str);
void to_upper_case(char *str);
void inizializza_richiesta(richiesta_client_t *richiesta);
void add_richiesta(linked_list_t *lista_arg, char *token);
void check_richiesta(char *token, char *string, int var_controllo, linked_list_t *lista_arg);
int fill_arr_socket(FILE *fd, int arr_socket[], char arr_server[]);

#endif // AUX_FUNCTION_H
