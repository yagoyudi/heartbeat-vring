#include <stdio.h>
#include <stdlib.h>
#include "../smpl.h"

typedef enum {
	EVENT_TEST = 1,
	EVENT_FAULT = 2,
	EVENT_RECOVERY = 3
} event_type;

#define TEST_INTERVAL_TIME 10
#define TEST_AFTER_RECOVERY_TIME 1.0

#define MAX_PROCESS 10000

// Vamos definir o descritor do processo:
typedef struct{ 
	int id;
} process;

process *proc;

int main(int argc, char **argv) {
	int token, event, r, i, max_simulation_time = 150;
	static char fa_name[5];

	if (argc != 2) {
		puts("Uso correto: tempo <número de processos>");
		exit(1);
	}

	int n = atoi(argv[1]);
	if (n < 0) {
		exit(1);
	}

	smpl(0, "Um Exemplo de simulação");
	reset();
	stream(1);

	// Inicializar processos:
	proc = (process *) malloc(sizeof(process)*n);
	for (i=0; i<n; i++) {
		memset(fa_name, '\0', 5);
		sprintf(fa_name, "%d", i);
		proc[i].id = facility(fa_name,1);
	}

	// Vamos fazer o escalonamento inicial de eventos:

	// Nossos processos vão executar testes em intervalos de testes o intervalo
	// de testes vai ser de 30 unidades de tempo a simulação começa no tempo 0
	// (zero) e vamos escalonar o primeiro teste de todos os processos para o
	// tempo 30.0.

	for (i=0; i<n; i++) {
		schedule(EVENT_TEST, TEST_INTERVAL_TIME, i); 
	}
	// 1 falho - 4 processos totais
	schedule(EVENT_FAULT, 11.0, 1);
	schedule(EVENT_RECOVERY, 21.0, 1);
	// 2 falhos seguidos - 4 processos totais
	schedule(EVENT_FAULT, 31.0, 1);
	schedule(EVENT_FAULT, 31.0, 2);
	schedule(EVENT_RECOVERY, 41.0, 1);
	schedule(EVENT_RECOVERY, 41.0, 2);
	// 2 falhos intercalados - 4 processos totais
	schedule(EVENT_FAULT, 51.0, 1);
	schedule(EVENT_FAULT, 51.0, 3);
	schedule(EVENT_RECOVERY, 61.0, 1);
	schedule(EVENT_RECOVERY, 61.0, 3);
	// Apenas 1 vivo - 4 processos totais
	schedule(EVENT_FAULT, 71.0, 1);
	schedule(EVENT_FAULT, 71.0, 2);
	schedule(EVENT_FAULT, 71.0, 3);
	schedule(EVENT_RECOVERY, 81.0, 1);
	schedule(EVENT_RECOVERY, 81.0, 2);
	schedule(EVENT_RECOVERY, 81.0, 3);
	// Todos falhos - 4 processos totais
	schedule(EVENT_FAULT, 91.0, 0);
	schedule(EVENT_FAULT, 91.0, 1);
	schedule(EVENT_FAULT, 91.0, 2);
	schedule(EVENT_FAULT, 91.0, 3);
	// Todos vivos - 4 processos totais
	schedule(EVENT_RECOVERY, 101.0, 0);
	schedule(EVENT_RECOVERY, 101.0, 1);
	schedule(EVENT_RECOVERY, 101.0, 2);
	schedule(EVENT_RECOVERY, 101.0, 3);

	// Agora vem o loop principal do simulador:
	puts("===============================================================");
	puts("           Sistemas Distribuídos Prof. Elias");
	puts("          LOG do Trabalho prático 0, Tarefa 2");
	puts("      Implementação do teste em anel entre processos");
	printf("   Este programa foi executado para: N=%d processos.\n", n); 
	printf("           Tempo Total de Simulação = %d\n", max_simulation_time);
	puts("===============================================================");

	while(time() < 150.0) {
		cause(&event, &token);
		switch(event) {
			case EVENT_TEST: 
				// Se o processo está falho, então não testa:
				if (status(proc[token].id) != 0) {
					break;
				}
				
				int next_proc = (token + 1) % n;
				
				// Enquanto o próximo processo estiver falho, testar o próximo processo
				while (next_proc != token && status(proc[next_proc].id) != 0) {
					printf("-> O processo %d testou o processo %d falho no tempo %4.1f\n", 
						token, next_proc, time());

					next_proc = (next_proc + 1) % n;
				}

				if (next_proc == token) {
					printf("-> Todos os processos falharam, menos o processo %d no tempo %4.1f\n", 
						token, time());
				} else {
					printf("-> O processo %d testou o processo %d correto no tempo %4.1f\n", 
						token, next_proc, time());
				}
				
				// Agendar o próximo teste para este processo
				schedule(EVENT_TEST, TEST_INTERVAL_TIME, token);
				break;
				
			case EVENT_FAULT:
				r = request(proc[token].id, token, 0);
				printf("-> O processo %d falhou no tempo %4.1f\n", token, time());
				break;
				
			case EVENT_RECOVERY:
				release(proc[token].id, token);
				printf("-> O processo %d recuperou no tempo %4.1f\n", token, time());
				schedule(EVENT_TEST, TEST_AFTER_RECOVERY_TIME, token);
				break;
		}
	}
}

