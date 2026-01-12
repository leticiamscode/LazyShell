#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// constantes
#define MAX_LINHA 1000
#define MAX_BUFFER 5



// prototipos de funcoes
void trataSigint(int signum);

void trataSigchld(int signum);



char linhaLsh[7] = "\nlsh> ";
int  tamBuffer=0, numFilhosVivos=0,
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
    struct sigaction saSigint = {&trataSigint, maskSigchld, 0},
                     saSigchld = {&trataSigchld, maskSigchld, 0};
    sigaction(SIGINT, &saSigint, NULL);
    sigaction(SIGCHLD, &saSigchld, NULL);

    
    char buff[MAX_LINHA];

    while (1) {
        // Assinatura da lazy shell
        printf("%s", linhaLsh);


        // Tenta ler da entrada
        fgets(buff, MAX_LINHA, stdin);


        // Verificacoes de encerramento
        if (resistir == 0) {
            // SIGCHLD nao pode interromper resistencia
            sigprocmask(SIG_BLOCK, &maskSigint, NULL);
            sleep(3);
            resistir = 1;
            sigprocmask(SIG_UNBLOCK, &maskSigint, NULL);
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
            printf("%sNão posso morrer... sou lenta mas sou mãe de família!!!%s", linhaLsh, linhaLsh);
            resistir = 0;
        }
        else {
            printf("%sOk... você venceu! Adeus!", linhaLsh);
            encerrar = 1;
        }
    }
}

void trataSigchld(int signum) {
    numFilhosVivos--;
}