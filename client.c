#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// constantes
#define MAX_LINHA 1000
#define MAX_BUFFER 5



// prototipos de funcoes
void trataSigint(int signum);

void trataSigalrm(int signum);

void trataSigchld(int signum);



char linhaLsh[7] = "\nlsh> ";
int  tamBuffer=0, numFilhosVivos=0,
     executar=0, resistir=1, encerrar=0;

int main() {
    // Configura mascara para bloquear SIGINT
    sigset_t maskSigint, maskNull;
    sigemptyset(&maskSigint);
    sigaddset(&maskSigint, SIGINT);
    sigemptyset(&maskNull);

    // Configura tratadores para SIGINT, SIGALRM e SIGCHLD
    // Os tratadores de SIGALRM e SIGCHLD devem bloquear SIGINT
    struct sigaction saSigint = {&trataSigint, maskNull, 0},
                     saSigalrm = {&trataSigalrm, maskSigint, 0},
                     saSigchld = {&trataSigchld, maskSigint, 0};
    sigaction(SIGINT, &saSigint, NULL);
    sigaction(SIGALRM, &saSigalrm, NULL);
    sigaction(SIGCHLD, &saSigchld, NULL);

    
    char buff[MAX_LINHA];

    while (1) {
        // Assinatura da lazy shell
        printf("%s", linhaLsh);


        // Tenta ler da entrada
        fgets(buff, MAX_LINHA, stdin);
        if (tamBuffer < MAX_BUFFER) {
            // copia pro buffer de comandos
            tamBuffer++;
        }


        // Executa os comandos armazenados
        if (executar == 1) {
            // trata buffer e executa

            resistir = 1; // Primeiro, para SIGINT nao ativar o encerramento
            tamBuffer = 0; // Segundo, para SIGINT nao reativar a execucao
            executar = 0;
        }


        // Verificacoes de encerramento
        if (resistir == 0)
            alarm(3);

        if (encerrar == 1)
            break;
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
            printf("%sNão posso morrer... sou lenta mas sou mãe de família!!!%s", linhaLsh, linhaLsh);
            resistir = 0;
        }
        else {
            printf("%sOk... você venceu! Adeus!", linhaLsh);
            encerrar = 1;
        }
    }
}

void trataSigalrm(int signum) {
    resistir = 1;
}

void trataSigchld(int signum) {
    numFilhosVivos--;
}