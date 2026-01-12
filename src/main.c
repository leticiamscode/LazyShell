#include <stdio.h>


int main() {
    
    //Esboço:
    // char* buffer[5] ?;
    //obs : arquivo executavel + maximo de argumentos 3 
    // loop para leitura - apenas validar comandos e enfileirar no buffer (buffer cheio ignorar)
        //comandos internos (comandos separados por linhas) - cd, wait, exit
        //comandos externos e pipes (#) - , uma linha de comando pode receber até 2 comandos externos , unidos por um ‘#’

    //Receba sinal Ctrl-c
        // if tem comandos no buffer 
            //executar os comandos  e esvaziar o buffer
        //else (buffer vazio e com processos filhos em execução )
            //exibir: “Não posso morrer... sou lenta mas sou mãe de família!!!”
            // ocorrer outro Ctrl-c em ate 3 seg
            //exibir: “Ok... você venceu! Adeus!” (… finalizando sua execução em seguida )
        //obs: buffer vazio e sem processos filhos em execucao (O que ocorre?)


    return 0;
}