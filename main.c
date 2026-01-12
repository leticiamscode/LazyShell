#include <stdio.h>
#include <stdlib.h>
#include <string.h>

   
int main() {
    //Esboço:
    // char* buffer[5] ? - - linhas de comando;
    //obs : arquivo executavel + maximo de argumentos 3 
    // loop para leitura - apenas validar comandos e enfileirar no buffer (buffer cheio ignorar)
        //comandos internos (comandos separados por linhas) - cd, wait, exit
        //comandos externos e pipes (#) - uma linha de comando pode receber até 2 comandos externos, unidos por um ‘#’

    //Receba sinal Ctrl-c
        // if tem comandos no buffer 
            //executar os comandos  e esvaziar o buffer
        //else (buffer vazio e com processos filhos em execução )
            //exibir: “Não posso morrer... sou lenta mas sou mãe de família!!!”
            // ocorrer outro Ctrl-c em ate 3 seg
            //exibir: “Ok... você venceu! Adeus!” (… finalizando sua execução em seguida )
        //obs: buffer vazio e sem processos filhos em execucao (O que ocorre?)

    #define MAX_LINHA 200
    #define MAX_BUFFER 5

    char* buffer[MAX_BUFFER];
    for(int i=0; i<MAX_BUFFER; i++){
        buffer[i] = malloc(sizeof(char)*MAX_LINHA); //tamanho da linha?
    }
    int position = 0; //resetar position em caso de CTRL -C


    while(1) {

        printf("lsh> ");

        if (position < MAX_BUFFER) {
            if (fgets(buffer[position], MAX_LINHA, stdin) != NULL) { //falta limitar quantidade de argumentos
                char* myPtr = strtok(buffer[position], " ");
                while(myPtr){
                    printf("%s\n", myPtr); //DEBUG
                    myPtr = strtok(NULL, " "); //PROXIMA STRING SEPARADA POR ESPAÇO EM buffer[position]
                }
                position++;
            }
        } else {
            char lixo[MAX_LINHA];
            fgets(lixo, MAX_LINHA, stdin); // descarta  no caso de buffer cheio
        }
    }

    //tratador de sinal 

    for(int i=0; i<MAX_BUFFER; i++){
        free(buffer[i]); 
    }

    return 0;
}