#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

#define MAX_PROCESS 10000
#define MAX_SIMULATION_TIME 50
#define HEARTBEAT_INTERVAL 10.0    // Intervalo entre heartbeats
#define TIMEOUT_INTERVAL 20.0      // Timeout para detectar falha
#define RECOVERY_HEARTBEAT_DELAY 1.0

typedef enum {
	EVENT_HEARTBEAT_TIMEOUT = 1,   // Evento para enviar heartbeat
	EVENT_TEST_TIMEOUT = 2,        // Evento para verificar timeout de heartbeats
	EVENT_FAULT = 3,               // Evento de falha de processo
	EVENT_RECOVERY = 4,            // Evento de recuperação de processo
} event_type;

typedef enum {
	STATE_UNKNOWN = -1,
	STATE_CORRECT = 0,
	STATE_FAULT = 1,
} process_state;

typedef struct{ 
	int id;                          // ID do facility para o processo
	process_state state[MAX_PROCESS]; // Estado percebido de cada processo
	double last_heartbeat;           // Timestamp do último heartbeat recebido do monitorado
	int monitored;                   // ID do processo que este processo esta sendo monitorado (envia heartbeat para ele)
	int monitoring;                  // ID do processo que esta monitorando este processo (recebe heartbeat dele)
} process;

process *proc;

void print_state(int process_id, int num_processes) {
    printf("\n=== Estado do Processo %d ===\n", process_id);
    printf("ID\tEstado\n");
    printf("--\t------\n");
    
    for (int i = 0; i < num_processes; i++) {
        const char *state_name = 
            proc[process_id].state[i] == STATE_UNKNOWN ? "UNKNOWN" :
            proc[process_id].state[i] == STATE_CORRECT ? "CORRECT" :
            "FAULT";
            
        printf("%d\t%s\n", i, state_name);
    }
    printf("Monitorando: %d, Monitorado por: %d\n", 
           proc[process_id].monitoring, proc[process_id].monitored);
    printf("Último heartbeat recebido: %4.1f\n", proc[process_id].last_heartbeat);
    printf("========================\n\n");
}

// Função para enviar heartbeat
void handle_event_heartbeat(int event, int token, int n) {
	process *current_process = &proc[token];

    // Se o processo está falho, então não envia heartbeat
    if (status(current_process->id) != 0) {
        return;
    }

	//  Recebedor do heartbeat
	process *monitored = &proc[current_process->monitored];

	// Desroteia o processo monitorado
	if (monitored->monitoring != token) {
		int prev_monitored_monitoring = monitored->monitoring;
		monitored->monitoring = token;

		proc[prev_monitored_monitoring].monitored = token;

		current_process->monitoring = prev_monitored_monitoring;
	}

	if (status(monitored->id) == 0) {
		monitored->last_heartbeat = time();
		monitored->state[token] = STATE_CORRECT;
	}

	printf("+ ENVIO_HEARTBEAT: O processo %d enviou heartbeat para o processo %d no tempo %4.1f\n", token, current_process->monitored, time());

    // Agenda o próximo heartbeat
    schedule(EVENT_HEARTBEAT_TIMEOUT, HEARTBEAT_INTERVAL, token);
}

// Função para verificar timeout de heartbeats
void handle_event_timeout(int event, int token, int n) {
	process *current_process = &proc[token];

    // Se o processo está falho, então não verifica timeouts
    if (status(current_process->id) != 0) {
        return;
    }

	// Processo monitorado falhou
	if (current_process->last_heartbeat + TIMEOUT_INTERVAL < time()) {
		printf("- DETECAO_FALHA: O processo %d detectou falha no processo %d no tempo %4.1f\n", token, current_process->monitoring, time());

		// Atualiza o estado do processo que esta monitorando o processo monitorado
		current_process->state[current_process->monitoring] = STATE_FAULT;

		// Realiza o rerouting
		current_process->monitoring = proc[current_process->monitoring].monitoring;
		proc[current_process->monitoring].monitored = token;

	} else {
		printf("- RECEBIMENTO_HEARTBEAT: O processo %d recebeu heartbeat dentro do timeout no tempo %4.1f\n", token, time());
	}

    // Agenda a próxima verificação de timeout
    schedule(EVENT_TEST_TIMEOUT, TIMEOUT_INTERVAL, token);
}

