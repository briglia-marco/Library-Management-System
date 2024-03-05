#include "libs/aux_function.h"
#include "libs/socket.h"
#include "libs/linked_list.h"
#include "libs/file.h"
#include "libs/queue.h"
#include "libs/mutx.h"
#include "libs/sig.h"
#include "libs/thread.h"
#define TEMPO_LIMITE_PRESTITO 30

// Functions prototypes

volatile int running = 1;
void *signal_handler_function(void *arg);
void *worker_function(void *arg);
int verifica_libro_richiesto(Libro_t *libro, Libro_t *libro_richiesto);
char *libro_to_record(Libro_t *libro, char *libro_record, char type);
void record_new_file_record(char *name_bib, linked_list_t *biblioteca);

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

    // _________Parsing file record_________
    if(argc!=4){
        fprintf(stderr, "Usage: %s name_bib file_record W\n", argv[0]);
        exit(1);
    }
    char *name_bib = argv[1];
    char *file_record = argv[2];
    int W = atoi(argv[3]);

    FILE *fd = myfopen(file_record, "r", __LINE__, __FILE__);
    linked_list_t *biblioteca = (linked_list_t *)malloc(sizeof(linked_list_t));
    if(biblioteca==NULL){ 
        perror("Errore allocazione memoria");
        exit(1);
    }
    initialize_list(biblioteca); 

    // LOG FILE
    char *log_name = strdup(name_bib);
    log_name = strcat(log_name, ".log");
    FILE *log = myfopen(log_name, "w", __LINE__, __FILE__);

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    // Reading file record line by line and creating a linked list of books 
    while((nread = getline(&line, &len, fd)) != -1){
        if(nread == 1){
            continue;
        }
        Libro_t *libro = (Libro_t *)malloc(sizeof(Libro_t)); 
        if(libro==NULL){
            perror("Errore allocazione memoria");
            exit(1);
        }
        inizializza_libro(libro);
        riempi_scheda_libro(libro, line); // fill the book record with the data from the file

        if(is_in_biblioteca(biblioteca, libro)){ // check if the book is already in the library comparing all the fields of the book
            safe_free(libro); // if true, free the memory
        }
        else{
            add_node(biblioteca, libro); // if false, add the book to the library
        }
        
    }
    myclose(fileno(fd), __LINE__, __FILE__);
    safe_free(line);

    // _________SOCKET MANAGEMENT_________

    // Shared queue for the worker threads
    queue_t *coda = (queue_t *)malloc(sizeof(queue_t));
    if(coda==NULL){
        perror("Errore allocazione memoria");
        exit(1);
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
        exit(1);
    }

    // _________CONFIG FILE_________

    // Create a file bib.conf to store the server name and the port number
    FILE *config = myfopen("bib.conf", "a", __LINE__, __FILE__);
    //myflock(fileno(config), LOCK_EX, __LINE__, __FILE__);
    int err = fprintf(config, "SERVER: %s, SOCKET: %d\n", name_bib, server.sin_port);
    if(err<0){
        perror("Errore scrittura file");
        exit(1);
    }
    //myflock(fileno(config), LOCK_UN, __LINE__, __FILE__);
    fflush(config);
    myfclose(config, __LINE__, __FILE__);

    mylisten(sfd, SOMAXCONN, __LINE__, __FILE__);

    pthread_mutex_t mutex;
    mypthread_mutex_init(&mutex, NULL, __LINE__, __FILE__);

    // Select for managing the clients 
    fd_set fdset, rfdset; // fdset è l'insieme dei file descriptors da monitorare, rfdset è l'insieme dei file descriptors pronti
    int maxfd = sfd; 
    FD_ZERO(&fdset); 
    FD_ZERO(&rfdset); 
    FD_SET(sfd, &fdset); 

    // thread worker parameters
    param_worker_t *args = (param_worker_t *)malloc(sizeof(param_worker_t));
    if(args==NULL){
        perror("Errore allocazione memoria");
        exit(1);
    }
    args->biblioteca = biblioteca; 
    args->coda = coda; 
    args->lock = mutex; 
    args->clients = &fdset;
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

    // main loop where the server accepts the clients and adds them to the queue used by the worker threads 
    // Dopo aver inizializzato l'insieme dei file descriptor fdset...
