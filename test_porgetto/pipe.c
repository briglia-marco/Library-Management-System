#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

#define N 100

int fibonacci(int n){
    if(n<2){
        return n;
    }
    else 
        return fibonacci(n-1) + fibonacci(n-2);
}

int main(void){
    int fd[2], l, z; //file descriptor pipe
    char buf[N]; //buffer per la lettura e scrittura sulla pipe

    if(pipe(fd) == -1){ //creo la pipe
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid_t pid;
    if((pid = fork()) == -1){ //creo un processo figlio
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(pid){
        close(fd[1]); //chiudo la scrittura
        l = read(fd[0], buf, N); //leggo dalla pipe
        if(l == -1){
            perror("read");
            exit(EXIT_FAILURE);
        }
        printf("Padre: letti %d caratteri con scritto %s\n", l, buf); //stampa
        close(fd[0]); //chiudo la lettura
        exit(EXIT_SUCCESS);
    }
    else{
        close(fd[0]); //chiudo la lettura
        z = getpid(); //pid del figlio
        int x = snprintf(buf, N, "%d", z); //scrivo su buf il pid del figlio
        l = write(fd[1], buf, x); //scrivo sulla pipe buf esattamente x caratteri
        if(l == -1){    
            perror("write");
            exit(EXIT_FAILURE);
        }
        close(fd[0]); //chiudo la scrittura
        exit(EXIT_SUCCESS);
    }
    return 0;
}
