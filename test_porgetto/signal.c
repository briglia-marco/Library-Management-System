#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

int main (void) {
    sigset_t set; int sig;
    /* costruisco la maschera con solo SIGALRM */
    sigemptyset(&set);
    sigaddset(&set,SIGALRM);
    /* blocco SIGALRM */
    pthread_sigmask(SIG_SETMASK, &set, NULL);
    alarm(3); /* SIGALRM fra 3 secondi */
    printf("Inizio attesa ...\n");
    sigwait(&set, &sig);
    printf("Pippo, %d\n", sig); 
    return 0;
}
