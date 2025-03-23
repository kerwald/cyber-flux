#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include <stdbool.h>

#ifdef __INTELLISENSE__
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif // CLOCK_MONOTONIC
#endif // __INTELLISENSE__

// Razão de tempo de simulação
#define SIMULATION_TIME_RATIO (1.0/1000.0)

// Quantidade de recursos
#define NUM_PCS 10
#define NUM_VR 6
#define NUM_CADEIRAS 8

// Tipos de clientes
typedef enum {
    GAMER,
    FREELANCER,
    ESTUDANTE
} TipoCliente;

typedef struct _tempoUsoRecursos_t {
    double pc;
    double vr;
    double cadeira;
} tempoUsoRecursos_t;

// Semáforos para controle de recursos
sem_t semPcs;
sem_t semVr;
sem_t semCadeiras;

// Variáveis globais para estatísticas
int totalClientes = 0;
int clientesAtendidos = 0;
int clientesFalharam = 0;
double tempoEsperaTotal = 0.0;
tempoUsoRecursos_t tempoUsoTotalRecursosGamer = {0.0, 0.0, 0.0};
tempoUsoRecursos_t tempoUsoTotalRecursosFreelancer = {0.0, 0.0, 0.0};
tempoUsoRecursos_t tempoUsoTotalRecursosEstudante = {0.0, 0.0, 0.0};
int usoPcs = 0;
int usoVr = 0;
int usoCadeiras = 0;

// Mutexes para proteger variáveis compartilhadas
pthread_mutex_t mutexStats = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPcs = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexVr = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCadeiras = PTHREAD_MUTEX_INITIALIZER;

// Controle de simulação
pthread_mutex_t mutexSimulacao = PTHREAD_MUTEX_INITIALIZER;
bool simulacaoRodando = true;
struct timespec fimSimulacao;

// Função para obter tempo atual em segundos
double tempoAtual() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Função para obter tempo atual em nanossegundos
long tempoAtualNs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

// Função para dormir um tempo aleatório entre min e max segundos
void dormirAleatorio(double min, double max) {
    double duracao = min + (max - min) * (rand() / (double)RAND_MAX);
    usleep(duracao * 1e6);
}

// Função para tentar adquirir recursos com verificação de segurança
int adquirirRecursos(TipoCliente tipo, struct timespec inicio) {
    int adquiridos = 0;
    int pc = 0, vr = 0, cadeira = 0;

    while (true) {
        // Verificar se a simulação terminou
        pthread_mutex_lock(&mutexSimulacao);
        if (!simulacaoRodando) {
            pthread_mutex_unlock(&mutexSimulacao);
            return -1;
        }
        pthread_mutex_unlock(&mutexSimulacao);

        // Tentar adquirir recursos conforme o tipo
        switch (tipo) {
            case GAMER:
            if (sem_trywait(&semPcs) == 0) {
                pc = 1;
                printf("Gamer adquiriu PC\n");

                if (sem_trywait(&semVr) == 0) {
                vr = 1;
                printf("Gamer adquiriu VR\n");

                if (sem_trywait(&semCadeiras) == 0) {
                    cadeira = 1;
                    printf("Gamer adquiriu Cadeira\n");

                    pthread_mutex_lock(&mutexPcs);
                    usoPcs++;
                    pthread_mutex_unlock(&mutexPcs);

                    pthread_mutex_lock(&mutexVr);
                    usoVr++;
                    pthread_mutex_unlock(&mutexVr);

                    pthread_mutex_lock(&mutexCadeiras);
                    usoCadeiras++;
                    pthread_mutex_unlock(&mutexCadeiras);

                    return 1;
                } else {
                    printf("Gamer não conseguiu Cadeira. Liberando recursos...\n");
                    if (vr) {
                        sem_post(&semVr);
                    }
                    
                    if (pc) {
                        sem_post(&semPcs);
                    }
                }
                } else {
                    printf("Gamer não conseguiu VR. Liberando PC...\n");
                    if (pc) {
                        sem_post(&semPcs);
                    }
                }
            }
            break;

            case FREELANCER:
            if (sem_trywait(&semPcs) == 0) {
                pc = 1;
                printf("Freelancer adquiriu PC\n");

                if (sem_trywait(&semCadeiras) == 0) {
                cadeira = 1;
                printf("Freelancer adquiriu Cadeira\n");

                if (sem_trywait(&semVr) == 0) {
                    vr = 1;
                    printf("Freelancer adquiriu VR\n");

                    pthread_mutex_lock(&mutexPcs);
                    usoPcs++;
                    pthread_mutex_unlock(&mutexPcs);

                    pthread_mutex_lock(&mutexCadeiras);
                    usoCadeiras++;
                    pthread_mutex_unlock(&mutexCadeiras);

                    pthread_mutex_lock(&mutexVr);
                    usoVr++;
                    pthread_mutex_unlock(&mutexVr);

                    return 1;
                } else {
                    printf("Freelancer não conseguiu VR. Liberando recursos...\n");
                    if (cadeira) {
                        sem_post(&semCadeiras);
                    }
                    
                    if (pc) {
                        sem_post(&semPcs);
                    }
                }
                } else {
                    printf("Freelancer não conseguiu Cadeira. Liberando PC...\n");
                    if (pc) {
                        sem_post(&semPcs);
                    }
                }
            }
            break;

            case ESTUDANTE:
            if (sem_trywait(&semPcs) == 0) {
                pthread_mutex_lock(&mutexPcs);
                usoPcs++;
                pthread_mutex_unlock(&mutexPcs);
                return 1;
            }
            break;
        }

        // Esperar de 5 a 15 minutos antes de tentar novamente
        dormirAleatorio(5 * 60 * SIMULATION_TIME_RATIO, 15 * 60 * SIMULATION_TIME_RATIO);
    }
}

