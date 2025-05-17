#include <stdio.h>
#include <stdlib.h>
#include "../smpl.h"

// Cores ANSI
#define RED     "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE    "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN    "\x1B[36m"
#define RESET   "\x1B[0m"

// Vamos definir os EVENTOS:
#define EVENT_TEST 1
#define EVENT_FAULT 2
#define EVENT_RECOVERY 3
#define TEST_INTERVAL 30

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
		schedule(EVENT_TEST, TEST_INTERVAL, i); 
	}
	schedule(EVENT_FAULT, 31.0, 1);
	schedule(EVENT_RECOVERY, 61.0, 1);
// 
	// Agora vem o loop principal do simulador:

	printf("%s===============================================================%s\n", BLUE, RESET);
	printf("%s           Sistemas Distribuídos Prof. Elias%s\n", BLUE, RESET);
	printf("%s          LOG do Trabalho prático 0, Tarefa 2%s\n", BLUE, RESET);
	printf("%s      Implementação do teste em anel entre processos%s\n", BLUE, RESET);
	printf("%s   Este programa foi executado para: N=%d processos.%s\n", BLUE, n, RESET); 
	printf("%s           Tempo Total de Simulação = %d%s\n", BLUE, max_simulation_time, RESET);
	printf("%s===============================================================%s\n", BLUE, RESET);

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
					printf("%s-> O processo %d testou o processo %d falho no tempo %4.1f%s\n", 
						RED, token, next_proc, time(), RESET);

					next_proc = (next_proc + 1) % n;
				}

				if (next_proc == token) {
					printf("%s-> Todos os processos falharam, menos o processo %d no tempo %4.1f%s\n", 
						RED, token, time(), RESET);
				} else {
					printf("%s-> O processo %d testou o processo %d correto no tempo %4.1f%s\n", 
						GREEN, token, next_proc, time(), RESET);
				}
				
				// Agendar o próximo teste para este processo
				schedule(EVENT_TEST, TEST_INTERVAL, token);
				break;
				
			case EVENT_FAULT:
				r = request(proc[token].id, token, 0);
				printf("%s-> O processo %d falhou no tempo %4.1f%s\n", RED, token, time(), RESET);
				break;
				
			case EVENT_RECOVERY:
				release(proc[token].id, token);
				printf("%s-> O processo %d recuperou no tempo %4.1f%s\n", GREEN, token, time(), RESET);
				schedule(EVENT_TEST, 1.0, token);
				break;
		}
	}
}

