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



// Prototipos de funcoes
void trataSigint(int signum);

void trataSigalrm(int signum);

void trataSigchld(int signum);

void comandoWait();

void comandoExit();



// Variaveis globais (para uso em tratadores)
char pidLsh[12], linhaLsh[6]="lsh> ";
int  tamBuffer=0, executar=0, resistir=1;

int main() {
    // Poe PID em uma string, para uso eventual
    if (snprintf(pidLsh, 11, "%d", getpid()) < 0) {
        perror("Falha ao imprimir PID");
        exit(EXIT_FAILURE);
    }

    // Configura mascara para bloquear SIGINT
    sigset_t maskSigalrm;
    if ((sigemptyset(&maskSigalrm) == -1) ||
        (sigaddset(&maskSigalrm, SIGINT) == -1)) {
        perror("Falha ao construir mascara de sinais");
        exit(EXIT_FAILURE);
    }

    // Configura tratadores para SIGINT e SIGALRM
    // O tratador de SIGALRM deve bloquear SIGINT
    struct sigaction saSigint, saSigalrm;
    saSigint.sa_handler = &trataSigint;
    saSigalrm.sa_handler = &trataSigalrm;
    saSigalrm.sa_mask = maskSigalrm;
    saSigint.sa_flags = saSigalrm.sa_flags = 0;
    if ((sigemptyset(&saSigint.sa_mask) == -1) ||
        (sigaction(SIGINT, &saSigint, NULL) == -1)) {
        perror("Falha ao instalar tratador de SIGINT");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGALRM, &saSigalrm, NULL) == -1) {
        perror("Falha ao instalar tratador de SIGALRM");
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
    return EXIT_SUCCESS;
}



void trataSigint(int signum) {
    printf("\n"); // Para separar a saida do comando ^C

    // Verificacao da existencia de comandos a executar
    if (tamBuffer > 0) {
        executar = 1;
    }
    else { // So pode ser 0
        // Verificacao da existencia de filhos vivos
        int pid = fork();
        if (pid == -1) {     // NAO HOUVE FORK
            perror("Falha ao procurar processos filhos (fork)");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) { // PROCESSO FILHO
            // Criacao de pipe interno para evitar saida no terminal
            int fd[2];
            if (pipe(fd) == -1) {
                perror("Falha ao procurar processos filhos (pipe)");
                exit(EXIT_FAILURE);
            }
            if ((dup2(fd[0], 0) == -1) || (dup2(fd[1], 1) == -1)) {
                perror("Falha ao procurar processos filhos (dup)");
                exit(EXIT_FAILURE);
            }
            close(fd[0]);
            close(fd[1]);
            
            // Execucao de pgrep para encontrar filhos vivos da lsh
            if (execlp("pgrep", "pgrep", "-P", pidLsh, NULL) == -1) {
                perror("Falha ao procurar processos filhos (exec)");
                exit(EXIT_FAILURE);
            }
        }
                            // PROCESSO PAI
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("Falha ao procurar processos filhos (wait)");
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

void trataSigalrm(int signum) {
    printf("\n"); // Para separar a proxima linhaLsh
    resistir = 1;
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

void comandoExit() {
    // Matando os processos vivos que restam
    int pid = fork();
    if (pid == -1) {     // NAO HOUVE FORK
        perror("Falha ao matar processos filhos (fork)");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) { // PROCESSO FILHO
        // Criacao de pipe interno para evitar saida no terminal
        int fd[2];
        if (pipe(fd) == -1) {
            perror("Falha ao matar processos filhos (pipe)");
            exit(EXIT_FAILURE);
        }
        if ((dup2(fd[0], 0) == -1) || (dup2(fd[1], 1) == -1)) {
            perror("Falha ao matar processos filhos (dup)");
            exit(EXIT_FAILURE);
        }
        close(fd[0]);
        close(fd[1]);
        
        // Execucao de pkill para matar filhos vivos da lsh
        if (execlp("pkill", "pkill", "-P", pidLsh, NULL) == -1) {
            perror("Falha ao matar processos filhos (exec)");
            exit(EXIT_FAILURE);
        }
    }
                        // PROCESSO PAI
    if (waitpid(pid, NULL, 0) == -1) {
        perror("Falha ao matar processos filhos (wait)");
        exit(EXIT_FAILURE);
    }
    // lidar com caso em que processo eh sinalizado?

    // Liberando todos os processos filhos
    while (1)
        if ((waitpid(-1, NULL, WNOHANG) == -1) && (errno == ECHILD))
            break;

    exit(EXIT_SUCCESS); // Deveria ser em outro lugar?
}