while(running) {
    rfdset = fdset;
    if(select(maxfd+1, &rfdset, NULL, NULL, &timeout) == -1) {
        perror("select");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i <= maxfd; i++) {
        if(FD_ISSET(i, &rfdset)) {
            if(i == sfd) { // Nuova connessione in arrivo
                client = myaccept(sfd, NULL, NULL, __LINE__, __FILE__);
                FD_SET(client, &fdset);
                if(client > maxfd) {
                    maxfd = client;
                }
                // Aggiungi il file descriptor del client alla coda condivisa
                mypthread_mutex_lock(&mutex, __LINE__, __FILE__);
                int *fdclient = calloc(1, sizeof(int));
                if(fdclient == NULL) {
                    perror("Errore allocazione memoria");
                    exit(EXIT_FAILURE);
                }
                *fdclient = client;
                push(coda, fdclient);
                mypthread_mutex_unlock(&mutex, __LINE__, __FILE__);
            } 
            else{ 
                mypthread_mutex_lock(&mutex, __LINE__, __FILE__);
                int *fdclient = calloc(1, sizeof(int));
                if(fdclient == NULL) {
                    perror("Errore allocazione memoria");
                    exit(EXIT_FAILURE);
                }
                *fdclient = i;
                push(coda, fdclient);
                FD_CLR(*fdclient, &fdset);
                mypthread_mutex_unlock(&mutex, __LINE__, __FILE__);
            }
        }
    }
}


    /*
    Il server termina quando riceve un segnale di SIGINT o SIGTERM, in questo caso si attende la
    terminazione dei thread worker, in modo che vengano elaborate le richieste pendenti, si termina la
    scrittura del file di log, si registra il nuovo file_record (con le nuove date di prestito) e si elimina
    la socket del server. Infine il server viene terminato.
    */

    // Completa le richieste pendenti
    for(int i=0; i<W; i++){
        mypthread_join(worker[i], NULL, __LINE__, __FILE__);
    }

    // Registra il nuovo file_record
    record_new_file_record(name_bib, biblioteca);

    // _________FREE MEMORY_________

/*
    // free the memory allocated for the linked list
    free_list(biblioteca);
    safe_free(biblioteca);
    // free the memory allocated for the queue
    free_queue(coda);
    safe_free(coda);
    // free the memory allocated for the worker parameters
    safe_free(args);
    // free the memory allocated for the log file
    myfclose(log, __LINE__, __FILE__);
    myfclose(new_bib, __LINE__, __FILE__);
    // free the memory allocated for the mutex
    mypthread_mutex_destroy(&mutex, __LINE__, __FILE__);
    // close the socket
    myclose(sfd, __LINE__, __FILE__);
*/
    // _________END_________

return 0;
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

void record_new_file_record(char *name_bib, linked_list_t *biblioteca){
    // update the file bib.data with the new data
    mkdir("new_bib_data", 0700);
    char filename[256];
    sprintf(filename, "new_bib_data/bib%c_new.txt", name_bib[0]);
    FILE *new_bib = myfopen(filename, "w", __LINE__, __FILE__);
    for(int i = 0; i < biblioteca->size; i++){
        Libro_t *libro = (Libro_t *)get_nth_element(biblioteca, i);
        char *record = calloc(1, sizeof(char)*BUFF_SIZE);
        if(record==NULL){
            perror("Errore allocazione memoria");
            exit(EXIT_FAILURE);
        }
        libro_to_record(libro, record, MSG_QUERY);
        fprintf(new_bib, "%s\n", record);
        safe_free(record);
    }
    fflush(new_bib);
    myfclose(new_bib, __LINE__, __FILE__);
}