// Thread do cliente
void *cliente(void *arg) {
    TipoCliente tipo = *(TipoCliente *)arg;
    free(arg);

    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    int resultado = adquirirRecursos(tipo, inicio);

    if (resultado == 1) {
        // Registrar sucesso
        clock_gettime(CLOCK_MONOTONIC, &fim);
        double espera = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;

        pthread_mutex_lock(&mutexStats);
        clientesAtendidos++;
        tempoEsperaTotal += espera;
        pthread_mutex_unlock(&mutexStats);

        // Simular uso dos recursos
        double tempoUso = (rand() % (45 * 60) + 15 * 60) * SIMULATION_TIME_RATIO; // Tempo de uso entre 15 e 60 minutos
        usleep(tempoUso * 1e6);

        // Atualizar tempo de uso total dos recursos
        pthread_mutex_lock(&mutexStats);
        switch (tipo) {
            case GAMER:
                tempoUsoTotalRecursosGamer.pc += tempoUso;
                tempoUsoTotalRecursosGamer.vr += tempoUso;
                tempoUsoTotalRecursosGamer.cadeira += tempoUso;
                break;
            case FREELANCER:
                tempoUsoTotalRecursosFreelancer.pc += tempoUso;
                tempoUsoTotalRecursosFreelancer.vr += tempoUso;
                tempoUsoTotalRecursosFreelancer.cadeira += tempoUso;
                break;
            case ESTUDANTE:
                tempoUsoTotalRecursosEstudante.pc += tempoUso;
                break;
        }
        pthread_mutex_unlock(&mutexStats);

        // Liberar recursos
        switch (tipo) {
            case GAMER:
                sem_post(&semCadeiras);
                sem_post(&semVr);
                sem_post(&semPcs);
                break;
            case FREELANCER:
                sem_post(&semVr);
                sem_post(&semCadeiras);
                sem_post(&semPcs);
                break;
            case ESTUDANTE:
                sem_post(&semPcs);
                break;
        }
    } else {
        // Registrar falha
        pthread_mutex_lock(&mutexStats);
        clientesFalharam++;
        clock_gettime(CLOCK_MONOTONIC, &fim);
        double espera = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;
        tempoEsperaTotal += espera;
        pthread_mutex_unlock(&mutexStats);
    }

    return NULL;
}

