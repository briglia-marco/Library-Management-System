#include <stdio.h>
#include <unistd.h>

int main(void) {
    pid_t pid = fork();

    if (pid == -1) {
        // Errore nella fork
        perror("Errore nella fork");
        return 1;
    } else if (pid == 0) {
        // Codice eseguito dal processo figlio
        printf("Sono il processo figlio (PID: %d) e la fork restituisce: %d\n", getpid(), pid);
    } else {
        // Codice eseguito dal processo padre
        printf("Sono il processo padre (PID: %d) e mio figlio ha PID: %d\n", getpid(), pid);
    }

    // Codice eseguito sia dal processo padre che dal processo figlio
    printf("Questo codice è eseguito da entrambi i processi (PID: %d)\n", getpid());

    return 0;
}
