#include "libs/aux_function.h"
#include "libs/socket.h"
#include "libs/linked_list.h"
#include "libs/file.h"
#include "libs/queue.h"
#include "libs/mutx.h"
#include "libs/sig.h"
#include "libs/thread.h"

#define CONFIG_FILE "bib.conf"
#define MAX_SERVER_SOCKET 5

/*
7) Ricezione della risposta dal server: 
Utilizzare la funzione recv per ricevere la risposta dal server.
Il client dovrebbe essere in grado di gestire i vari tipi di messaggi di risposta 
(MSG_RECORD, MSG_NO, MSG_ERROR).

8) Elaborazione della risposta: 
Analizzare la risposta ricevuta dal server e stampare i risultati su stdout.
Se la risposta è un MSG_ERROR, stampare anche il messaggio di errore. 
Se la risposta è un MSG_RECORD e l'opzione -p è stata specificata, -> 
stampare solo i record per cui è stato accordato il prestito.

9) Chiusura del socket: 
Utilizzare la funzione close per chiudere il socket quando il cliente ha terminato
*/

//$ ./bibclient --author="ciccio" -p 
//$ ./bibclient --author="ciccio" --title="pippo" 

int main(int argc, char *argv[]){
    //________Parsing degli argomenti della linea di comando________
    if(argc<2){
        fprintf(stderr, "%s Usage: ./biblient [--field]... [-p]\n\n[--field]: What are you searching for [-p]: if you want to loan", argv[0]);
        exit(1);
    }
    if(argc==2 && strcmp(argv[1], "-p")==0){
        fprintf(stderr, "%s Usage: ./biblient [--field]... [-p]\n\n[--field]: What are you searching for [-p]: if you want to loan", argv[0]);
        exit(1);
    }

    // Creazione della lista di richieste del client e restituisce se è stato richiesto un prestito o meno
    linked_list_t *lista_arg = (linked_list_t*)malloc(sizeof(linked_list_t));
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
        strcat(data, req->etichetta);
        strcat(data, ":");
        strcat(data, req->valore);
        strcat(data, ";");
    }

    msg_client_t *msg = (msg_client_t *)malloc(sizeof(msg_client_t));
    if(msg==NULL){
        perror("Errore allocazione memoria");
        exit(1);
    }
    msg->type = type;
    msg->length = dim;
    strcpy(msg->data, data); 

    // Inserisco i dati del file di configurazione in un array di stringhe e un array di interi 
    FILE *fd = myfopen(CONFIG_FILE, "r", __LINE__, __FILE__);
    char arr_server[MAX_SERVER_SOCKET]; 
    int arr_port[MAX_SERVER_SOCKET]; 
    int dim_arr = fill_arr_socket(fd, arr_port, arr_server);

    // Creazione del socket
    // Poi interroga tutte le biblioteche connettendosi sulla socket e mandando una richiesta
    int socket_arr[dim_arr], csfd; // array di socket e socket del client
    struct sockaddr_in server_addr[dim_arr]; // array di indirizzi dei server
    for (int i = 0; i < dim_arr; i++){
        server_addr[i].sin_family = AF_INET; 
        server_addr[i].sin_addr.s_addr = inet_addr(IP); 
        server_addr[i].sin_port = arr_port[i]; 

        csfd = mysocket(AF_INET, SOCK_STREAM, 0, __LINE__, __FILE__);
        socket_arr[i] = csfd;
        myconnect(csfd, (struct sockaddr *)&server_addr[i], sizeof(server_addr[i]), __LINE__, __FILE__);
        if(write(csfd, msg, sizeof(msg_client_t)) == -1){
            perror("write");
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
        for (int i = 0; i < dim_arr; i++){
            setsockopt(socket_arr[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
            if (FD_ISSET(socket_arr[i], &rfdset)){
                finished = 0;
                printf("arr_server: %c\n", arr_server[i]);
                while(1){
                    msg_client_t *msg = (msg_client_t *)malloc(sizeof(msg_client_t));
                    if(msg==NULL){
                        perror("Errore allocazione memoria");
                        exit(1);
                    }
                    printf("messaggio allocato\n");
                    int byte_read = 0;
                    if((byte_read = read(socket_arr[i], msg, sizeof(msg_client_t))) == -1){
                        perror("niente da leggere");
                        break;
                    }
                    if(byte_read == 0){
                        printf("byte_read: %d\n", byte_read);
                        break;
                    }
                    printf("byte_read: %d\n", byte_read);
                    if(msg->type == MSG_RECORD){
                        printf("%s\n", msg->data);
                    }
                    if(msg->type == MSG_ERROR){
                        fprintf(stderr, "Errore: %s\n", msg->data);
                        break;
                    }
                    if(msg->type == MSG_NO){
                        printf("Nessun risultato\n");
                        break;
                    }
                    safe_free(msg);
                    printf("messaggio ricevuto liberato\n\n");
                }
                //safe_free(msg);
                //myclose(socket_arr[i], __LINE__, __FILE__);
            }
        }
        if(finished){
            break;
        }   
    }

    //myclose(csfd, __LINE__, __FILE__); // chiudo il socket
    //myfclose(fd, __LINE__, __FILE__);


    

return 0;
}

