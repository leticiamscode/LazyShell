#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

// Constantes
#define MAX_LINHA 1000
#define MAX_BUFFER 5
#define MAX_ARGS 3


#define WHITESPACES " \n\t" // Whitespaces usados como delimitadores do strtok()


// Prototipos de funcoes
void trataSigint(int signum);

void trataSigalrm(int signum);

void trataSigterm(int signum);

void comandoCd(char *arg);

void comandoWait();

void comandoExit();

void comandoExternoUnico(char **comando1);
void comandoExternoPipe(char **comando1, char **comando2);



// Variaveis globais (para uso em tratadores)
char pidLsh[12], linhaLsh[6]="lsh> ";
int  tamBuffer=0, executar=0, resistir=1;

int PGIDatual = -1;

int main() {
    // Poe PID em uma string, para uso eventual
    if (snprintf(pidLsh, 11, "%d", getpid()) < 0) {
        perror("Falha ao imprimir PID");
        exit(EXIT_FAILURE);
    }

    // Configura mascara para bloquear SIGINT
    sigset_t maskSigint;
    if ((sigemptyset(&maskSigint) == -1) ||
        (sigaddset(&maskSigint, SIGINT) == -1)) {
        perror("Falha ao construir mascara de sinais");
        exit(EXIT_FAILURE);
    }

    // Configura tratadores para SIGINT, SIGALRM e SIGTERM
    // Os tratadores de SIGALRM e SIGTERM devem bloquear SIGINT
    struct sigaction saSigint, saSigalrm, saSigterm;
    saSigint.sa_handler = &trataSigint;
    saSigalrm.sa_handler = &trataSigalrm;
    saSigterm.sa_handler = &trataSigterm;
    saSigalrm.sa_mask = saSigterm.sa_mask = maskSigint;
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
    if (sigaction(SIGTERM, &saSigterm, NULL) == -1) {
        perror("Falha ao instalar tratador de SIGTERM");
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
                    comandoCd(comando1[1]);

                } else if (strcmp(comando1[0], "wait") == 0){
                    comandoWait();

                } else if (strcmp(comando1[0], "exit") == 0){
                    comandoExit();

                } else {
                    // Quando strComando2 possui valor, significa existência de pipe/segundo comando
                    if (strComando2 != NULL) {
                        comandoExternoPipe(comando1, comando2);
                    } else {
                        comandoExternoUnico(comando1);
                    }
                }
            }

            resistir = 1; // Primeiro, para SIGINT nao ativar o encerramento
            tamBuffer = 0; // Segundo, para SIGINT nao reativar a execucao
            executar = 0;

            PGIDatual = -1;
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
            if (execlp("pgrep", "pgrep", "-P", pidLsh, "-r", "R,S,D,T", NULL) == -1) {
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
        if (!WIFEXITED(status)) {
            perror("Falha ao procurar processos filhos (signal)");
            exit(EXIT_FAILURE);
        }

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

void trataSigterm(int signum) {
    if (kill(0, SIGTERM) == -1) {
        perror("Falha ao matar grupo de processos");
        exit(EXIT_FAILURE);
    }

    // Liberando todos os processos filhos
    while ((wait(NULL) != -1) || (errno != ECHILD));

    exit(EXIT_SUCCESS);
}

void comandoCd(char *arg){
    if (chdir(arg) == -1){
        perror("Erro ao executar cd");
    }
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
        // Os filhos que receberem o sinal (SIGTERM) matarao seus filhos, se houver
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

    // Liberando todos os processos filhos
    while ((wait(NULL) != -1) || (errno != ECHILD));

    exit(EXIT_SUCCESS); // Deveria ser em outro lugar?
}

void comandoExternoUnico(char **comando1){
    // Função de DEBUG para listar/imprimir os argumentos separadamente
    // for (int i=0; i<5; i++) printf("arg %d: '%s'\n", i, comando1[i]);

    // Se o elemento de índice 4 foi alterado de NULL, significa que excedeu o limite de argumentos
    if (comando1[4] != (char *) NULL){
        printf("\tERRO: Comando com mais de 3 argumentos.\n");
        return;
    }

    int pid = fork();
    
    if (pid < 0){
        printf("Erro ao fazer fork.\n");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Código do pai, mudará o GPID do filho e criará um pipe
        // para o segundo filho, se necessário
        
        if (PGIDatual == -1) PGIDatual = pid;
        setpgid(pid, PGIDatual);
    } else {
        // Código do primeiro filho, aqui será tratado o
        // pipe e então chamado o comando exec

        if (execvp(comando1[0], comando1) == -1){
            perror("Erro ao executar exec");
            // Tem que tratar com o errno, e provavelmente tratar o segundo filho (se tiver)
        }
    }
}

void comandoExternoPipe(char **comando1, char **comando2){
    // Função de DEBUG para listar/imprimir os argumentos separadamente
    // for (int i=0; i<5; i++) printf("arg %d: '%s'\n", i, comando1[i]);
    // for (int i=0; i<5; i++) printf("arg %d: '%s'\n", i, comando2[i]);

    // Se o elemento de índice 4 foi alterado de NULL, significa que excedeu o limite de argumentos
    if ((comando1[4] != (char *) NULL) || (comando2[4] != (char *) NULL)){
        printf("\tERRO: Comando com mais de 3 argumentos.\n");
        return;
    }

    int fd[2];

    int ret = pipe(fd);
    if (ret == -1){
        printf("Erro ao criar pipe.\n");
        exit(EXIT_FAILURE);
    }

    int pid1 = fork();

    if (pid1 < 0){
        printf("Erro ao fazer fork.\n");
        exit(EXIT_FAILURE);
    } else if (pid1 > 0) {
        // Código do pai, criará um pipe para o segundo filho

        if (PGIDatual == -1) PGIDatual = pid1;
        setpgid(pid1, PGIDatual);

        int pid2 = fork();

        if (pid2 < 0){
            printf("Erro ao fazer fork.\n");
            exit(EXIT_FAILURE);
        } else if (pid2 > 0) {
            // Código do pai, mudará o GPID do segundo filho

            setpgid(pid2, PGIDatual);

            // Fecha os descritores do pipe no pai
            close(fd[0]);
            close(fd[1]);

            int wstatus;
            waitpid(-1, &wstatus, WUNTRACED);

            if (WIFEXITED(wstatus)){
                if (WEXITSTATUS(wstatus) == EXIT_FAILURE){
                    kill(pid1, SIGKILL);
                    kill(pid2, SIGKILL);

                    printf("DEBUG: Recebeu erro, matando filhos\n");
                    // Tem chance de algum exec retornar EXIT_SUCCESS antes do outro dar erro?
                }
            }

        } else {
            // Código do segundo filho, trata
            // o pipe e chama o exec

            // Fecha o descritor de escrita, já que esse filho apenas lê
            close(fd[1]);

            // Redireciona a entrada do filho para a saída do pipe
            dup2(fd[0], 0);
            close(fd[0]);

            if (execvp(comando2[0], comando2) == -1){
                perror("Erro ao executar segundo exec");
                exit(EXIT_FAILURE);
                // Tem que tratar com o errno, e provavelmente tratar o primeiro filho

            }
        }
    } else {
        // Código do primeiro filho, aqui será tratado o
        // pipe e então chamado o comando exec

        // Fecha o descritor de leitura, já que esse filho apenas escreve
        close(fd[0]);

        // Redireciona a saída do filho para a entrada do pipe
        dup2(fd[1], 1);
        close(fd[1]);

        if (execvp(comando1[0], comando1) == -1){
            perror("Erro ao executar primeiro exec");
            exit(EXIT_FAILURE);
            // Tem que tratar com o errno, e provavelmente tratar o segundo filho (se tiver)
        }
    }
}