#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){

    if(argc != 2){
        fprintf(stderr, "usa: %s num\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num = atoi(argv[1]);

    for(int i=0; i<num; i++){
        pid_t pid = fork();
        if(pid == 0){
            execlp("./esempio", "esempio", (char*)NULL);
            while(1){
                sleep(1);
            
            };
        }
    }
    sleep(10);
    printf("I am parent process\n");
    return 0;

}