void handle_event_fault(int event, int token) {
    int r = request(proc[token].id, token, 0);
    printf("- FALHA: O processo %d falhou no tempo %4.1f\n", token, time());
}

void handle_event_recovery(int event, int token, int n) {
    release(proc[token].id, token);
    printf("+ RECUPERACAO: O processo %d recuperou no tempo %4.1f\n", token, time());
    
    // Reinicia os heartbeats e verificações de timeout para este processo
    schedule(EVENT_HEARTBEAT_TIMEOUT, RECOVERY_HEARTBEAT_DELAY, token);
    schedule(EVENT_TEST_TIMEOUT, TIMEOUT_INTERVAL, token);
}

void setup_processes(int n) {
	static char fa_name[5];

    proc = (process *) malloc(sizeof(process)*n);
    for (int i=0; i<n; i++) {
        memset(fa_name, '\0', 5);
        sprintf(fa_name, "%d", i);
        proc[i].id = facility(fa_name, 1);

        // Inicializa estados
        for (int j=0; j<n; j++) {
            proc[i].state[j] = STATE_UNKNOWN;
        }

		proc[i].state[i] = STATE_CORRECT;
        
        // Inicializa a estrutura do anel virtual
        proc[i].monitoring = (i + 1) % n;           // Cada processo monitora o próximo
        proc[i].monitored = (i - 1 + n) % n;       // Cada processo é monitorado pelo anterior
        proc[i].last_heartbeat = 0.0;              // Inicializa o último heartbeat recebido
    }
}

void setup_events(int n) {
    for (int i=0; i<n; i++) {
        schedule(EVENT_HEARTBEAT_TIMEOUT, HEARTBEAT_INTERVAL, i);
        schedule(EVENT_TEST_TIMEOUT, TIMEOUT_INTERVAL, i);
    }
    
	// Teste de falha e recuperação
    schedule(EVENT_FAULT, 11.0, 1);
    // schedule(EVENT_RECOVERY, 22.0, 1);
}

int main(int argc, char **argv) {
    int token, event;

    if (argc != 2) {
        puts("Uso correto: ./heartbeat <número de processos>");
        exit(1);
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        puts("O número de processos deve ser maior que zero");
        exit(1);
    }

    smpl(0, "Simulação de Detecção de Falhas com Heartbeats em VRing");
    reset();
    stream(1);

	setup_processes(n);

	setup_events(n);

    // Loop principal do simulador:
    puts("===============================================================");
    puts("           Sistemas Distribuídos Prof. Elias");
    puts("          LOG do Trabalho prático 0, Tarefa 4");
    puts("     Implementação de Heartbeats em Anel Virtual (VRing)");
    printf("   Este programa foi executado para: N=%d processos.\n", n); 
    printf("           Tempo Total de Simulação = %d\n", MAX_SIMULATION_TIME);
    puts("===============================================================");

    while (time() < MAX_SIMULATION_TIME) {
        cause(&event, &token);
        switch (event) {
            case EVENT_HEARTBEAT_TIMEOUT:
                handle_event_heartbeat(event, token, n);
				print_state(token, n);
                break;
                
            case EVENT_TEST_TIMEOUT:
                handle_event_timeout(event, token, n);
				print_state(token, n);
                break;
                
            case EVENT_FAULT:
                handle_event_fault(event, token);
				print_state(token, n);
                break;
                
            case EVENT_RECOVERY:
                handle_event_recovery(event, token, n);
				print_state(token, n);
                break;
        }
    }
    
    free(proc);
    return 0;
}