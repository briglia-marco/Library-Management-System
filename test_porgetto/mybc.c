#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

#define N 100

int main(void){

    int pfd[2];
    char buf[N];

    if(pipe(pfd) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    if((pid = fork()) == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if(pid){
        close(pfd[1]); //chiudo la scrittura
        if(read(pfd[0], buf, N) == -1){ //errore lettura pipe
            perror("read");
            exit(EXIT_FAILURE);
        }
        printf("Padre: risultato da bc = %s\n", buf); 
        close(pfd[0]); //chiudo la lettura
    }
    else{
        printf("Figlio: inserisci un'espressione: \n"); //stampo il prompt
        if (dup2(pfd[1], 1) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        else {
            close(pfd[0]);  //chiudo la lettura
            close(pfd[1]);  //chiudo la scrittura
            execlp("bc", "bc", "-q", NULL); //eseguo bc con -q per non stampare il prompt e NULL per terminare la lista di argomenti
            perror("execlp");   //se fallisce la execlp stampo l'errore e termino
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
