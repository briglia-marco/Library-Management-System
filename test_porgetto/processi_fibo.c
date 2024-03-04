//scrivere fibonacci con 2 processi che calcolano le chiamate ricorsive

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int dofib(int, int);

int main(int argc, char* argv[]){
    if(argc != 3){
        fprintf(stderr, "usa: %s n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]); //numero su cui farò fibonacci
    int doprint = atoi(argv[2]); //se 1 stampo il risultato

    if(n < 0 || n > 14){
        fprintf(stderr, "n deve essere >= 0 e <= 14\n");
        exit(EXIT_FAILURE);
    }

    dofib(n, doprint); //chiamo la funzione ricorsiva
    return 0;
}

int dofib(int n, int doprint){
    pid_t pid1, pid2; //pid dei processi figli
    int tot=n;
    int res_pid;

    if(n>=2){
        if((pid1 = fork()) == 0){
            res_pid = dofib(n-1, 0);
            exit(res_pid);
        }
        else if(pid1 == -1){
            exit(EXIT_FAILURE);
        }

        if((pid2 = fork()) == 0){
            res_pid = dofib(n-2, 0);
            exit(res_pid);
        }
        else if(pid2 == -1){
            exit(EXIT_FAILURE);
        }

        int status;
        if(waitpid(pid1, &status, 0) == -1){
            exit(EXIT_FAILURE);
        }
        tot = WEXITSTATUS(status);

        if(waitpid(pid2, &status, 0) == -1){
            exit(EXIT_FAILURE);
        }
        tot += WEXITSTATUS(status);
    }

    if(doprint)
        printf("risultato: %d\n", tot);
    exit(tot);
}
