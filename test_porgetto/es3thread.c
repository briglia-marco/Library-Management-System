#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <util.h>
#include <errno.h>
#include "coda.h"

struct argt1{
    coda coda_riga;  //coda è lo struct da implementare
    coda coda_parola
};

void* tokenizza(void* arg);
void* printa(void* arg);

int main(int argc, char* argv[]){

    pthread_t thread[2];
    int error;

    struct argt1 arg1;
    arg1.coda_riga = coda_init(); //inizializzo la coda
    arg1.coda_parola = coda_init(); //inizializzo la coda

    if((error = pthread_create(&thread[0], NULL, tokenizza, &arg1)) != 0){ //passo sia la coda delle righe che delle parole
        dprintf(2, "Errore creazione thread: %s\n", strerror(error)); //dprintf stampa su stderr (2) 
        exit(1);
    }

    if((error = pthread_create(&thread[1], NULL, printa, &arg1.coda_parola)) != 0){ //passo solo la coda delle parole
        dprintf(2, "Errore creazione thread: %s\n", strerror(error)); 
        exit(1);
    }







}