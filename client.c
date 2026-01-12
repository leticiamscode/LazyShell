#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// constantes
#define MAX_BUFFER 5



// prototipos de funcoes
void trataSigint(int signum);

void trataSigchld(int signum);



int tamBuffer=0, numFilhosVivos=0,
    resistir=1, encerrar=0;

int main() {
    // Configura mascaras para SIGINT e SIGCHLD
    sigset_t maskSigint, maskSigchld;
    sigemptyset(&maskSigint);
    sigemptyset(&maskSigchld);
    sigaddset(&maskSigint, SIGCHLD);
    sigaddset(&maskSigchld, SIGINT);

    // Configura tratadores para SIGINT e SIGCHLD
    // Cada tratador deve bloquear o sinal complementar
    struct sigaction saSigint = {&trataSigint, maskSigchld, NULL},
                     saSigchld = {&trataSigchld, maskSigchld, NULL};
    sigaction(SIGINT, &saSigint, NULL);
    sigaction(SIGCHLD, &saSigchld, NULL);

    
    while (1) {
        // Assinatura da lazy shell
        printf("lsh> ");


        // Tenta ler da entrada


        // Verificacoes de encerramento
        if (resistir == 0) {
            sigsuspend(&maskSigint); // SIGCHLD nao pode interromper
            resistir = 1;
        }

        if (encerrar == 1)
            break;


        // executa comandos
        if (tamBuffer > 0) {
            // trata buffer e executa

            tamBuffer = 0;
        }
    }

    // encerra e espera processos filhos?

    return 0;
}



void trataSigint(int signum) {
    if (tamBuffer == 0) {
        if ((numFilhosVivos > 0) && (resistir == 1)) {
            printf("Não posso morrer... sou lenta mas sou mãe de família!!!\nlsh> ");
            resistir = 0;
        }
        else {
            printf("Ok... você venceu! Adeus!");
            encerrar = 1;
        }
    }
}

void trataSigchld(int signum) {
    numFilhosVivos--;
}