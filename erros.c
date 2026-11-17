/*
 * erros.c - Guarda os erros em um vetor simples e imprime a tabela.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "erros.h"

#define MAX_ERROS 200

typedef struct {
    char fase[16];
    int eh_aviso;        /* 1 = aviso, 0 = erro */
    int linha, coluna;
    char mensagem[256];
} Erro;

static Erro erros[MAX_ERROS];
static int qtd_erros = 0;

/* Função interna usada por registrar_erro e registrar_aviso. */
static void registrar(const char *fase, int eh_aviso, int linha, int coluna,
                      const char *formato, va_list args)
{
    Erro *e;
    if (qtd_erros >= MAX_ERROS) return; /* tabela cheia: ignora o resto */

    e = &erros[qtd_erros++];
    snprintf(e->fase, sizeof e->fase, "%s", fase);
    e->eh_aviso = eh_aviso;
    e->linha = linha;
    e->coluna = coluna;
    vsnprintf(e->mensagem, sizeof e->mensagem, formato, args);
}

void registrar_erro(const char *fase, int linha, int coluna, const char *formato, ...)
{
    va_list args;
    va_start(args, formato);
    registrar(fase, 0, linha, coluna, formato, args);
    va_end(args);
}

void registrar_aviso(const char *fase, int linha, int coluna, const char *formato, ...)
{
    va_list args;
    va_start(args, formato);
    registrar(fase, 1, linha, coluna, formato, args);
    va_end(args);
}

int contar_erros(const char *fase)
{
    int i, total = 0;
    for (i = 0; i < qtd_erros; i++)
        if (!erros[i].eh_aviso && strcmp(erros[i].fase, fase) == 0)
            total++;
    return total;
}

int contar_avisos(void)
{
    int i, total = 0;
    for (i = 0; i < qtd_erros; i++)
        if (erros[i].eh_aviso) total++;
    return total;
}

void imprimir_tabela_erros(void)
{
    int i;
    if (qtd_erros == 0) {
        printf("  Nenhum erro encontrado.\n");
        return;
    }
    /* "Léxico", "Sintático" e "Semântico" têm 1 acento (1 byte a mais),
     * por isso o cabeçalho usa largura 10 e as linhas usam 11 */
    printf("  %-3s %-10s %-6s %-8s %s\n", "#", "Fase", "Tipo", "Posição", "Mensagem");
    printf("  --- ---------- ------ ------- ----------------------------------------\n");
    for (i = 0; i < qtd_erros; i++) {
        char posicao[32];
        /* o semântico só conhece a linha (coluna = 0) */
        if (erros[i].coluna > 0)
            snprintf(posicao, sizeof posicao, "%d:%d", erros[i].linha, erros[i].coluna);
        else
            snprintf(posicao, sizeof posicao, "%d", erros[i].linha);
        printf("  %-3d %-11s %-6s %-7s %s\n", i + 1, erros[i].fase,
               erros[i].eh_aviso ? "AVISO" : "ERRO", posicao, erros[i].mensagem);
    }
}
