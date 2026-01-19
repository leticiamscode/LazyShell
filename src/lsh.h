/**
 * @file lsh.h
 * @author Rafael Vieira / Igor Bittencourt / Leticia Mariana
 * @brief Definição das funções auxiliares da LSH.
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef LSH_H
#define LSH_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

// Constantes
#define MAX_LINHA 1000
#define MAX_BUFFER 5
#define MAX_ARGS 3
#define WHITESPACES " \n\t"

// Variáveis Globais
extern char pidLsh[12];
extern char linhaLsh[6];
extern int tamBuffer;
extern int executar;
extern int resistir;
extern int PGIDatual;

// Tratadores de Sinais
void trataSigint(int signum);
void trataSigalrm(int signum);
void trataSigterm(int signum);

// Comandos Internos
void comandoCd(char *arg);
void comandoWait();
void comandoExit();

// Comandos Externos
void comandoExternoUnico(char **comando1, struct sigaction saIgnoreSigint);
void comandoExternoPipe(char **comando1, char **comando2, struct sigaction saIgnoreSigint);

#endif // LSH_H
