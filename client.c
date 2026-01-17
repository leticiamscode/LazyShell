#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// Constantes
#define MAX_LINHA 1000
#define MAX_BUFFER 5
#define MAX_ARGS 3

#define MAX_PGID_DFL 10
#define MAX_PGID_RSZ 2



// Prototipos de funcoes
void trataSigint(int signum);

void trataSigalrm(int signum);

void trataSigchld(int signum);

void comandoWait();

void comandoExit(int *vetPgid);



// Variaveis globais (para uso em tratadores)
char linhaLsh[6] = "lsh> ";
int  tamBuffer=0, numPgid=0, numFilhosVivos=0,
     executar=0, resistir=1;

int main() {
    // Configura mascara para bloquear SIGINT
    sigset_t maskSigint;
    if ((sigemptyset(&maskSigint) == -1) ||
        (sigaddset(&maskSigint, SIGINT) == -1)) {
        perror("Falha ao construir mascara de sinais");
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
        perror("Falha ao instalar tratador de SIGINT");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGALRM, &saSigalrm, NULL) == -1) {
        perror("Falha ao instalar tratador de SIGALRM");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGCHLD, &saSigchld, NULL) == -1) {
        perror("Falha ao instalar tratador de SIGCHLD");
        exit(EXIT_FAILURE);
    }

    // Aloca vetor de inteiros para guardar os PGIDs dos processos filhos
    int  *vetPgid, maxPgid=MAX_PGID_DFL;
    vetPgid = malloc(sizeof(int) * maxPgid);
    if (vetPgid == NULL) {
        perror("Falha ao alocar vetor de PGIDs");
        exit(EXIT_FAILURE);
    }



    // Loop de funcionamento da lazy shell
    char buff[MAX_LINHA], buffComandos[MAX_BUFFER][MAX_LINHA];
    while (1) {
        // Assinatura da lazy shell
        printf("%s", linhaLsh);


        // Tenta ler da entrada
        if (fgets(buff, MAX_LINHA, stdin) != NULL)
            if ((tamBuffer < MAX_BUFFER) &&
                (sscanf(buff, " %[^\n]", buffComandos[tamBuffer]) == 1))
                tamBuffer++;


        // Executa os comandos armazenados
        if (executar == 1) {
            // trata buffer e executa

            resistir = 1; // Primeiro, para SIGINT nao ativar o encerramento
            tamBuffer = 0; // Segundo, para SIGINT nao reativar a execucao
            executar = 0;
        }
    }



    // Saida deve ser por aqui?
    free(vetPgid);
    return EXIT_SUCCESS;
}



void trataSigint(int signum) {
    printf("\n"); // Para separar a saida do comando ^C

    if (tamBuffer > 0) {
        executar = 1;
    }
    else { // So pode ser 0
        int pid = fork();
        if (pid == -1) {     // Nao houve fork
            perror("Falha ao procurar processos filhos (fork)");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) { // Processo filho
            if (execlp("pgrep", "pgrep", "-A", "-c", "-s", "0", NULL) == -1) {
                perror("Falha ao procurar processos filhos (exec)");
                exit(EXIT_FAILURE);
            }
        }
        else {               // Processo pai
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("Falha ao procurar processos filhos (exec)");
                exit(EXIT_FAILURE);
            }
            // lidar com caso em que processo eh sinalizado?

            // Existem filhos vivos e a lsh pode resistir
            if ((WEXITSTATUS(status) == 0) && (resistir == 1)) {
                printf("%sNão posso morrer... sou lenta mas sou mãe de família!!!\n", linhaLsh);
                resistir = 0;
                alarm(3);
            }
            else {
                printf("%sOk... você venceu! Adeus!\n", linhaLsh);
                exit(EXIT_SUCCESS);
            }
        }
    }
}

void trataSigalrm(int signum) {
    printf("\n"); // Para separar a proxima linhaLsh
    resistir = 1;
}

void trataSigchld(int signum) {
    numFilhosVivos--;

    // Medida de economia de memoria, caso nao haja mais filhos
    if (numFilhosVivos == 0)
        numPgid = 0;
}

void comandoWait() {
    int pid, wstatus, numWait=0;

    while ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
        if (WIFEXITED(wstatus))
            printf("O processo %d terminou com retorno %d.\n", pid, WEXITSTATUS(wstatus));
        else if (WIFSIGNALED(wstatus))
            printf("O processo %d foi terminado pelo sinal %d.\n", pid, WTERMSIG(wstatus));

        numWait++;
    }

    if (numWait == 0)
        printf("Nao ha processos filhos a liberar.\n");
}

void comandoExit(int *vetPgid) {
    // Matando os processos vivos que restam
    for (int i=0; (i < numPgid) && (numFilhosVivos > 0); i++)
        kill(-vetPgid[i], SIGKILL);

    // Liberando todos os processos filhos
    while (1)
        if ((waitpid(-1, NULL, WNOHANG) == -1) && (errno == ECHILD))
            break;

    free(vetPgid);
    exit(EXIT_SUCCESS); // Deveria ser em outro lugar?
}