int main() {
    srand(time(NULL));

    // Inicializar semáforos
    sem_init(&semPcs, 0, NUM_PCS);
    sem_init(&semVr, 0, NUM_VR);
    sem_init(&semCadeiras, 0, NUM_CADEIRAS);

    // Configurar tempo de simulação
    double duracaoSimulacao = 8 * 3600 * SIMULATION_TIME_RATIO;
    struct timespec fim;
    clock_gettime(CLOCK_MONOTONIC, &fim);
    fim.tv_sec += floor(duracaoSimulacao);
    fim.tv_nsec += (duracaoSimulacao - floor(duracaoSimulacao)) * 1e9;

    printf("Simulação iniciada (%.2f segundos reais)\n", duracaoSimulacao);

    // Teste de deadlock controlado
    printf("\n=== TESTE DE PREVENÇÃO DE DEADLOCK ===\n");
    printf("Alocando quase todos os recursos para forçar situação crítica...\n");
    for (int i = 0; i < NUM_PCS-1; i++) sem_wait(&semPcs);
    for (int i = 0; i < NUM_VR-1; i++) sem_wait(&semVr);

    TipoCliente * gamer = malloc(sizeof(TipoCliente));
    TipoCliente * freelancer = malloc(sizeof(TipoCliente));
    *gamer = GAMER;
    *freelancer = FREELANCER;
    pthread_t t1, t2;
    pthread_create(&t1, NULL, cliente, gamer);
    pthread_create(&t2, NULL, cliente, freelancer);
    totalClientes += 2;
    
    
    sleep(5); // Dar tempo para as threads tentarem
    
    // Liberar recursos reservados
    for (int i = 0; i < NUM_PCS-1; i++) sem_post(&semPcs);
    for (int i = 0; i < NUM_VR-1; i++) sem_post(&semVr);
    printf("Situação crítica resolvida sem deadlock!\n\n");

    // Loop principal de criação de clientes
    while (tempoAtualNs() < (fim.tv_sec * 1e9 + fim.tv_nsec)) {
        int tamanhoGrupo = rand() % 5 + 1; // Grupos de 1 a 5 clientes
        for (int i = 0; i < tamanhoGrupo; i++) {
            TipoCliente *tipo = malloc(sizeof(TipoCliente));
            *tipo = rand() % 3; // Tipo aleatório
            pthread_t tid;
            pthread_create(&tid, NULL, cliente, tipo);
            pthread_detach(tid);

            pthread_mutex_lock(&mutexStats);
            totalClientes++;
            pthread_mutex_unlock(&mutexStats);
        }
        // Criar clientes a cada 1-5 minutos
        dormirAleatorio(5 * 60 * SIMULATION_TIME_RATIO, 10 * 60 * SIMULATION_TIME_RATIO);
    }

    // Finalizar simulação
    pthread_mutex_lock(&mutexSimulacao);
    simulacaoRodando = false;
    clock_gettime(CLOCK_MONOTONIC, &fimSimulacao);
    pthread_mutex_unlock(&mutexSimulacao);

    sleep(5); // Garantir processamento final

    // Relatório
    printf("\n=== RELATÓRIO FINAL ===\n");
    printf("Total de clientes: %d\n", totalClientes);
    printf("Clientes atendidos: %d\n", clientesAtendidos);
    printf("Clientes não atendidos: %d\n", clientesFalharam);
    printf("Tempo médio de espera: %.2f horas\n", 
           (tempoEsperaTotal / (clientesAtendidos + clientesFalharam)) / 3600 / SIMULATION_TIME_RATIO);
    printf("Utilização de recursos:\n");
    printf("  PCs: %d\n", usoPcs);
    printf("  VRs: %d\n", usoVr);
    printf("  Cadeiras: %d\n", usoCadeiras);
    printf("Tempo total de uso dos recursos:\n");
    printf("  Uso total por tipo de recurso:\n");
    printf("    PCs: %.2f horas\n", 
           (tempoUsoTotalRecursosGamer.pc + 
            tempoUsoTotalRecursosFreelancer.pc + 
            tempoUsoTotalRecursosEstudante.pc) / (3600 * SIMULATION_TIME_RATIO));
    printf("    VRs: %.2f horas\n", 
           (tempoUsoTotalRecursosGamer.vr + 
            tempoUsoTotalRecursosFreelancer.vr) / (3600 * SIMULATION_TIME_RATIO));
    printf("    Cadeiras: %.2f horas\n", 
           (tempoUsoTotalRecursosGamer.cadeira + 
            tempoUsoTotalRecursosFreelancer.cadeira) / (3600 * SIMULATION_TIME_RATIO));

    printf("\n  Uso total por tipo de cliente:\n");
    printf("    PCs:\n");
    printf("      Gamer: %.2f horas\n", tempoUsoTotalRecursosGamer.pc / (3600 * SIMULATION_TIME_RATIO));
    printf("      Freelancer: %.2f horas\n", tempoUsoTotalRecursosFreelancer.pc / (3600 * SIMULATION_TIME_RATIO));
    printf("      Estudante: %.2f horas\n", tempoUsoTotalRecursosEstudante.pc / (3600 * SIMULATION_TIME_RATIO));
    printf("    VRs:\n");
    printf("      Gamer: %.2f horas\n", tempoUsoTotalRecursosGamer.vr / (3600 * SIMULATION_TIME_RATIO));
    printf("      Freelancer: %.2f horas\n", tempoUsoTotalRecursosFreelancer.vr / (3600 * SIMULATION_TIME_RATIO));
    printf("    Cadeiras:\n");
    printf("      Gamer: %.2f horas\n", tempoUsoTotalRecursosGamer.cadeira / (3600 * SIMULATION_TIME_RATIO));
    printf("      Freelancer: %.2f horas\n", tempoUsoTotalRecursosFreelancer.cadeira / (3600 * SIMULATION_TIME_RATIO));

    // Destruir semáforos
    sem_destroy(&semPcs);
    sem_destroy(&semVr);
    sem_destroy(&semCadeiras);

    return 0;
}