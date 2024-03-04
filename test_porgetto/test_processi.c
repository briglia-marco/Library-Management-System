#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]){
    
    if(argc != 2){  //se il numero di argomenti è diverso da 2
        fprintf(stderr, "usa: %s nomedir\n", argv[0]);  //stampa su stderr
        exit(EXIT_FAILURE); //esci con errore
    }

    const char* dir_name = argv[1]; //nome della directory
    struct stat statbuf; //struttura per le informazioni sul file

    if(stat(dir_name, &statbuf) == -1){ //se non è possibile aprire la directory
        perror("stat"); //stampa su stderr
        fprintf(stderr, "impossibile aprire la directory %s\n", dir_name); //stampa su stderr
        exit(EXIT_FAILURE);
    } 

    if(!S_ISDIR(statbuf.st_mode)){ //se non è una directory
        fprintf(stderr, "%s non è una directory\n", dir_name); //stampa su stderr
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork(); //crea un processo figlio
    if(pid == 0){  //processo figlio
        execlp("ls", "ls", "-al", dir_name, (char*)NULL); //esegui ls -al dir_name
        perror("execlp"); //stampa su stderr
        exit(EXIT_FAILURE);
    }
    //processo padre
    int status; //stato del processo figlio
    if(waitpid(pid, &status, 0) == -1) { //attendi la terminazione del processo figlio 
        perror("waitpid"); //stampa su stderr
        exit(EXIT_FAILURE);
    }

    if(WIFEXITED(status)){ //se il processo figlio è terminato normalmente
        printf("il processo figlio è terminato con stato %d\n", WEXITSTATUS(status)); //stampa su stdout
    } 
    else{
        printf("il processo figlio è terminato in modo anomalo\n"); //stampa su stdout
    }

}
