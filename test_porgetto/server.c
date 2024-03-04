#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define UNIX_PATH_MAX 108 
#define SOCKNAME "./mysock" 
#define N 100


int main(void){
    int fd_skt, fd_c; //file descriptor socket e client
    struct sockaddr_un sa; //indirizzo socket
    strncpy(sa.sun_path, SOCKNAME, sizeof(sa.sun_path)); //copio il path del socket    
    sa.sun_family = AF_UNIX; //famiglia AF_UNIX
    char buf[N]; //buffer per la lettura e scrittura sul socket


    if(fork() != 0){ //padre
        fd_skt = socket(AF_UNIX, SOCK_STREAM, 0); //creo il socket
        bind(fd_skt, (struct sockaddr *)&sa, sizeof(sa)); //lo associo all'indirizzo
        listen(fd_skt, SOMAXCONN); //metto il socket in ascolto
        fd_c = accept(fd_skt, NULL, 0); //accetto la connessione
        read(fd_c, buf, N); //leggo dal socket
        printf("Server got: %s\n", buf); //stampa "Server got: Ciao!"
        write(fd_c, "Ciao bello!", 11); //scrivo sul socket
        close(fd_skt); //chiudo il socket del server
        close(fd_c); //chiudo il socket del client
        exit(EXIT_SUCCESS); //padre terminato
    }
    else{ //figlio
        fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);
        while(connect(fd_skt, (struct sockaddr *)&sa, sizeof(sa)) == -1){ //mi connetto al socket
            if(errno == ENOENT){ //se il socket non esiste, riprovo dopo 1 secondo
                sleep(1); 
            }
            else{
                perror("connect"); //errore
                exit(EXIT_FAILURE); 
            }
        }
        write(fd_skt, "Ciao!", 5); //scrivo sul socket
        read(fd_skt, buf, N); //leggo dal socket
        printf("Client got: %s\n", buf); //stampa "Client got: Ciao!"
        close(fd_skt); //chiudo socket
        exit(EXIT_SUCCESS); //figlio terminato
    }

}
