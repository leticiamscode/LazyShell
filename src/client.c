/**
 * @file client.c
 * @author Rafael Vieira / Igor Bittencourt / Leticia Mariana
 * @brief Arquivo principal da LSH.
 * @copyright Copyright (c) 2026
 * 
 */
#include "lsh.h"

// Definição das Variáveis Globais 
char pidLsh[12];
char linhaLsh[6] = "lsh> ";
int tamBuffer = 0;  
int executar = 0; 
int resistir = 1;   
int PGIDatual = -1; // Armazena o ID do grupo atual de execução

int main() {
    // Salva o PID em uma string, para uso eventual
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

    // Configura tratador para SIGINT que será utilizado nos
    // processos filhos, que devem ignorar o sinal
    struct sigaction saIgnoreSigint;
    saIgnoreSigint.sa_handler = SIG_IGN;
    saIgnoreSigint.sa_flags = NULL;
    if ((sigemptyset(&saIgnoreSigint.sa_mask) == -1)){
        perror("Falha ao criar máscara de sinais para processos filhos");
    }

    // Loop de funcionamento da lazy shell
    char buff[MAX_LINHA];
    char buffComandos[MAX_BUFFER][MAX_LINHA];
    
    while (1) {
        // Assinatura da lazy shell
        printf("%s", linhaLsh);

        // Leitura de comandos
        if(fgets(buff, MAX_LINHA, stdin) != NULL){ 
            if ((tamBuffer < MAX_BUFFER) && sscanf(buff, " %[^\n]", buffComandos[tamBuffer]) == 1) {
                // Copia pro buffer de comandos
                tamBuffer++;
            }
        }


        // Executa os comandos armazenados
        if (executar == 1) {
            // Para cada "grupo de comandos" do buffer, é criado antecipadamente um processo "representante",
            // que servirá como mantenedor do Process Group desses comandos, provendo o PGID a ser usado por eles
            int pidRepresentanteGrupo = fork();

            if (pidRepresentanteGrupo < 0){
                perror("Falha ao criar representante pra execução do buffer (fork)");
                exit(EXIT_FAILURE);
            } else if (pidRepresentanteGrupo == 0){
                // Representante do grupo, cria o PGID e deve permanecer vivo  até todos
                // os comandos serem iniciados, então eles mesmos manterão o grupo

                // Cria o grupo (PGID) a partir de seu próprio PID
                setpgid(0, 0);

                // Permanece vivo por 1 segundo, tempo suficiente para os comandos iniciarem
                sleep(1);

                // Finaliza com sucesso
                exit(EXIT_SUCCESS);
            } else {
                // Processo pai (lsh), salva o PGID do representante
                // na variável global PGIDatual pela duração do loop

                PGIDatual = pidRepresentanteGrupo;
            }

            for (int i = 0; i < tamBuffer; i++) {
                // Cancela qualquer alarme, que poderia interromper a execução dos filhos
                alarm(0);

                // Separação dos argumentos (e comandos, caso haja pipe)
                char* strComando1 = buffComandos[i];
                char* strComando2 = NULL; // NULL se não tiver pipe
            
                char* comando1[5] = {NULL, NULL, NULL, NULL, NULL}; // executavel + até 3 args + NULL
                char* comando2[5] = {NULL, NULL, NULL, NULL, NULL}; // executavel + até 3 args + NULL
                
                
                // Verifica existência de Pipe, e seta a flag temPipe apropriadamente
                int temPipe = 0;

                char *posPipe = strchr(buffComandos[i], '#');
                if (posPipe != NULL) {
                    *posPipe = '\0';        // insere \0 no strComando1
                    strComando2 = posPipe + 1;  // posicao inicio do segundo comando
                    temPipe = 1;
                }

                // Separa por espaços para preencher o vetor comando1
                int qtdComando1 = 0;
                char* token = strtok(strComando1, WHITESPACES); 
                while (token != NULL && qtdComando1 < 5) {  
                    comando1[qtdComando1++] = token;
                    token = strtok(NULL, WHITESPACES); 
                }
                
                // Separa por espaços para preencher o vetor comando2
                if (strComando2 != NULL) {
                    int qtdComando2 = 0;
                    
                    token = strtok(strComando2, WHITESPACES); 
                    while (token != NULL && qtdComando2 < 5) {  
                        comando2[qtdComando2++] = token;
                        token = strtok(NULL, WHITESPACES);
                    }
                }

                if (strcmp(comando1[0], "cd") == 0){
                    if (temPipe){ // Tentativa de pipe, cd inválido
                        printf("Erro no cd: pipe não é permitido\n");
                    } else if (comando1[2] != NULL){ // Mais de 1 argumento, cd inválido
                        printf("Erro no cd: espera-se apenas 1 argumento\n");
                    } else { // 1 argumento ou menos, aceitável para o chdir()
                        comandoCd(comando1[1]);
                    }

                } else if (strcmp(comando1[0], "wait") == 0){
                    if (temPipe){ // Tentativa de pipe, wait inválido
                        printf("Erro no wait: pipe não é permitido\n");
                    } else if (comando1[1] != NULL){ // Possui argumento, wait inválido
                        printf("Erro no wait: não deve receber nenhum argumento\n");
                    } else { // Nenhum argumento, aceitável para o comando wait
                        comandoWait();
                    }

                } else if (strcmp(comando1[0], "exit") == 0){
                    if (temPipe){ // Tentativa de pipe, exit inválido
                        printf("Erro no exit: pipe não é permitido\n");
                    } else if (comando1[1] != NULL){ // Possui argumento, exit inválido
                        printf("Erro no exit: não deve receber nenhum argumento\n");
                    } else { // Nenhum argumento, aceitável para o comando exit
                        comandoExit();
                    }

                } else {
                    if (temPipe) {
                        comandoExternoPipe(comando1, comando2, saIgnoreSigint);
                    } else {
                        comandoExternoUnico(comando1, saIgnoreSigint);
                    }
                }

                // Delay de 0.1s para garantir a ordem esperada dos comandos executados
                usleep(100000);
            }

            resistir = 1; // Primeiro, para SIGINT nao ativar o encerramento
            tamBuffer = 0; // Segundo, para SIGINT nao reativar a execucao
            executar = 0;  // Desativa a flag de execução

            // Reseta o valor de PGIDatual, além de matar o processo
            // representante desse grupo, agora que já foi executado
            PGIDatual = -1;

            kill(pidRepresentanteGrupo, SIGKILL);
            waitpid(pidRepresentanteGrupo, NULL, WUNTRACED);
        }
    }

    return EXIT_SUCCESS;
}
