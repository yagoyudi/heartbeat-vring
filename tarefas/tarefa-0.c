#include <stdio.h>
#include <stdlib.h>
#include "../smpl.h"

// Vamos definir os EVENTOS:
#define EVENT_TEST 1
#define EVENT_FAULT 2
#define EVENT_RECOVERY 3

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
		schedule(EVENT_TEST, 30.0, i); 
	}
	schedule(EVENT_FAULT, 31.0, 1);
	schedule(EVENT_RECOVERY, 61.0, 1);

	// Agora vem o loop principal do simulador:

	puts("===============================================================");
	puts("           Sistemas Distribuídos Prof. Elias");
	puts("          LOG do Trabalho prático 0, Tarefa 0");
	puts("      Digitar, compilar e executar o programa tempo.c");
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
				printf("proc %d test: %4.1f\n", token, time());
				schedule(EVENT_TEST, 30.0, token);
				break;
			case EVENT_FAULT:
				r = request(proc[token].id, token, 0);
				printf("proc %d fault: %4.1f\n", token, time());
				break;
			case EVENT_RECOVERY:
				release(proc[token].id, token);
				printf("proc %d recovery: %4.1f\n", token, time());
				schedule(EVENT_TEST, 1.0, token);
				break;
		}
	}
}