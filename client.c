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

#define WHITESPACES " \n\t" // Whitespaces usados como delimitadores do strtok()


// Prototipos de funcoes
void trataSigint(int signum);

void trataSigalrm(int signum);

void trataSigchld(int signum);

void comandoWait();

void comandoExit(int *vetPgid);

void comandoExterno(char *comando1, char *comando2, char *strComando2);



// Variaveis globais (para uso em tratadores)
char linhaLsh[6] = "lsh> ";
int  tamBuffer=0, numPgid=0, numFilhosVivos=0,
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
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGALRM, &saSigalrm, NULL) == -1) {
        perror("Falha ao instalar tratador de SIGALRM.\n");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGCHLD, &saSigchld, NULL) == -1) {
        perror("Falha ao instalar tratador de SIGCHLD.\n");
        exit(EXIT_FAILURE);
    }

    // Aloca vetor de inteiros para guardar os PGIDs dos processos filhos
    int  *vetPgid, maxPgid=MAX_PGID_DFL;
    vetPgid = malloc(sizeof(int) * maxPgid);
    if (vetPgid == NULL) {
        perror("Falha ao alocar vetor de PGIDs.\n");
        exit(EXIT_FAILURE);
    }



    // Loop de funcionamento da lazy shell
    char buff[MAX_LINHA];
    char buffComandos[MAX_BUFFER][MAX_LINHA];
    
    while (1) {
        // Assinatura da lazy shell
        printf("%s", linhaLsh);

        // Tenta ler da entrada
        if(fgets(buff, MAX_LINHA, stdin) != NULL){ 
            if ((tamBuffer < MAX_BUFFER) && sscanf(buff, " %[^\n]", buffComandos[tamBuffer]) == 1) {
                // copia pro buffer de comandos
                tamBuffer++;
            }
        }


        // Executa os comandos armazenados
        if (executar == 1) {
            for (int i = 0; i < tamBuffer; i++) {
                // Separação dos argumentos (e comandos, caso haja pipe)
                char* strComando1 = buffComandos[i];
                char* strComando2 = NULL; // NULL se não tiver pipe
            
                char* comando1[5] = {NULL, NULL, NULL, NULL, NULL}; // executavel + até 3 args + NULL
                char* comando2[5] = {NULL, NULL, NULL, NULL, NULL}; // executavel + até 3 args + NULL
                
                // procurar pipe
                char *posPipe = strchr(buffComandos[i], '#');
                if (posPipe != NULL) {
                    *posPipe = '\0';        // insere \0 no strComando1
                    strComando2 = posPipe + 1;  // posicao inicio do segundo comando
                }

                // separa por espaços para preencher o vetor comando1
                int qtdComando1 = 0;
                char* token = strtok(strComando1, WHITESPACES); 
                while (token != NULL && qtdComando1 < 5) {  //falta notificar caso ultrapasse numero de argumentos?
                    comando1[qtdComando1++] = token;
                    token = strtok(NULL, WHITESPACES); 
                }
                
                // separa por espaços para preencher o vetor comando2
                if (strComando2 != NULL) {
                    int qtdComando2 = 0;
                    
                    token = strtok(strComando2, WHITESPACES); 
                    while (token != NULL && qtdComando2 < 5) {  //falta notificar caso ultrapasse numero de argumentos?
                        comando2[qtdComando2++] = token;
                        token = strtok(NULL, WHITESPACES);
                    }
                }

                if (strcmp(comando1[0], "cd") == 0){
                    printf("\tComando interno: cd\n");
                    // Aqui trata o comando cd, onde comando[1] é o caminho esperado, e o resto é ignorado

                } else if (strcmp(comando1[0], "wait") == 0){
                    printf("\tcomando interno: wait\n");
                    comandoWait();

                } else if (strcmp(comando1[0], "exit") == 0){
                    printf("\tcomando interno: exit\n");
                    //comandoExit();

                } else {
                    comandoExterno(comando1, comando2, strComando2);
                }

            resistir = 1; // Primeiro, para SIGINT nao ativar o encerramento
            tamBuffer = 0; // Segundo, para SIGINT nao reativar a execucao
            executar = 0;
            
            }
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
        if ((numFilhosVivos > 0) && (resistir == 1)) {
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

void comandoExterno(char *comando1, char *comando2, char *strComando2){
    // Função de DEBUG para listar/imprimir os argumentos separadamente
    // for (int i=0; i<5; i++) printf("arg %d: '%s'\n", i, comando1[i]);
    // for (int i=0; i<5; i++) printf("arg %d: '%s'\n", i, comando2[i]);

    // Se o elemento de índice 4 foi alterado de NULL, significa que excedeu o limite de argumentos
    if ((comando1[4] != (char *) NULL) || (comando2[4] != (char *) NULL)){
        printf("\tERRO: Comando com mais de 3 argumentos.\n");
        return;
    }

    // Variável/Flag pra facilitar o uso de pipe
    int temPipe = 0;

    int fd[2];
    if (strComando2 != NULL){
        temPipe = 1;

        int ret = pipe(fd);
        if (ret == -1){
            printf("Erro ao criar pipe.\n");
            exit(EXIT_FAILURE);
        }
    }

    int pid = fork();

    if (pid < 0){
        printf("Erro ao fazer fork.\n");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Código do pai, mudará o GPID do filho e criará um pipe
        // para o segundo filho, se necessário

        if (PGIDfilhos == -1) PGIDfilhos = pid;

        setpgid(pid, PGIDfilhos);

        if (temPipe){
            int pid2 = fork();

            if (pid2 < 0){
                printf("Erro ao fazer fork.\n");
                exit(EXIT_FAILURE);
            } else if (pid2 > 0) {
                // Código do pai, mudará o GPID do segundo filho

                setpgid(pid2, PGIDfilhos);

                // Fecha os descritores do pipe no pai
                close(fd[0]);
                close(fd[1]);

            } else {
                // Código do segundo filho, trata
                // o pipe e chama o exec

                // Fecha o descritor de escrita, já que esse filho apenas lê
                close(fd[1]);

                // Redireciona a entrada do filho para a saída do pipe
                dup2(fd[0], 0);
                close(fd[0]);

                if (execvp(comando2[0], comando2) == -1){
                    printf("\tErro no exec.\n");
                    // Tem que tratar com o errno, e provavelmente tratar o primeiro filho

                }
            }
        }
    }
}