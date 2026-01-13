#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// Constantes
#define MAX_LINHA 1000
#define MAX_BUFFER 5
#define MAX_ARGS 3



// Prototipos de funcoes
void trataSigint(int signum);

void trataSigalrm(int signum);

void trataSigchld(int signum);



// Variaveis globais (para uso em tratadores)
char linhaLsh[7] = "\nlsh> ";
int  tamBuffer=0, numFilhosVivos=1,
     executar=0, resistir=1;

int main() {
    // Configura mascara para bloquear SIGINT
    sigset_t maskSigint;
    if ((sigemptyset(&maskSigint) == -1) ||
        (sigaddset(&maskSigint, SIGINT) == -1)) {
        perror("Falha ao construir mascara de sinais.\n");
        exit(1);
    }

    // Configura tratadores para SIGINT, SIGALRM e SIGCHLD
    // Os tratadores de SIGALRM e SIGCHLD devem bloquear SIGINT
    struct sigaction saSigint, saSigalrm, saSigchld;
    saSigint.sa_handler = &trataSigint;
    saSigalrm.sa_handler = &trataSigalrm;
    saSigchld.sa_handler = &trataSigchld;
    saSigalrm.sa_mask = saSigchld.sa_mask = maskSigint;
    saSigint.sa_flags = saSigalrm.sa_flags = saSigchld.sa_flags = 0;
    if ((sigemptyset(&saSigint.sa_mask) == -1) ||
        (sigaction(SIGINT, &saSigint, NULL) == -1)) {
        perror("Falha ao instalar tratador de SIGINT.\n");
        exit(1);
    }
    if (sigaction(SIGALRM, &saSigalrm, NULL) == -1) {
        perror("Falha ao instalar tratador de SIGALRM.\n");
        exit(1);
    }
    if (sigaction(SIGCHLD, &saSigchld, NULL) == -1) {
        perror("Falha ao instalar tratador de SIGCHLD.\n");
        exit(1);
    }

    
    char buff[MAX_LINHA];

    while (1) {
        // Assinatura da lazy shell
        printf("%s", linhaLsh);


        // Tenta ler da entrada
        if (fgets(buff, MAX_LINHA, stdin) != NULL)
            if (tamBuffer < MAX_BUFFER) {
                tamBuffer++;
                // copia pro buffer de comandos
            }


        // Executa os comandos armazenados
        if (executar == 1) {
            // trata buffer e executa

            resistir = 1; // Primeiro, para SIGINT nao ativar o encerramento
            tamBuffer = 0; // Segundo, para SIGINT nao reativar a execucao
            executar = 0;
        }
    }

    // encerra e espera processos filhos?

    return 0;
}



void trataSigint(int signum) {
    if (tamBuffer > 0) {
        executar = 1;
    }
    else { // So pode ser 0
        if ((numFilhosVivos > 0) && (resistir == 1)) {
            printf("%sNão posso morrer... sou lenta mas sou mãe de família!!!", linhaLsh);
            resistir = 0;
            alarm(3);
        }
        else {
            printf("%sOk... você venceu! Adeus!\n", linhaLsh);
            exit(0);
        }
    }
}

void trataSigalrm(int signum) {
    resistir = 1;
}

void trataSigchld(int signum) {
    numFilhosVivos--;
}