void* worker_function(void *arg){
    param_worker_t *args = (param_worker_t *)arg; // passo i parametri al thread
    linked_list_t *biblioteca = args->biblioteca; // passo la biblioteca al thread
    queue_t *coda = args->coda; // passo la coda al thread
    pthread_mutex_t lock = args->lock; // passo il mutex al thread
    FILE *log = args->log; // passo il file di log al thread
    //fd_set clients = args->clients; // passo l'insieme dei file descriptors al thread
    int client; // file descriptor del client
    msg_client_t *msg_to_client = (msg_client_t *)malloc(sizeof(msg_client_t));
    if(msg_to_client==NULL){
        perror("Errore allocazione memoria");
        exit(EXIT_FAILURE);
    }
    printf("worker pronto\n");
    char record_inviati[BUFF_SIZE];

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
        int *p = pop(coda); // preleva un client dalla coda
        if(p==NULL){
            mypthread_mutex_unlock(&lock, __LINE__, __FILE__);
            continue;
        }
        client = *p;
        //printf("client prelevato %d\n", client);
        free(p);
        mypthread_mutex_unlock(&lock, __LINE__, __FILE__);
        if(client==-1){ // se la coda è vuota
            continue;
        }
        if(FD_ISSET(client, args->clients)){ // controlla se il client è pronto per la lettura
            msg_client_t *msg = (msg_client_t *)malloc(sizeof(msg_client_t)); // allocazione memoria per i dati 
            if(msg==NULL){
                perror("Errore allocazione memoria");
                exit(EXIT_FAILURE);
            }
            memset(msg, 0, sizeof(msg_client_t));
            // leggo i dati dal client
            if(read(client, msg, sizeof(msg_client_t)) == -1){
                perror("read");
                exit(EXIT_FAILURE);
            }
            //printf("Messaggio ricevuto: %c\n", msg->type);
            //printf("Lunghezza messaggio: %d\n", msg->length);
            //printf("Dati: %s\n", msg->data);

            time_t now;
            time(&now);
            struct tm *tm = localtime(&now); // now è la data attuale in secondi dal 1 gennaio 1970 
            char data_attuale[20];
            strftime(data_attuale, 20, "%d-%m-%Y %H:%M:%S", tm);

            int num_record = 0; // numero dei record inviati
            Libro_t *libro_richiesto = (Libro_t *)malloc(sizeof(Libro_t));
            if(libro_richiesto==NULL){
                perror("Errore allocazione memoria");
                exit(EXIT_FAILURE);
            }
            inizializza_libro(libro_richiesto);
            riempi_scheda_libro(libro_richiesto, msg->data);

            for(int i=0; i<biblioteca->size; i++){
                Libro_t *libro = (Libro_t *)get_nth_element(biblioteca, i);
                if(verifica_libro_richiesto(libro, libro_richiesto)){ // verifico se tutti i campi del libro richiesto sono presenti nel libro
                    //printf("Libro trovato\n");
                    num_record++;
                    char *libro_record = calloc(1, sizeof(char)*BUFF_SIZE);
                    if(libro_record==NULL){
                        perror("Errore allocazione memoria");
                        exit(EXIT_FAILURE);
                    }
                    libro_to_record(libro, libro_record, msg->type);
                    // rimepio record_inviati con i record che verificano la richiesta
                    strcat(record_inviati, libro_record);
                    strcat(record_inviati, "\n");
                    int len_data = strlen(libro_record);
                    char data[len_data];
                    memset(data, 0, len_data);
                    strcat(data, libro_record);
                    //invio messaggio al client
                    msg_to_client->type = MSG_RECORD;
                    msg_to_client->length = len_data;
                    strcat(msg_to_client->data, data);
                    //printf("Messaggio: %s\n", msg_to_client->data);
                    if(write(client, msg_to_client, sizeof(msg_client_t)) == -1){
                        perror("write SERVER");
                        exit(EXIT_FAILURE);
                    }
                    if(strcmp(libro_record, "prestito non disponibile")==0){
                        num_record--;
                    }
                }
            }
            if(num_record==0){
                strcat(record_inviati, " ");
                msg_to_client->type = MSG_NO;
                msg_to_client->length = 0;
                msg_to_client->data[0] = '\0';
                if(write(client, msg_to_client, sizeof(msg_client_t)) == -1){
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }
            if(num_record<0){
                msg_to_client->type = MSG_ERROR;
                msg_to_client->length = 0;
                msg_to_client->data[0] = '\0';
                if(write(client, msg_to_client, sizeof(msg_client_t)) == -1){
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }

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

            //myfclose(log, __LINE__, __FILE__);
            free_libro(libro_richiesto);
            safe_free(msg);
            FD_CLR(client, args->clients); // rimuovi il client dall'insieme dei file descriptors
        }
        memset(record_inviati, 0, sizeof(record_inviati));
    }
    return NULL;
}

int verifica_libro_richiesto(Libro_t *libro, Libro_t *libro_richiesto){ 
    if(libro->prestito!=NULL){
        struct tm tempo_prestito = {0};
        strptime(libro->prestito, "%d-%m-%Y %H:%M:%S", &tempo_prestito);
        time_t t = mktime(&tempo_prestito); // t è la data di prestito in secondi dal 1 gennaio 1970
        time_t now;
        time(&now); // now è la data attuale in secondi dal 1 gennaio 1970
        if(difftime(t, now)>TEMPO_LIMITE_PRESTITO){ // 30 secondi di prestito 
            safe_free(libro->prestito); // se il prestito è scaduto lo elimino
        }
    }
    if(libro_richiesto->autore!=NULL){
        if(libro->autore==NULL){
            return 0;
        }
        if(strstr(libro->autore, libro_richiesto->autore)==NULL){
            return 0;
        }
    }
    if(libro_richiesto->titolo!=NULL){
        if(libro->titolo==NULL){
            return 0;
        }
        if(strstr(libro->titolo, libro_richiesto->titolo)==NULL){
            return 0;
        }
    }
    if(libro_richiesto->editore!=NULL){
        if(libro->editore==NULL){
            return 0;
        }
        if(strstr(libro->editore, libro_richiesto->editore)==NULL){
            return 0;
        }
    }
    if(libro_richiesto->anno!=0){
        if(libro->anno!=libro_richiesto->anno){
            return 0;
        }
    }
    if(libro_richiesto->nota!=NULL){
        if(libro->nota==NULL){
            return 0;
        }
        if(strstr(libro->nota, libro_richiesto->nota)==NULL){
            return 0;
        }
    }
    if(libro_richiesto->collocazione!=NULL){
        if(libro->collocazione==NULL){
            return 0;
        }
        if(strstr(libro->collocazione, libro_richiesto->collocazione)==NULL){
            return 0;
        }
    }
    if(libro_richiesto->luogo_pubblicazione!=NULL){
        if(libro->luogo_pubblicazione==NULL){
            return 0;
        }
        if(strstr(libro->luogo_pubblicazione, libro_richiesto->luogo_pubblicazione)==NULL){
            return 0;
        }
    }
    if(libro_richiesto->descrizione_fisica!=NULL){
        if(libro->descrizione_fisica==NULL){
            return 0;
        }
        if(strstr(libro->descrizione_fisica, libro_richiesto->descrizione_fisica)==NULL){
            return 0;
        }
    }
    if(libro_richiesto->volume!=NULL){
        if(libro->volume==NULL){
            return 0;
        }
        if(strstr(libro->volume, libro_richiesto->volume)==NULL){
            return 0;
        }
    }
    if(libro_richiesto->scaffale!=NULL){
        if(libro->scaffale==NULL){
            return 0;
        }
        if(strstr(libro->scaffale, libro_richiesto->scaffale)==NULL){
            return 0;
        }
    }
    return 1;
}

char *libro_to_record(Libro_t *libro, char *libro_record, char type){
    time_t now;
    time(&now); // now è la data attuale in secondi dal 1 gennaio 1970
    struct tm *tm = localtime(&now);
    char data_attuale[20];
    strftime(data_attuale, 20, "%d-%m-%Y %H:%M:%S", tm);

    if(libro->autore!=NULL){
        strcat(libro_record, "autore: ");
        strcat(libro_record, libro->autore);
    }
    if(libro->titolo!=NULL){
        strcat(libro_record, "; titolo: ");
        strcat(libro_record, libro->titolo);
    }

    if(type==MSG_LOAN){
        if(libro->prestito!=NULL){
            struct tm tempo_prestito = {0}; 
            strptime(libro->prestito, "%d-%m-%Y %H:%M:%S", &tempo_prestito); // converto la data di prestito in una struttura tm
            time_t t = mktime(&tempo_prestito); // t è la data di prestito in secondi dal 1 gennaio 1970
            if(difftime(now, t)<TEMPO_LIMITE_PRESTITO){ // 30 secondi di prestito 
                //resetto libro_record e ci scrivo prestito non disponibile
                strcpy(libro_record, "prestito non disponibile");
                return libro_record;
            }
        }
        strcat(libro_record, "; prestito: ");
        strcat(libro_record, data_attuale);
        libro->prestito = strdup(data_attuale);
    }
    else if(libro->prestito!=NULL){
        strcat(libro_record, "; prestito: "); 
        strcat(libro_record, libro->prestito);
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


