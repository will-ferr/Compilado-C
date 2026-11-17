/*
 * tabela_simbolos.h - TABELA DE SÍMBOLOS com ESCOPOS
 *
 * Guarda tudo o que foi declarado (variáveis, parâmetros e funções) e
 * em qual escopo. Cada escopo é uma TABELA HASH, e os escopos formam uma
 * PILHA:
 *
 *      topo ->  [ bloco do if ]      nível 2
 *               [ função main ]      nível 1
 *               [ global      ]      nível 0
 *
 *  - entrar em um bloco  = empilhar um escopo novo
 *  - sair do bloco       = desempilhar (as variáveis dele deixam de existir)
 *  - procurar um nome    = do topo para baixo (o mais interno vence)
 */
#ifndef TABELA_SIMBOLOS_H
#define TABELA_SIMBOLOS_H

#include "ast.h"

#define TAMANHO_HASH 31   /* número de "gavetas" de cada tabela hash */
#define MAX_PARAMETROS 16

typedef enum { VARIAVEL, PARAMETRO, FUNCAO } Categoria;

typedef struct Simbolo {
    char nome[64];
    Categoria categoria;
    Tipo tipo;                 /* tipo da variável ou tipo de retorno da função */
    int linha;
    int nivel;                 /* profundidade do escopo */
    char escopo[64];           /* nome do escopo: "global", "main", ... */

    /* usados só por funções */
    Tipo parametros[MAX_PARAMETROS];
    int qtd_parametros;
    int aceita_mais_argumentos;   /* 1 para o printf (quantidade variável) */
    int predefinida;              /* 1 = função da biblioteca (não é impressa) */

    struct Simbolo *proximo;   /* próximo símbolo na mesma gaveta da hash */
} Simbolo;

typedef struct Escopo {
    Simbolo *tabela[TAMANHO_HASH];
    char nome[64];
    int nivel;
    struct Escopo *anterior;   /* escopo de baixo na pilha */
} Escopo;

void entrar_escopo(const char *nome);
void sair_escopo(void);

Simbolo *inserir_simbolo(const char *nome, Categoria categoria, Tipo tipo, int linha);
Simbolo *buscar_no_escopo_atual(const char *nome);  /* só no topo da pilha */
Simbolo *buscar_simbolo(const char *nome);          /* em todos os escopos */

void imprimir_tabela_simbolos(void);
void liberar_tabela_simbolos(void);

#endif
