#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_PCS 10
#define NUM_VR 6
#define NUM_CADEIRAS 8
#define NUM_CLIENTES 20

sem_t pcs, vr, cadeiras;
pthread_mutex_t mutex_print;

void* cliente(void* arg);

int main() {
    srand(time(NULL));
    pthread_t threads[NUM_CLIENTES];

    // Inicializando semáforos
    sem_init(&pcs, 0, NUM_PCS);
    sem_init(&vr, 0, NUM_VR);
    sem_init(&cadeiras, 0, NUM_CADEIRAS);
    pthread_mutex_init(&mutex_print, NULL);

    // Criando clientes (threads)
    for (int i = 0; i < NUM_CLIENTES; i++) {
        int* tipo = malloc(sizeof(int));
        *tipo = rand() % 3; // Define tipo aleatório (0, 1 ou 2)
        pthread_create(&threads[i], NULL, cliente, tipo);
        sleep(rand() % 2); // Simula chegada aleatória
    }

    // Aguardando todas as threads terminarem
    for (int i = 0; i < NUM_CLIENTES; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destruir semáforos e mutex
    sem_destroy(&pcs);
    sem_destroy(&vr);
    sem_destroy(&cadeiras);
    pthread_mutex_destroy(&mutex_print);

    return 0;
}

void* cliente(void* arg) {
    int tipo = *(int*)arg; // 0: Estudante, 1: Freelancer, 2: Gamer
    free(arg);

    if (tipo == 2) { // Gamer precisa de PC + VR
        sem_wait(&pcs);
        sem_wait(&vr);
        pthread_mutex_lock(&mutex_print);
        printf("Gamer usando PC + VR\n");
        pthread_mutex_unlock(&mutex_print);
        sleep(rand() % 3 + 1);
        sem_post(&pcs);
        sem_post(&vr);
    } else if (tipo == 1) { // Freelancer  precisa de PC + Cadeira
        sem_wait(&pcs);
        sem_wait(&cadeiras);
        pthread_mutex_lock(&mutex_print);
        printf(" Freelancer usando PC + Cadeira\n");
        pthread_mutex_unlock(&mutex_print);
        sleep(rand() % 3 + 1);
        sem_post(&pcs);
        sem_post(&cadeiras);
    } else { // Estudante  precisa apenas do PC
        sem_wait(&pcs);
        pthread_mutex_lock(&mutex_print);
        printf(" Estudante usando PC\n");
        pthread_mutex_unlock(&mutex_print);
        sleep(rand() % 3 + 1);
        sem_post(&pcs);
    }
    return NULL;
}