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
	int prev;                   // ID do processo que este processo esta sendo monitorado (envia heartbeat para ele)
	int next;                   // ID do processo que esta monitorando este processo (recebe heartbeat dele)
} process;

process *proc;

void print_state(int token, int n) {
    printf("\n=== Estado do Processo %d ===\n", token);
    printf("ID\tEstado\n");
    printf("--\t------\n");
    
    for (int i = 0; i < n; i++) {
        const char *state_name;
		switch (proc[token].state[i]) {
			case STATE_UNKNOWN:
				state_name = "UNKNOWN";
				break;
			case STATE_CORRECT:
				state_name = "CORRECT";
				break;
			case STATE_FAULT:
				state_name = "FAULT";
				break;
			default:
				state_name = "UNKNOWN";
				break;
		}
            
        printf("%d\t%s\n", i, state_name);
    }
    printf("Monitorando: %d, Monitorado por: %d\n", proc[token].next, proc[token].prev);
    printf("Último heartbeat recebido: %4.1f\n", proc[token].last_heartbeat);
    printf("========================\n\n");
}

// Trata o recebimento do heartbeat do processo @token.
void handle_event_heartbeat(int event, int token, int n) {
	process *curr = &proc[token];

    // Se o processo está falho, então não envia heartbeat:
    if (status(curr->id) != 0) {
        return;
    }

	// Processo monitor do processo atual:
	process *curr_monitor = &proc[curr->prev];

	// Se o monitor do processo atual não está monitorando o processo atual,
	// então significa que o processo atual falhou em algum momento, mas agora
	// está vivo novamente. Logo, precisamos arrumar os "ponteiros" para
	// novamente testá-lo:
	if (curr_monitor->next != token) {
		int last_monitored_by_curr_monitor = curr_monitor->next;
		curr_monitor->next = token;
		proc[last_monitored_by_curr_monitor].prev = token;
		curr->next = last_monitored_by_curr_monitor;
	}

	// Marca o recebimento do heartbeat de @token no estado de seu monitor:
	if (status(curr_monitor->id) == 0) {
		curr_monitor->last_heartbeat = time();
		curr_monitor->state[token] = STATE_CORRECT;
	}

	printf("> HEARTBEAT: O processo %d enviou heartbeat para o processo %d no tempo %4.1f\n", token, curr->prev, time());

    // Agenda o próximo heartbeat:
    schedule(EVENT_HEARTBEAT_TIMEOUT, HEARTBEAT_INTERVAL, token);
}

// O processo de token @token testa o processo que está monitorando.
void handle_event_timeout(int event, int token, int n) {
	process *curr = &proc[token];

    // Se o processo está falho, então não verifica:
    if (status(curr->id) != 0) {
        return;
    }

	// Se o processo que o processo atual testa falhou:
	if ((curr->last_heartbeat + TIMEOUT_INTERVAL) < time()) {
		printf("> TESTE: O processo %d detectou falha no processo %d no tempo %4.1f\n", token, curr->next, time());

		// Atualiza o estado do processo que está monitorando:
		curr->state[curr->next] = STATE_FAULT;

		// Realiza o rerouting:

		// Agora o processo atual testa o próximo do próximo e esse é testado
		// pelo atual:
		curr->next = proc[curr->next].next;
		proc[curr->next].prev = token;
	} else {
		printf("> TESTE: O processo %d detectou que o processo %d está correto no tempo %4.1f\n", token, curr->next, time());
		curr->state[curr->next] = STATE_CORRECT;
	}

    // Agenda a próxima verificação de timeout:
    schedule(EVENT_TEST_TIMEOUT, TIMEOUT_INTERVAL, token);
}

void handle_event_fault(int event, int token) {
    int r = request(proc[token].id, token, 0);
    printf("> FALHA: O processo %d falhou no tempo %4.1f\n", token, time());
}

void handle_event_recovery(int event, int token, int n) {
    release(proc[token].id, token);
    printf("> RECUPERACAO: O processo %d recuperou no tempo %4.1f\n", token, time());
    
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

        // Inicializa estados:
        for (int j=0; j<n; j++) {
            proc[i].state[j] = STATE_UNKNOWN;
        }

		proc[i].state[i] = STATE_CORRECT;
        
        // Inicializa a estrutura do anel virtual:
		//
		// Cada processo monitora o próximo:
        proc[i].next = (i + 1) % n;
		// Cada proceso é monitorado pelo anterior:
        proc[i].prev = (i - 1 + n) % n;
		// Cada processo mantém o tempo que recebeu o último heartbeat:
        proc[i].last_heartbeat = 0.0;
    }
}

void setup_events(int n) {
    for (int i=0; i<n; i++) {
        schedule(EVENT_HEARTBEAT_TIMEOUT, HEARTBEAT_INTERVAL, i);
        schedule(EVENT_TEST_TIMEOUT, TIMEOUT_INTERVAL, i);
    }
    
	// Teste de falha e recuperação:
    schedule(EVENT_FAULT, 0.0, 2);
    schedule(EVENT_FAULT, 40.0, 3);
    schedule(EVENT_FAULT, 80.0, 4);
    schedule(EVENT_FAULT, 120.0, 5);
    schedule(EVENT_RECOVERY, 140.0, 4);
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
                break;
                
            case EVENT_TEST_TIMEOUT:
                handle_event_timeout(event, token, n);
                break;
                
            case EVENT_FAULT:
                handle_event_fault(event, token);
                break;
                
            case EVENT_RECOVERY:
                handle_event_recovery(event, token, n);
                break;
        }
		print_state(token, n);
    }
    
    free(proc);
    return 0;
}
