#include "libs/aux_function.h"
#include "libs/socket.h"
#include "libs/linked_list.h"
#include "libs/file.h"
#include "libs/queue.h"
#include "libs/mutx.h"
#include "libs/sig.h"
#include "libs/thread.h"
#define TEMPO_LIMITE_PRESTITO 30

int static running = 1;
void *signal_handler_function(void *arg);
void *worker_function(void *arg);
int verifica_libro_richiesto(Libro_t *libro, Libro_t *libro_richiesto);
char *libro_to_record(Libro_t *libro, char *libro_record, char type);

/*
3) Gestione delle connessioni: 
Scrivi una funzione che viene eseguita in un nuovo thread per gestire una connessione. 
Questa funzione dovrebbe leggere i dati dalla connessione, interpretare i dati come un comando, 
eseguire il comando sulla biblioteca e inviare una risposta al client.

4) Implementazione dei comandi: 
Implementa i comandi che il server dovrebbe essere in grado di gestire. 
Questi potrebbero includere comandi per ottenere informazioni su un libro, 
prenotare un libro, restituire un libro, ecc.
*/

int main(int argc, char *argv[]){
    // creazione gestione segnali
    sigset_t set;
    mysigemptyset(&set, __LINE__, __FILE__);
    mysigaddset(&set, SIGINT, __LINE__, __FILE__);
    mysigaddset(&set, SIGTERM, __LINE__, __FILE__);
    // creo un thread per gestire i segnali
    pthread_t signal_handler;
    mypthread_sigmask(SIG_BLOCK, &set, NULL, __LINE__, __FILE__);
    mypthread_create(&signal_handler, NULL, signal_handler_function, (void *)&set, __LINE__, __FILE__);
    mypthread_detach(signal_handler, __LINE__, __FILE__);
    // _________Parsing del file di record_________
    if(argc!=4){
        fprintf(stderr, "Usage: %s name_bib file_record W\n", argv[0]);
        exit(1);
    }
    char *name_bib = argv[1]; 
    char *file_record = argv[2]; 
    int W = atoi(argv[3]); // numero di worker threads

    FILE *fd = myfopen(file_record, "r", __LINE__, __FILE__); // apre il file di record in lettura
    linked_list_t *biblioteca = (linked_list_t *)malloc(sizeof(linked_list_t)); // crea la biblioteca
    if(biblioteca==NULL){ 
        perror("Errore allocazione memoria");
        exit(1);
    }
    initialize_list(biblioteca); 

    // crea un file di log
    char *log_name = strdup(name_bib);
    log_name = strcat(log_name, ".log");
    FILE *log = myfopen(log_name, "w", __LINE__, __FILE__);

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    // leggi il file di record riga per riga e crea una struttura dati del libro per ogni record 
    while((nread = getline(&line, &len, fd)) != -1){
        if(nread == 1){
            continue;
        }
        // crea una struttura dati del libro
        Libro_t *libro = (Libro_t *)malloc(sizeof(Libro_t));
        if(libro==NULL){
            perror("Errore allocazione memoria");
            exit(1);
        }
        inizializza_libro(libro); 
        riempi_scheda_libro(libro, line); // riempi la struttura dati del libro con i dati del record

        if(is_in_biblioteca(biblioteca, libro)){
            safe_free(libro);
        }
        else{
            add_node(biblioteca, libro);
        }
        
    }
    myclose(fileno(fd), __LINE__, __FILE__);
    //print_biblioteca(biblioteca);

    /*
    Crea un server socket che ascolta su una specifica porta. 
    Quando il server riceve una connessione, dovrebbe creare un nuovo thread per gestire la connessione.
    Il server registra un file di log (./name_bib.log) in cui per ogni richiesta effettuata da un client si
    registrano il numero dei record inviati, dei prestiti effettuati e le informazioni relative a ciascun record.
    */

    // creazione coda condivisa
    queue_t *coda = (queue_t *)malloc(sizeof(queue_t));
    if(coda==NULL){
        perror("Errore allocazione memoria");
        exit(1);
    }
    initialize_queue(coda);

    // crea una socket
    int sfd, client;
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(IP);

    // creazione socket
    sfd = mysocket(AF_INET, SOCK_STREAM, 0, __LINE__, __FILE__);
    mybind(sfd, (struct sockaddr *)&server, sizeof(server), __LINE__, __FILE__);
    // recupero porta
    socklen_t leng = sizeof(server);
    if (getsockname(sfd, (struct sockaddr *)&server, &leng) == -1) {
        perror("getsockname");
        exit(1);
    }

    // creazione file config
    int err;
    FILE *config = myfopen("bib.conf", "a", __LINE__, __FILE__);
    myflock(fileno(config), LOCK_EX, __LINE__, __FILE__);
    err = fprintf(config, "SERVER: %s, SOCKET: %d\n", name_bib, server.sin_port);
    if(err<0){
        perror("Errore scrittura file");
        exit(1);
    }
    myflock(fileno(config), LOCK_UN, __LINE__, __FILE__);
    fflush(config);
    myclose(fileno(config), __LINE__, __FILE__);

    mylisten(sfd, SOMAXCONN, __LINE__, __FILE__);

    // inizializzazione mutex
    pthread_mutex_t mutex;
    mypthread_mutex_init(&mutex, NULL, __LINE__, __FILE__);

    // gestione delle connessioni dei client con select
    fd_set fdset, rfdset; // fdset è l'insieme dei file descriptors da monitorare, rfdset è l'insieme dei file descriptors pronti
    int maxfd = sfd; // massimo file descriptor 
    FD_ZERO(&fdset); // inizializza l'insieme dei file descriptors
    FD_ZERO(&rfdset); // inizializza l'insieme dei file descriptors
    FD_SET(sfd, &fdset); // aggiungi la socket all'insieme dei file descriptors

    // inizializzazione thread worker
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

    // timeout per la select
    struct timeval timeout;
    timeout.tv_sec = 1; 
    timeout.tv_usec = 0;

    // ciclo principale dove il server accetta le connessioni con select e le inserisce nella coda condivisa 
    // da cui i thread worker le prelevano e le processano 
    while(running){
        rfdset = fdset; // copia l'insieme dei file descriptors da monitorare
        if(select(maxfd+1, &rfdset, NULL, NULL, &timeout) == -1){ // monitora i file descriptors
            perror("select");
            exit(EXIT_FAILURE);
        }
        for(int i=0; i<=maxfd; i++){
            if(FD_ISSET(i, &rfdset)){ // controlla se il file descriptor è pronto per la lettura
                if(i==sfd){ // se è la socket del server
                    client = myaccept(sfd, NULL, NULL, __LINE__, __FILE__); // accetta la connessione
                    printf("Connessione accettata %d\n", client);
                    FD_SET(client, &fdset); // aggiungi il client all'insieme dei file descriptors
                    printf("cliente aggiunto %d\n", client);
                    if(client > maxfd){ // aggiorna il massimo file descriptor
                        maxfd = client;
                    }
                }
                else{ // se è un client
                    mypthread_mutex_lock(&mutex, __LINE__, __FILE__);
                    int *fdclient = calloc(1, sizeof(int));
                    if(fdclient==NULL){
                        perror("Errore allocazione memoria");
                        exit(EXIT_FAILURE);
                    }
                    *fdclient = i;
                    push(coda, fdclient); // aggiungi il client alla coda condivisa
                    FD_CLR(i, &fdset); // rimuovi il client dall'insieme dei file descriptors
                    printf("client aggiunto alla coda %d\n", i);
                    mypthread_mutex_unlock(&mutex, __LINE__, __FILE__);
                }
            }
        }
    }

    // join dei thread worker
    for(int i=0; i<W; i++){
        mypthread_join(worker[i], NULL, __LINE__, __FILE__);
    }

    printf("Server terminato\n");
    // aggiornamento file bib
    config = myfopen("bib_prova.txt", "w", __LINE__, __FILE__);
    for(int i = 0; i < biblioteca->size; i++){
        Libro_t *libro = (Libro_t *)get_nth_element(biblioteca, i);
        char *record = calloc(1, sizeof(char)*BUFF_SIZE);
        if(record==NULL){
            perror("Errore allocazione memoria");
            exit(EXIT_FAILURE);
        }
        libro_to_record(libro, record, MSG_QUERY); //fix perchè no stampa i record a causa di MSG_QUERY
        fprintf(config, "%s\n", record);
        safe_free(record);
    }
    fflush(config);
    myfclose(config, __LINE__, __FILE__);
    printf("Aggiornamento file bib completato\n");

    // fine programma e chiusura di tutto
    //mypthread_mutex_destroy(&mutex, __LINE__, __FILE__);
    //myclose(sfd, __LINE__, __FILE__);
    
    // chiusura file di log
    //myfclose(log, __LINE__, __FILE__);

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

/*
Il worker scandisce la struttura condivisa alla ricerca di uno o più record che
verificano la richiesta effettuata ed eventualmente registrare il prestito. Le risposte alle richieste fatte
verranno inviate al client sulla stessa socket di connessione.
Le richieste del client possono essere di due soli tipi. Una query, in cui si chiedono i record che
contengono una specifica stringa in uno o più campi e un loan (prestito) in cui si richiede anche il
prestito di tutti i volumi relativi a record che verificano la query.
*/

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

    while(running){ 
        mypthread_mutex_lock(&lock, __LINE__, __FILE__);
        if(is_empty(coda)){
            mypthread_mutex_unlock(&lock, __LINE__, __FILE__);
            continue;
        }
        int* p = pop(coda); // preleva un client dalla coda
        if(p==NULL){
            mypthread_mutex_unlock(&lock, __LINE__, __FILE__);
            continue;
        }
        client = *p;
        printf("client prelevato %d\n", client);
        free(p);
        FD_SET(client, args->clients);
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
            printf("Messaggio ricevuto: %c\n", msg->type);
            printf("Lunghezza messaggio: %d\n", msg->length);
            printf("Dati: %s\n", msg->data);

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
                    printf("Libro trovato\n");
                    num_record++;
                    char *libro_record = calloc(1, sizeof(char)*BUFF_SIZE);
                    if(libro_record==NULL){
                        perror("Errore allocazione memoria");
                        exit(EXIT_FAILURE);
                    }
                    printf("pre libro_to_record\n");
                    libro_to_record(libro, libro_record, msg->type);
                    printf("post libro_to_record\n");
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
                    strcpy(msg_to_client->data, data);
                    printf("Messaggio: %s\n", msg_to_client->data);
                    if(write(client, msg_to_client, sizeof(msg_client_t)) == -1){
                        perror("write");
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
        if(libro->anno==0){
            return 0;
        }
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


