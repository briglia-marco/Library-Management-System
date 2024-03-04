#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <util.h>
#include <errno.h>


#define N 5
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // dichiarazione e inizializzazione mutex globale

void* fun(void* arg){

    long i = (long)arg;

    //pthread_mutex_lock(&mutex); funziona se uso j per tenere il conto dell'ordine di esecuzione
    printf("Ciao sono il thread %ld\n", i);
    //pthread_mutex_unlock(&mutex); funziona se uso j per tenere il conto dell'ordine di esecuzione

    int* res = malloc(sizeof(int)); 
    *res = 2;   
    return (void*)res;
}


int main(void){    
    /*
    pthread_mutex_t mutex; // dichiarazione mutex 
    if(pthread_mutex_init(&mutex, NULL) != 0){ // inizializzazione mutex locale
        perror("Errore inizializzazione mutex"); 
        exit(1);
    }
    */
    pthread_t thread[N]; // dichiarazione thread
    long i;
    int *res, somma = 0;

    for(i=0;i<N;i++){
        if(pthread_create(&thread[i], NULL, fun, (void*)i) != 0){
            perror("Errore creazione thread");
            exit(1);
        }
    }

    for(i=0;i<N;i++){
        if(pthread_join(thread[i], (void**)&res) != 0){
            perror("Errore join");
            exit(1);
        }
        somma += *res;
    }

    printf("Somma = %d\n", somma);

    return 0;

}

