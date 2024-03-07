#include "libs/aux_function.h"
#include "libs/socket.h"
#include "libs/linked_list.h"
#include "libs/file.h"
#include "libs/queue.h"
#include "libs/mutx.h"
#include "libs/sig.h"
#include "libs/thread.h"
#define MAX_SERVER_SOCKET 5 

typedef struct{
    char type;
    unsigned int length;
    char data[BUFF_SIZE];
} msg_from_client_t;

int main(int argc, char *argv[]){
    //________Parsing degli argomenti della linea di comando________
    if(argc<2){
        fprintf(stderr, "%s Usage: ./biblient [--field]... [-p]\n\n[--field]: What are you searching for [-p]: if you want to loan", argv[0]);
        exit(EXIT_FAILURE);
    }
    if(argc==2 && strcmp(argv[1], "-p")==0){
        fprintf(stderr, "%s Usage: ./biblient [--field]... [-p]\n\n[--field]: What are you searching for [-p]: if you want to loan", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Creazione della lista di richieste del client e restituisce se è stato richiesto un prestito o meno
    linked_list_t *lista_arg = (linked_list_t*)malloc(sizeof(linked_list_t));
    if(lista_arg==NULL){
        perror("Errore allocazione memoria lista_arg");
        exit(EXIT_FAILURE);
    }
    initialize_list(lista_arg);
    int prestito = parsing_client(argc, argv, lista_arg);

    // protocollo di comunicazione
    char type;
    if(prestito == 1)
        type = MSG_LOAN;
    else
        type = MSG_QUERY; 
    unsigned int length = 0;
    size_t dim = 0;
    for(int i=0; i<lista_arg->size; i++){
        richiesta_client_t *req = (richiesta_client_t *)get_nth_element(lista_arg, i);
        dim += strlen(req->etichetta) + strlen(req->valore) + 2; // per : e ;
    }
    dim += 1; // per il terminatore di stringa
    length = dim; // per il tipo e la dimensione
    char data[length];
    memset(data, 0, length);
    for(int i=0; i<lista_arg->size; i++){
        richiesta_client_t *req = (richiesta_client_t *)get_nth_element(lista_arg, i);
        strcpy(data, req->etichetta);
        strcat(data, ":");
        strcat(data, req->valore);
        strcat(data, ";");
    }

    msg_from_client_t *msg = calloc(1, sizeof(msg_from_client_t));
    if(msg==NULL){
        perror("Errore allocazione memoria");
        exit(EXIT_FAILURE);
    }
    msg->type = type;
    msg->length = dim;
    strcpy(msg->data, data); 

    // Inserisco i dati del file di configurazione in un array di stringhe e un array di interi 
    char config_server[MAX_SERVER_SOCKET]; 
    int config_port[MAX_SERVER_SOCKET]; 
    int dim_arr = fill_arr_socket(config_port, config_server);

    // Creazione del socket
    // Poi interroga tutte le biblioteche connettendosi sulla socket e mandando una richiesta
    int socket_arr[dim_arr], csfd; // array di socket e socket del client
    struct sockaddr_in server_addr[dim_arr]; // array di indirizzi dei server
    for (int i = 0; i < dim_arr; i++){
        server_addr[i].sin_family = AF_INET; 
        server_addr[i].sin_addr.s_addr = inet_addr(IP); 
        server_addr[i].sin_port = config_port[i]; 

        csfd = mysocket(AF_INET, SOCK_STREAM, 0, __LINE__, __FILE__);
        socket_arr[i] = csfd;
        myconnect(csfd, (struct sockaddr *)&server_addr[i], sizeof(server_addr[i]), __LINE__, __FILE__);        
        if(write(csfd, msg, sizeof(msg_from_client_t)) == -1){
            perror("write client");
            exit(EXIT_FAILURE);
        }
    }

    // preparo la select per le risposte dei server
    fd_set fdset, rfdset;
    int maxfd = 0;
    FD_ZERO(&fdset);
    FD_ZERO(&rfdset);
    for (int i = 0; i < dim_arr; i++){
        FD_SET(socket_arr[i], &fdset);
        if (socket_arr[i] > maxfd)
            maxfd = socket_arr[i];
    }
    // timeout
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // Connessione al server
    while(1){
        rfdset = fdset;
        if(select(maxfd + 1, &rfdset, NULL, NULL, &timeout) == -1){
            perror("select");
            exit(EXIT_FAILURE);
        }
        int finished = 1;
        for(int i=0; i<dim_arr; i++){
            setsockopt(socket_arr[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
            int bytes_read = 0;
            if(FD_ISSET(socket_arr[i], &rfdset)){
                finished = 0;
                msg_from_client_t *msg = (msg_from_client_t *)malloc(sizeof(msg_from_client_t));
                if(msg==NULL){
                    perror("Errore allocazione memoria msg del client");
                    exit(EXIT_FAILURE);
                }
                while((bytes_read = read(socket_arr[i], msg, sizeof(msg_from_client_t))) > 0){
                    if(msg->type == MSG_RECORD){
                        printf("%s\n", msg->data);
                    }
                    else if(msg->type == MSG_NO){
                        printf("Nessun risultato trovato\n");
                        break;
                    }
                    else if(msg->type == MSG_ERROR){
                        printf("Errore: %s\n", msg->data);
                        break;
                    }
                }
                free(msg);
            }
            FD_CLR(socket_arr[i], &fdset);
        }
        if(finished == 1)
            break;
    }

    // Chiudo i socket

    // Libero la memoria
    free_list(lista_arg);

return 0;
}

