#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

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


// Muda o GPID de todos os filhos pra esse, que será o GPID do
// primeiro filho criado, garantindo que não será usado novamente
int PGIDfilhos = -1;

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
    char buffComandos[MAX_BUFFER][MAX_LINHA];
    
    while (1) {
        // Assinatura da lazy shell
        printf("%s", linhaLsh);


        // Tenta ler da entrada
        if(fgets(buff, MAX_LINHA, stdin)!=NULL){ 
     
            if (tamBuffer < MAX_BUFFER) {
                // copia pro buffer de comandos
                strcpy(buffComandos[tamBuffer], buff); 
                tamBuffer++;
            }
        }



        // Executa os comandos armazenados
        if (executar == 1) {
            // trata buffer e executa
            for (int i = 0; i < tamBuffer; i++) {
                char* strComando1 = buffComandos[i];
                char* strComando2 = NULL; // NULL se não tiver pipe
            
                char* comando1[5]; // executavel + até 3 args + NULL
                char* comando2[5]; // executavel + até 3 args + NULL
                
                // procurar pipe
                char *posPipe = strchr(buffComandos[i], '#');
                if (posPipe != NULL) {
                    *posPipe = '\0';        // insere \0 no strComando1
                    strComando2 = posPipe + 1;  // posicao inicio do segundo comando
                }

                // separa por espaços para preencher o vetor comando1
                int qtdComando1 = 0;
                char* token = strtok(strComando1, " \n"); 
                while (token != NULL && qtdComando1 < 4) {  //falta notificar caso ultrapasse numero de argumentos?
                    comando1[qtdComando1++] = token;
                    token = strtok(NULL, " \n"); 
                }

                comando1[qtdComando1] = (char *) NULL; // Termina com null para indicar fim dos argumentos no exec
               
                
                // separa por espaços para preencher o vetor comando2
                if (strComando2 != NULL) {
                    int qtdComando2 = 0;

                    token = strtok(strComando2, " \n"); 
                    while (token != NULL && qtdComando2 < 4) {  //falta notificar caso ultrapasse numero de argumentos?
                        comando2[qtdComando2++] = token;
                        token = strtok(NULL, " zn");
                    }

                    comando2[qtdComando2] = (char *) NULL; // Termina com null para indicar fim dos argumentos no exec
                }
                


                if (strcmp(comando1[0], "cd") == 0){
                    printf("\tComando interno: cd\n");
                    // Aqui trata o comando cd, onde comando[1] é o caminho esperado, e o resto é ignorado

                } else if (strcmp(comando1[0], "wait") == 0){
                    printf("\tcomando interno: wait\n");
                    // Aqui trata o comando wait, onde qualquer outro argumento é ignorado

                } else if (strcmp(comando1[0], "exit") == 0){
                    printf("\tcomando interno: exit\n");
                    // Aqui trata o comando exit, onde qualquer outro argumento é ignorado

                } else {
                    // Execução dos comandos externos

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
                    } else {
                        // Código do primeiro filho, aqui será tratado o
                        // pipe e então chamado o comando exec

                        if (temPipe){
                            // Fecha o descritor de leitura, já que esse filho apenas escreve
                            close(fd[0]);

                            // Redireciona a saída do filho para a entrada do pipe
                            dup2(fd[1], 1);
                            close(fd[1]);
                        }

                        if (execvp(comando1[0], comando1) == -1){
                            printf("\tErro no exec.\n");
                            // Tem que tratar com o errno, e provavelmente tratar o segundo filho (se tiver)

                        }
                    }
                }
            }


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
