/*
 * tabela_simbolos.c - Implementação da tabela hash e da pilha de escopos.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabela_simbolos.h"

static Escopo *escopo_atual = NULL;   /* topo da pilha */

/* Lista com TODOS os símbolos já criados. Quando um escopo é desempilhado
 * os símbolos somem da busca, mas continuam aqui para a impressão final. */
#define MAX_SIMBOLOS 1000
static Simbolo *todos[MAX_SIMBOLOS];
static int qtd_todos = 0;

/* Função de hash: transforma o nome em um número entre 0 e TAMANHO_HASH-1.
 * Nomes diferentes tendem a cair em gavetas diferentes, então a busca
 * olha poucos símbolos em vez de percorrer a tabela inteira. */
static unsigned int hash(const char *nome)
{
    unsigned int h = 0;
    while (*nome)
        h = h * 31 + (unsigned char)*nome++;
    return h % TAMANHO_HASH;
}

void entrar_escopo(const char *nome)
{
    Escopo *e = calloc(1, sizeof(Escopo));   /* todas as gavetas começam vazias */
    snprintf(e->nome, sizeof e->nome, "%s", nome);
    e->nivel = escopo_atual ? escopo_atual->nivel + 1 : 0;
    e->anterior = escopo_atual;             /* empilha */
    escopo_atual = e;
}

void sair_escopo(void)
{
    Escopo *e = escopo_atual;
    if (!e) return;
    escopo_atual = e->anterior;              /* desempilha */
    free(e);                                 /* os símbolos continuam em 'todos' */
}

Simbolo *inserir_simbolo(const char *nome, Categoria categoria, Tipo tipo, int linha)
{
    Simbolo *s = calloc(1, sizeof(Simbolo));
    unsigned int gaveta = hash(nome);

    snprintf(s->nome, sizeof s->nome, "%s", nome);
    s->categoria = categoria;
    s->tipo = tipo;
    s->linha = linha;
    s->nivel = escopo_atual->nivel;
    snprintf(s->escopo, sizeof s->escopo, "%s", escopo_atual->nome);

    /* insere no início da lista encadeada da gaveta */
    s->proximo = escopo_atual->tabela[gaveta];
    escopo_atual->tabela[gaveta] = s;

    if (qtd_todos < MAX_SIMBOLOS) todos[qtd_todos++] = s;
    return s;
}

/* Procura o nome em um único escopo. */
static Simbolo *buscar_em(Escopo *e, const char *nome)
{
    Simbolo *s;
    for (s = e->tabela[hash(nome)]; s; s = s->proximo)
        if (strcmp(s->nome, nome) == 0) return s;
    return NULL;
}

Simbolo *buscar_no_escopo_atual(const char *nome)
{
    return escopo_atual ? buscar_em(escopo_atual, nome) : NULL;
}

Simbolo *buscar_simbolo(const char *nome)
{
    Escopo *e;
    for (e = escopo_atual; e; e = e->anterior) {   /* do topo para baixo */
        Simbolo *s = buscar_em(e, nome);
        if (s) return s;
    }
    return NULL;
}

static const char *nome_categoria(Categoria c)
{
    if (c == VARIAVEL) return "variável";
    if (c == PARAMETRO) return "parâmetro";
    return "função";
}

/* Imprime um texto e completa com espaços até 'largura' colunas.
 * (O printf("%-10s") conta BYTES, e letras acentuadas ocupam 2 bytes em
 * UTF-8; por isso contamos só os bytes que começam uma letra.) */
static void imprimir_coluna(const char *texto, int largura)
{
    int visiveis = 0;
    const char *c;
    for (c = texto; *c; c++)
        if ((*c & 0xC0) != 0x80) visiveis++;
    printf("%s", texto);
    for (; visiveis < largura; visiveis++) printf(" ");
    printf(" ");
}

void imprimir_tabela_simbolos(void)
{
    int i, j;
    printf("  %-14s %-10s %-22s %-22s %-6s %s\n",
           "Nome", "Categoria", "Tipo", "Escopo", "Nível", "Linha");
    printf("  -------------- ---------- ---------------------- ---------------------- ----- -----\n");
    for (i = 0; i < qtd_todos; i++) {
        Simbolo *s = todos[i];
        char tipo[160];   /* cabe "float (float, float, ...)" com 16 parâmetros */
        if (s->predefinida) continue;

        if (s->categoria == FUNCAO) {   /* ex.: int (int, int) */
            snprintf(tipo, sizeof tipo, "%s (", nome_do_tipo(s->tipo));
            for (j = 0; j < s->qtd_parametros; j++) {
                strcat(tipo, j ? ", " : "");
                strcat(tipo, nome_do_tipo(s->parametros[j]));
            }
            strcat(tipo, ")");
        } else {
            snprintf(tipo, sizeof tipo, "%s", nome_do_tipo(s->tipo));
        }
        printf("  %-14s ", s->nome);
        imprimir_coluna(nome_categoria(s->categoria), 10);
        printf("%-22s %-22s %-5d %d\n", tipo, s->escopo, s->nivel, s->linha);
    }
}

void liberar_tabela_simbolos(void)
{
    int i;
    while (escopo_atual) sair_escopo();
    for (i = 0; i < qtd_todos; i++) free(todos[i]);
    qtd_todos = 0;
}
