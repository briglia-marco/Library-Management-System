#include "libs/aux_function.h"
#include "libs/socket.h"
#include "libs/linked_list.h"
#include "libs/file.h"
#include "libs/queue.h"
#include "libs/mutx.h"
#include "libs/sig.h"
#include "libs/thread.h"
#define TEMPO_LIMITE_PRESTITO 30

// Structs

typedef struct{
    queue_t *coda;
    linked_list_t *biblioteca;
    pthread_mutex_t lock;
    FILE *log;
} param_worker_t;

// Functions prototypes

volatile int running = 1;
void *worker_function(void *arg);
void *signal_handler_function(void *arg);
int verifica_libro_richiesto(Libro_t *libro, Libro_t *libro_richiesto);
char *libro_to_record(Libro_t *libro, char *libro_record, char type);
void new_file_bib(linked_list_t *biblioteca, char *name_bib);

// Start main

int main(int argc, char *argv[]){

    // _________Signal Management_________
    sigset_t set;
    mysigemptyset(&set, __LINE__, __FILE__);
    mysigaddset(&set, SIGINT, __LINE__, __FILE__);
    mysigaddset(&set, SIGTERM, __LINE__, __FILE__);
    // detached signal handler thread
    pthread_t signal_handler;
    mypthread_sigmask(SIG_BLOCK, &set, NULL, __LINE__, __FILE__);
    mypthread_create(&signal_handler, NULL, signal_handler_function, (void *)&set, __LINE__, __FILE__);
    mypthread_detach(signal_handler, __LINE__, __FILE__);

    pthread_mutex_t mutex;
    mypthread_mutex_init(&mutex, NULL, __LINE__, __FILE__);


    // _________Parsing file record_________
    if(argc!=4){
        fprintf(stderr, "Usage: %s name_bib file_record W\n", argv[0]);
        exit(1);
    }
    char *name_bib = argv[1];
    char *file_record = argv[2];
    int W = atoi(argv[3]);

    linked_list_t *biblioteca = (linked_list_t *)malloc(sizeof(linked_list_t));
    if(biblioteca==NULL){ 
        perror("Errore allocazione memoria biblioteca");
        exit(EXIT_FAILURE);
    }
    initialize_list(biblioteca);

    char *line = calloc(1, sizeof(char)*BUFF_SIZE);
    if(line==NULL){
        perror("Errore allocazione memoria line");
        exit(EXIT_FAILURE);
    }
    size_t len = 0;
    ssize_t nread;

    FILE *fd = myfopen(file_record, "r", __LINE__, __FILE__);
    // Reading file record line by line and creating a linked list of books 
    while((nread = getline(&line, &len, fd)) != -1){
        if(nread == 1){
            continue;
        }

        Libro_t *libro = (Libro_t *)malloc(sizeof(Libro_t));
        if(libro==NULL){
            perror("Errore allocazione memoria libro");
            exit(EXIT_FAILURE);
        }
        inizializza_libro(libro);
        riempi_scheda_libro(libro, line); // fill the book record with the data from the file
        if(is_in_biblioteca(biblioteca, libro)){ // check if the book is already in the library comparing all the fields of the book
            safe_free(libro);
        }
        else{
            check_prestito(libro); // check if loan has expired
            add_node(biblioteca, libro); 
        }
    }
    myfclose(fd, __LINE__, __FILE__);
    safe_free(line);

    // _________SOCKET MANAGEMENT_________

    // Shared queue for the worker threads
    queue_t *coda = (queue_t *)malloc(sizeof(queue_t));
    if(coda==NULL){
        perror("Errore allocazione memoria coda");
        exit(EXIT_FAILURE);
    }
    initialize_queue(coda);

    // Socket creation and binding
    int sfd, client; 
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(IP);
    sfd = mysocket(AF_INET, SOCK_STREAM, 0, __LINE__, __FILE__);
    mybind(sfd, (struct sockaddr *)&server, sizeof(server), __LINE__, __FILE__);
    // get the port number assigned to the socket by the system
    socklen_t leng = sizeof(server);
    if (getsockname(sfd, (struct sockaddr *)&server, &leng) == -1) {
        perror("getsockname");
        exit(EXIT_FAILURE);
    }
    mylisten(sfd, SOMAXCONN, __LINE__, __FILE__);

    // _________CONFIG FILE_________

    // Create a file bib.conf to store the server name and the port number
    FILE *config = myfopen("bib.conf", "a", __LINE__, __FILE__);
    //myflock(fileno(config), LOCK_EX, __LINE__, __FILE__);
    int err = fprintf(config, "SERVER: %s, SOCKET: %d\n", name_bib, server.sin_port);
    if(err<0){
        perror("Errore scrittura file di configurazione");
        exit(EXIT_FAILURE);
    }
    fflush(config);
    //myflock(fileno(config), LOCK_UN, __LINE__, __FILE__);
    myfclose(config, __LINE__, __FILE__);

    // Select for managing the clients 
    fd_set fdset, rfdset; 
    int maxfd = sfd; 
    FD_ZERO(&fdset); 
    FD_ZERO(&rfdset); 
    FD_SET(sfd, &fdset); 

    // ________LOG FILE_________

    char *log_name = strdup(name_bib);
    strcat(log_name, ".log");
    FILE *log = myfopen(log_name, "w", __LINE__, __FILE__);
    safe_free(log_name);

    // thread worker parameters
    param_worker_t *args = (param_worker_t *)malloc(sizeof(param_worker_t));
    if(args==NULL){
        perror("Errore allocazione memoria parametri worker");
        exit(EXIT_FAILURE);
    }
    args->biblioteca = biblioteca; 
    args->coda = coda; 
    args->lock = mutex; 
    args->log = log;

    pthread_t worker[W];
    for(int i=0; i<W; i++){
        mypthread_create(&worker[i], NULL, worker_function, (void *)args, __LINE__, __FILE__);
    }

    // timeout select
    struct timeval timeout;
    timeout.tv_sec = 1; 
    timeout.tv_usec = 0;

    //_________MAIN LOOP_________

    while(running) {
        mypthread_mutex_lock(&mutex, __LINE__, __FILE__);
        rfdset = fdset;
        mypthread_mutex_unlock(&mutex, __LINE__, __FILE__);
        if(select(maxfd+1, &rfdset, NULL, NULL, &timeout) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        for(int i = 0; i <= maxfd; i++) {
            if(FD_ISSET(i, &rfdset)) {
                if(i == sfd) { // if the server socket is ready to read, accept the client
                    client = myaccept(sfd, NULL, NULL, __LINE__, __FILE__);
                    mypthread_mutex_lock(&mutex, __LINE__, __FILE__);
                    FD_SET(client, &fdset);
                    if(client > maxfd) {
                        maxfd = client;
                    }
                    mypthread_mutex_unlock(&mutex, __LINE__, __FILE__);
                } 
                else{ // if the client socket is ready to read, add the client to the queue
                    mypthread_mutex_lock(&mutex, __LINE__, __FILE__);
                    int *fdclient = calloc(1, sizeof(int));
                    if(fdclient == NULL) {
                        perror("Errore allocazione memoria fdclient");
                        exit(EXIT_FAILURE);
                    }
                    *fdclient = i;
                    push(coda, (void *)fdclient);
                    FD_CLR(i, &fdset);
                    mypthread_mutex_unlock(&mutex, __LINE__, __FILE__);
                }
            }
        }
    }

    // _________NEW FILE RECORD_________
    new_file_bib(biblioteca, name_bib);

    // _________WAIT FOR THREADS_________
    for(int i=0; i<W; i++){
        mypthread_join(worker[i], NULL, __LINE__, __FILE__);
    }

    // _________LOG FILE_________
    myfclose(log, __LINE__, __FILE__);

    // _________FREE MEMORY_________
    free_queue(coda);
    free_biblioteca(biblioteca);
    mypthread_mutex_destroy(&mutex, __LINE__, __FILE__);
    safe_free(args);

    // _________END_________
    return 0;
}

// Functions

void new_file_bib(linked_list_t *biblioteca, char *name_bib){
    char *name_dir = "new_bibData";
    mkdir(name_dir, 0777);
    char *new_file_name = strdup(name_bib);
    strcat(new_file_name, ".txt");
    char *new_file_path = strdup(name_dir);
    strcat(new_file_path, "/");
    strcat(new_file_path, new_file_name);
    FILE *new_file = myfopen(new_file_path, "w", __LINE__, __FILE__);
    for(int i=0; i<biblioteca->size; i++){
        Libro_t *libro = (Libro_t *)get_nth_element(biblioteca, i);
        char *libro_record = calloc(1, sizeof(char)*BUFF_SIZE);
        if(libro_record==NULL){
            perror("Errore allocazione memoria");
            exit(EXIT_FAILURE);
        }
        libro_to_record(libro, libro_record, MSG_QUERY);
        fprintf(new_file, "%s\n", libro_record);
    }
    fflush(new_file);
    myfclose(new_file, __LINE__, __FILE__);
}

void* worker_function(void *arg){
    param_worker_t *args = (param_worker_t *)arg; 
    linked_list_t *biblioteca = args->biblioteca; 
    queue_t *coda = args->coda; 
    pthread_mutex_t lock = args->lock; 
    FILE *log = args->log;  
    int client; 

    while(1){ 
        mypthread_mutex_lock(&lock, __LINE__, __FILE__);
        if(!running && is_empty(coda)){
            mypthread_mutex_unlock(&lock, __LINE__, __FILE__);
            break;
        } 
        if(is_empty(coda)){
            mypthread_mutex_unlock(&lock, __LINE__, __FILE__);
            continue;
        }

        int *p = calloc(1, sizeof(int));
        p = (int *)pop(coda); 
        if(p==NULL){
            mypthread_mutex_unlock(&lock, __LINE__, __FILE__);
            continue;
        }
        client = *p;
        free(p);
        mypthread_mutex_unlock(&lock, __LINE__, __FILE__);
        if(client==-1){
            continue;
        }

        msg_client_t *msg_to_client = calloc(1, sizeof(msg_client_t));
        if(msg_to_client==NULL){
            perror("Errore allocazione memoria msg_to_client");
            exit(EXIT_FAILURE);
        }
        msg_client_t *msg = calloc(1, sizeof(msg_client_t)); 
        if(msg==NULL){
            perror("Errore allocazione memoria");
            exit(EXIT_FAILURE);
        }
        char record_inviati[BUFF_SIZE]; // string for log
        memset(record_inviati, 0, sizeof(record_inviati));

        mypthread_mutex_lock(&lock, __LINE__, __FILE__);
        if(read(client, msg, sizeof(msg_client_t)) == -1){
            perror("read");
            exit(EXIT_FAILURE);
        }
        mypthread_mutex_unlock(&lock, __LINE__, __FILE__);

        int num_record = 0; // counter for log stats
        Libro_t *libro_richiesto = (Libro_t *)malloc(sizeof(Libro_t));
        if(libro_richiesto==NULL){
            perror("Errore allocazione memoria");
            exit(EXIT_FAILURE);
        }
        inizializza_libro(libro_richiesto);
        riempi_scheda_libro(libro_richiesto, msg->data);

        for(int i=0; i<biblioteca->size; i++){
            Libro_t *libro = (Libro_t *)get_nth_element(biblioteca, i);
            if(verifica_libro_richiesto(libro, libro_richiesto)){ 
                num_record++;
                char *libro_record = calloc(1, sizeof(char)*BUFF_SIZE);
                if(libro_record==NULL){
                    perror("Errore allocazione memoria");
                    exit(EXIT_FAILURE);
                }
                libro_to_record(libro, libro_record, msg->type);

                // record string for log 
                strcat(record_inviati, libro_record);
                strcat(record_inviati, "\n");

                int len_data = strlen(libro_record);
                char data[len_data];
                memset(data, 0, len_data);
                strcpy(data, libro_record);

                //invio messaggio al client
                msg_to_client->type = MSG_RECORD;
                msg_to_client->length = len_data;
                strcpy(msg_to_client->data, data);

                mypthread_mutex_lock(&lock, __LINE__, __FILE__);
                if(write(client, msg_to_client, sizeof(msg_client_t)) == -1){
                    perror("write MSG_RECORD");
                    exit(EXIT_FAILURE);
                }   
                mypthread_mutex_unlock(&lock, __LINE__, __FILE__);

                if(strcmp(libro_record, "prestito non disponibile")==0){
                    num_record--;
                }
            }
        }
        if(num_record==0){
            strcpy(record_inviati, " ");
            msg_to_client->type = MSG_NO;
            msg_to_client->length = 0;
            msg_to_client->data[0] = '\0';
            mypthread_mutex_lock(&lock, __LINE__, __FILE__);
            if(write(client, msg_to_client, sizeof(msg_client_t)) == -1){
                perror("write MSG_NO");
                exit(EXIT_FAILURE);
            }
            mypthread_mutex_unlock(&lock, __LINE__, __FILE__);
        }
        if(num_record<0){
            msg_to_client->type = MSG_ERROR;
            msg_to_client->length = 0;
            msg_to_client->data[0] = '\0';
            mypthread_mutex_lock(&lock, __LINE__, __FILE__);
            if(write(client, msg_to_client, sizeof(msg_client_t)) == -1){
                perror("write MSG_ERROR");
                exit(EXIT_FAILURE);
            }
            mypthread_mutex_unlock(&lock, __LINE__, __FILE__);
        }
        close(client);
        // riempi il file di log
        mypthread_mutex_lock(&lock, __LINE__, __FILE__);
        if(msg->type==MSG_LOAN){
            fprintf(log, "LOAN %d\n\n%s\n_______________\n", num_record, record_inviati);
        }
        if(msg->type==MSG_QUERY){
            fprintf(log, "QUERY %d\n\n%s\n______________\n", num_record, record_inviati);
        }
        fflush(log);
        mypthread_mutex_unlock(&lock, __LINE__, __FILE__);
    }
    return NULL;
}

void *signal_handler_function(void *arg){
    sigset_t *set = (sigset_t *)arg;
    int sig;
    while(running){
        sigwait(set, &sig);
        if(sig==SIGINT || sig==SIGTERM){
            running = 0;
        }
    }
    return NULL;
}

int verifica_libro_richiesto(Libro_t *libro, Libro_t *libro_richiesto){
    if(libro_richiesto->autore != NULL && (libro->autore == NULL || strstr(libro->autore, libro_richiesto->autore) == NULL)){
        return 0;
    }
    if(libro_richiesto->titolo != NULL && (libro->titolo == NULL || strstr(libro->titolo, libro_richiesto->titolo) == NULL)){
        return 0;
    }
    if(libro_richiesto->editore != NULL && (libro->editore == NULL || strstr(libro->editore, libro_richiesto->editore) == NULL)){
        return 0;
    }
    if(libro_richiesto->anno != 0 && libro->anno != libro_richiesto->anno){
        return 0;
    }
    if(libro_richiesto->nota != NULL && (libro->nota == NULL || strstr(libro->nota, libro_richiesto->nota) == NULL)){
        return 0;
    }
    if(libro_richiesto->collocazione != NULL && (libro->collocazione == NULL || strstr(libro->collocazione, libro_richiesto->collocazione) == NULL)){
        return 0;
    }
    if(libro_richiesto->luogo_pubblicazione != NULL && (libro->luogo_pubblicazione == NULL || strstr(libro->luogo_pubblicazione, libro_richiesto->luogo_pubblicazione) == NULL)){
        return 0;
    }
    if(libro_richiesto->descrizione_fisica != NULL && (libro->descrizione_fisica == NULL || strstr(libro->descrizione_fisica, libro_richiesto->descrizione_fisica) == NULL)){
        return 0;
    }
    if(libro_richiesto->volume != NULL && (libro->volume == NULL || strstr(libro->volume, libro_richiesto->volume) == NULL)){
        return 0;
    }
    if(libro_richiesto->scaffale != NULL && (libro->scaffale == NULL || strstr(libro->scaffale, libro_richiesto->scaffale) == NULL)){
        return 0;
    }
    return 1;
}

char *libro_to_record(Libro_t *libro, char *libro_record, char type){

    if(libro->autore!=NULL){
        strcat(libro_record, "autore: ");
        strcat(libro_record, libro->autore);
    }
    if(libro->titolo!=NULL){
        strcat(libro_record, "; titolo: ");
        strcat(libro_record, libro->titolo);
    }

    if(type==MSG_LOAN){
        if(libro->prestito != NULL){ // if the book is not available
            if(difftime(time(NULL), calcola_data_sec(libro->prestito)) > TEMPO_LIMITE_PRESTITO){
                strcpy(libro_record, "prestito non disponibile");
                return libro_record;
            }
        }
        // if the book is available, the loan is registered
        char data_attuale[20];
        data_to_string(data_attuale);
        strcat(libro_record, "; prestito: ");
        strcat(libro_record, data_attuale);
        libro->prestito = strdup(data_attuale);
    }
    if(type == MSG_QUERY){
        if(libro->prestito!=NULL){
            strcat(libro_record, "; prestito: ");
            strcat(libro_record, libro->prestito);
        }
    }

    if(libro->editore!=NULL){
        strcat(libro_record, "; editore: ");
        strcat(libro_record, libro->editore);
    }
    if(libro->anno!=0){
        char anno[5];
        sprintf(anno, "%d", libro->anno);
        strcat(libro_record, "; anno: ");
        strcat(libro_record, anno);
    }
    if(libro->nota!=NULL){
        strcat(libro_record, "; nota: ");
        strcat(libro_record, libro->nota);
    }
    if(libro->collocazione!=NULL){
        strcat(libro_record, "; collocazione: ");
        strcat(libro_record, libro->collocazione);
    }
    if(libro->luogo_pubblicazione!=NULL){
        strcat(libro_record, "; luogo_pubblicazione: ");
        strcat(libro_record, libro->luogo_pubblicazione);
    }
    if(libro->descrizione_fisica!=NULL){
        strcat(libro_record, "; descrizione_fisica: ");
        strcat(libro_record, libro->descrizione_fisica);
    }
    if(libro->volume!=NULL){
        strcat(libro_record, "; volume: ");
        strcat(libro_record, libro->volume);
    }
    if(libro->scaffale!=NULL){
        strcat(libro_record, "; scaffale: ");
        strcat(libro_record, libro->scaffale);
    }

    strcat(libro_record, "\0");
    return libro_record;
}


