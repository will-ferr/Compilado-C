/*
 * ast.c - Criação, impressão e liberação da árvore sintática.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

No *criar_no(TipoNo tipo, const char *texto, int linha)
{
    No *no = calloc(1, sizeof(No)); /* calloc zera todos os campos */
    no->tipo = tipo;
    no->linha = linha;
    snprintf(no->texto, sizeof no->texto, "%s", texto ? texto : "");
    return no;
}

void adicionar_filho(No *pai, No *filho)
{
    if (pai->qtd_filhos == pai->capacidade) { /* vetor cheio: dobra o tamanho */
        pai->capacidade = pai->capacidade ? pai->capacidade * 2 : 4;
        pai->filhos = realloc(pai->filhos, pai->capacidade * sizeof(No *));
    }
    pai->filhos[pai->qtd_filhos++] = filho;
}

const char *nome_do_tipo(Tipo tipo)
{
    switch (tipo) {
    case TIPO_INT:   return "int";
    case TIPO_FLOAT: return "float";
    case TIPO_CHAR:  return "char";
    case TIPO_VOID:  return "void";
    case TIPO_TEXTO: return "texto";
    case TIPO_ERRO:  return "erro";
    default:         return "?";
    }
}

/* Escreve a descrição de um nó, ex.: "Declaração int x". */
static void descrever(const No *no)
{
    switch (no->tipo) {
    case NO_PROGRAMA:     printf("Programa"); break;
    case NO_FUNCAO:       printf("Função %s (retorna %s)", no->texto, nome_do_tipo(no->tipo_declarado)); break;
    case NO_PARAMETRO:    printf("Parâmetro %s %s", nome_do_tipo(no->tipo_declarado), no->texto); break;
    case NO_DECLARACAO:   printf("Declaração %s %s", nome_do_tipo(no->tipo_declarado), no->texto); break;
    case NO_BLOCO:        printf("Bloco { }"); break;
    case NO_IF:           printf("If"); break;
    case NO_WHILE:        printf("While"); break;
    case NO_FOR:          printf("For"); break;
    case NO_RETURN:       printf("Return"); break;
    case NO_COMANDO_EXPR: printf("Comando"); break;
    case NO_ATRIBUICAO:   printf("Atribuição %s =", no->texto); break;
    case NO_OPERACAO:     printf("Operação %s", no->texto); break;
    case NO_OP_UNARIA:    printf("Operação unária %s", no->texto); break;
    case NO_INCREMENTO:   printf("Incremento %s", no->texto); break;
    case NO_CHAMADA:      printf("Chamada %s()", no->texto); break;
    case NO_VARIAVEL:     printf("Variável %s", no->texto); break;
    case NO_INTEIRO:      printf("Inteiro %s", no->texto); break;
    case NO_REAL:         printf("Real %s", no->texto); break;
    case NO_CARACTERE:    printf("Caractere %s", no->texto); break;
    case NO_TEXTO:        printf("Texto %s", no->texto); break;
    }
}

/* Imprime a árvore com recuo: cada nível desloca 4 espaços para a direita.
 * Percorre em pré-ordem: primeiro o nó, depois os filhos (recursão). */
void imprimir_arvore(const No *no, int nivel)
{
    int i;
    if (!no) return;

    printf("  ");
    for (i = 0; i < nivel; i++)
        printf(i == nivel - 1 ? "|-- " : "|   ");

    descrever(no);
    if (no->tipo_calculado != TIPO_NENHUM)          /* tipo da expressão */
        printf("  : %s", nome_do_tipo(no->tipo_calculado));
    if (no->tipo != NO_PROGRAMA)
        printf("   (linha %d)", no->linha);
    printf("\n");

    for (i = 0; i < no->qtd_filhos; i++)
        imprimir_arvore(no->filhos[i], nivel + 1);
}

void liberar_arvore(No *no)
{
    int i;
    if (!no) return;
    for (i = 0; i < no->qtd_filhos; i++)
        liberar_arvore(no->filhos[i]);
    free(no->filhos);
    free(no);
}
