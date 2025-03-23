#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#define SIMULATION_TIME 8      // Horas de funcionamento
#define TOTAL_PC 10
#define TOTAL_VR 6
#define TOTAL_CHAIR 8

typedef enum {GAMER, FREELANCER, STUDENT} ClientType;

typedef struct {
    int pc;
    int vr;
    int chair;
} Resources;

typedef struct {
    Resources available;
    Resources used;
    sem_t mutex;
    sem_t condition;
    int clients_served;
    int clients_failed;
    double total_wait_time;
    time_t start_time;
} ResourceManager;

ResourceManager manager;

void init_manager() {
    manager.available.pc = TOTAL_PC;
    manager.available.vr = TOTAL_VR;
    manager.available.chair = TOTAL_CHAIR;
    manager.clients_served = 0;
    manager.clients_failed = 0;
    manager.total_wait_time = 0.0;
    sem_init(&manager.mutex, 0, 1);
    sem_init(&manager.condition, 0, 0);
    manager.start_time = time(NULL);
}

Resources get_required(ClientType type) {
    Resources req = {0};
    switch(type) {
        case GAMER:
            req.pc = 1;
            req.vr = 1;
            req.chair = 1;
            break;
        case FREELANCER:
            req.pc = 1;
            req.chair = 1;
            req.vr = 1; // Prioridade diferente na ordem de aquisição
            break;
        case STUDENT:
            req.pc = 1;
            break;
    }
    return req;
}

int try_acquire_resources(ClientType type) {
    Resources req = get_required(type);
    time_t start_wait = time(NULL);
    
    sem_wait(&manager.mutex);
    
    // Verificar se todos os recursos necessários estão disponíveis
    if(manager.available.pc >= req.pc &&
       manager.available.vr >= req.vr &&
       manager.available.chair >= req.chair) {
        
        manager.available.pc -= req.pc;
        manager.available.vr -= req.vr;
        manager.available.chair -= req.chair;
        
        manager.used.pc += req.pc;
        manager.used.vr += req.vr;
        manager.used.chair += req.chair;
        
        manager.clients_served++;
        manager.total_wait_time += difftime(time(NULL), start_wait);
        
        sem_post(&manager.mutex);
        return 1;
    }
    
    sem_post(&manager.mutex);
    return 0;
}

void release_resources(ClientType type) {
    Resources req = get_required(type);
    
    sem_wait(&manager.mutex);
    
    manager.available.pc += req.pc;
    manager.available.vr += req.vr;
    manager.available.chair += req.chair;
    
    manager.used.pc -= req.pc;
    manager.used.vr -= req.vr;
    manager.used.chair -= req.chair;
    
    sem_post(&manager.condition);
    sem_post(&manager.mutex);
}

void* client_thread(void* arg) {
    ClientType type = *(ClientType*)arg;
    
    while(!try_acquire_resources(type)) {
        sem_wait(&manager.condition);
        if(difftime(time(NULL), manager.start_time) > SIMULATION_TIME * 3600) {
            sem_wait(&manager.mutex);
            manager.clients_failed++;
            sem_post(&manager.mutex);
            return NULL;
        }
    }
    
    // Simular uso dos recursos
    unsigned seed = time(NULL);
    sleep(rand(&seed) % 5 + 1);
    
    release_resources(type);
    return NULL;
}

void print_report() {
    printf("\n=== Relatório Final ===\n");
    printf("Clientes atendidos: %d\n", manager.clients_served);
    printf("Clientes não atendidos: %d\n", manager.clients_failed);
    printf("Tempo médio de espera: %.2f segundos\n", 
           manager.total_wait_time / manager.clients_served);
    printf("Utilização de recursos:\n");
    printf("- PCs: %d\n", manager.used.pc);
    printf("- VRs: %d\n", manager.used.vr);
    printf("- Cadeiras: %d\n", manager.used.chair);
}

int main() {
    srand(time(NULL));
    init_manager();
    
    pthread_t threads[100];
    int thread_count = 0;
    
    time_t start = time(NULL);
    
    while(difftime(time(NULL), start) < SIMULATION_TIME * 3600) {
        ClientType type = rand() % 3;
        pthread_create(&threads[thread_count], NULL, client_thread, &type);
        thread_count++;
        
        // Intervalo aleatório entre chegada de clientes
        usleep(rand() % 1000000 + 500000); // 0.5-1.5 segundos
    }
    
    // Aguardar threads finalizarem
    for(int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    print_report();
    
    sem_destroy(&manager.mutex);
    sem_destroy(&manager.condition);
    
    return 0;
}