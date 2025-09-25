# Cyber Flux: Simulação de um Cyber Café com Controle de Concorrência

## 📝 Descrição

`Cyber Flux` é um projeto desenvolvido em C que simula o ambiente de um cyber café, gerenciando a alocação de recursos compartilhados (Computadores, Óculos de Realidade Virtual e Cadeiras) entre diferentes tipos de clientes (Gamers, Freelancers e Estudantes).

O objetivo principal é demonstrar a utilização de mecanismos de concorrência, como **semáforos** e **mutexes**, para garantir o acesso seguro aos recursos e, crucialmente, para **prevenir condições de *deadlock***. A simulação coleta estatísticas sobre a utilização dos recursos, tempo de espera e satisfação dos clientes.

Este projeto foi desenvolvido como parte dos estudos sobre sistemas operacionais, explorando a complexidade do gerenciamento de processos e recursos em um ambiente multithread.

## 🚀 Funcionalidades

* **Simulação de Clientes**: O sistema cria diferentes tipos de clientes (Gamer, Freelancer, Estudante), cada um com necessidades de recursos distintas.
* **Gerenciamento de Recursos**: Controla o acesso a um número limitado de PCs, VRs e cadeiras.
* **Controle de Concorrência**: Utiliza `semáforos` para gerenciar o acesso aos recursos e `mutexes` para proteger o acesso a dados estatísticos compartilhados entre as threads.
* **Prevenção de Deadlock**: A lógica de aquisição de recursos foi cuidadosamente implementada para evitar impasses. Um cliente só retém os recursos se conseguir alocar *todos* os que precisa, liberando-os caso contrário e tentando novamente mais tarde.
* **Coleta de Estatísticas**: Ao final da simulação, o programa gera um relatório detalhado sobre:
    * Número total de clientes.
    * Número de clientes atendidos e não atendidos.
    * Tempo médio de espera.
    * Taxa de utilização e tempo total de uso para cada tipo de recurso.
    * Detalhamento do uso de recursos por perfil de cliente.

## 🛠️ Tecnologias Utilizadas

* **Linguagem**: C
* **Bibliotecas**:
    * `pthread`: Para criação e gerenciamento de threads (clientes).
    * `semaphore.h`: Para a implementação de semáforos no controle de recursos.
    * `time.h` e `unistd.h`: Para controle de tempo e da lógica da simulação.

## 🧑‍💻 Perfis de Cliente e Suas Necessidades

O cyber café atende a três perfis de clientes, cada um com uma demanda específica de recursos:

1.  **Gamer**: Precisa de 1 PC, 1 Óculos VR e 1 Cadeira.
2.  **Freelancer**: Precisa de 1 PC, 1 Óculos VR e 1 Cadeira. (A lógica de aquisição no código difere da do Gamer, testando outras ordens de alocação).
3.  **Estudante**: Precisa apenas de 1 PC.

## 🔬 Lógica de Prevenção de Deadlock

A principal estratégia para evitar deadlock neste projeto é a política de **alocação "tudo-ou-nada"** de forma não bloqueante.

Quando um cliente (thread) tenta alocar os recursos de que precisa, ele utiliza a função `sem_trywait()`. Se um recurso não estiver disponível, em vez de esperar (o que poderia levar a um *hold and wait*), a thread libera imediatamente todos os recursos que conseguiu adquirir naquela tentativa e entra em um estado de espera por um tempo aleatório antes de tentar novamente.

Isso quebra a condição de "espera circular" e "segurar e esperar", que são duas das quatro condições necessárias para a ocorrência de um deadlock. O código inclui um teste específico no início da função `main` para forçar uma situação de alta contenção de recursos e demonstrar que o sistema se resolve sem travar.

## ⚙️ Como Compilar e Executar

Este projeto utiliza a biblioteca `pthread`, então é necessário linká-la durante a compilação.

1.  **Clone o repositório:**
    ```bash
    git clone https://github.com/kerwald/cyber-flux
    cd cyber-flux
    ```

2.  **Compile o programa:**
    Use o GCC (ou outro compilador C) para compilar o arquivo, incluindo a flag `-pthread`.
    ```bash
    gcc -o cyber_flux main.c -pthread
    ```

3.  **Execute a simulação:**
    ```bash
    cyber_flux
    ```

A simulação irá rodar por um tempo pré-determinado no código (equivalente a 8 horas de funcionamento do cyber café, acelerado pela `SIMULATION_TIME_RATIO`) e, ao final, exibirá o relatório com as estatísticas.

## 📊 Exemplo de Saída (Relatório Final)
=== RELATÓRIO FINAL ===  
Total de clientes: 155  
Clientes atendidos: 108  
Clientes não atendidos: 47  
Tempo médio de espera: 0.94 horas  
Utilização de recursos:  
  PCs: 108  
  VRs: 61  
  Cadeiras: 61  
Tempo total de uso dos recursos:  
  Uso total por tipo de recurso:  
    PCs: 67.21 horas  
    VRs: 39.34 horas  
    Cadeiras: 39.34 horas  
  
  Uso total por tipo de cliente:  
    PCs:  
      Gamer: 19.12 horas  
      Freelancer: 20.22 horas  
      Estudante: 27.88 horas  
    VRs:  
      Gamer: 19.12 horas  
      Freelancer: 20.22 horas  
    Cadeiras:  
      Gamer: 19.12 horas  
      Freelancer: 20.22 horas  

      
