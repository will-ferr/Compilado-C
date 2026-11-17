/*
 * ===========================================================================
 *  Analisador Léxico, Sintático e Semântico para um subconjunto de C
 *  (versão básica / didática)
 * ===========================================================================
 *  Curso:      Ciência da Computação - FUMEC
 *  Disciplina: Compiladores
 *  Professora: Amanda Danielle Lima de Oliveira
 *
 *  Integrantes:
 *    - BERNARDO SIMÕES MELO
 *    - BRUNO SOARES JORDÃO LOPES
 *    - FRANCO COUTO ALVES PINTO
 *    - WILLIAM FERREIRA DE BRITO
 * ===========================================================================
 *
 *  Como usar:   compilador arquivo.c
 *
 *  O programa executa as 3 fases, uma depois da outra:
 *
 *    código-fonte --> [LÉXICO] --> tokens --> [SINTÁTICO] --> árvore --> [SEMÂNTICO]
 *
 *  Se uma fase encontra erros, as seguintes não são executadas
 *  (não faz sentido montar a árvore de um texto com caracteres inválidos).
 */
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>   /* para mostrar acentos corretamente no Windows */
#endif

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "semantic.h"
#include "tabela_simbolos.h"
#include "erros.h"

/* Lê o arquivo inteiro para uma string. Devolve NULL se não conseguir. */
static char *ler_arquivo(const char *caminho)
{
    FILE *arquivo = fopen(caminho, "rb");
    char *conteudo;
    long tamanho;

    if (!arquivo) return NULL;
    fseek(arquivo, 0, SEEK_END);
    tamanho = ftell(arquivo);
    fseek(arquivo, 0, SEEK_SET);

    conteudo = malloc(tamanho + 1);
    tamanho = (long)fread(conteudo, 1, tamanho, arquivo);
    conteudo[tamanho] = '\0';
    fclose(arquivo);
    return conteudo;
}

static void titulo(const char *texto)
{
    printf("\n==================================================================\n");
    printf(" %s\n", texto);
    printf("==================================================================\n");
}

int main(int argc, char *argv[])
{
    char *codigo;
    ListaTokens tokens;
    No *arvore = NULL;
    int erros_lexicos, erros_sintaticos = 0, erros_semanticos = 0;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc < 2) {
        printf("Uso: %s arquivo.c\n", argv[0]);
        return 2;
    }
    codigo = ler_arquivo(argv[1]);
    if (!codigo) {
        printf("Erro: não foi possível abrir o arquivo '%s'\n", argv[1]);
        return 2;
    }
    printf("Analisando o arquivo: %s\n", argv[1]);

    /* ----------------------- FASE 1: LÉXICA ----------------------- */
    tokens = analisar_lexico(codigo);
    erros_lexicos = contar_erros("Léxico");

    titulo("FASE 1 - ANÁLISE LÉXICA: lista de tokens");
    imprimir_tokens(&tokens);

    titulo("TABELA DE SÍMBOLOS INICIAL (identificadores encontrados)");
    imprimir_tabela_inicial();

    /* --------------------- FASE 2: SINTÁTICA ---------------------- */
    titulo("FASE 2 - ANÁLISE SINTÁTICA");
    if (erros_lexicos > 0) {
        printf("  Não executada: o arquivo tem erros léxicos.\n");
    } else {
        arvore = analisar_sintatico(&tokens, &erros_sintaticos);
        if (erros_sintaticos > 0) {
            printf("  O programa NÃO segue a gramática (%d erro(s)).\n", erros_sintaticos);
        } else {
            printf("  OK: o programa segue a gramática.\n");

            /* -------------------- FASE 3: SEMÂNTICA ------------------- */
            erros_semanticos = analisar_semantico(arvore);

            titulo("ÁRVORE SINTÁTICA ABSTRATA (AST) com os tipos calculados");
            imprimir_arvore(arvore, 0);

            titulo("FASE 3 - ANÁLISE SEMÂNTICA: tabela de símbolos");
            imprimir_tabela_simbolos();
            if (erros_semanticos == 0)
                printf("\n  OK: nenhum erro semântico.\n");
        }
    }

    /* ------------------------ ERROS ------------------------------- */
    titulo("TABELA DE ERROS");
    imprimir_tabela_erros();

    printf("\nResumo: %d erro(s) léxico(s), %d sintático(s), %d semântico(s), %d aviso(s)\n",
           erros_lexicos, erros_sintaticos, erros_semanticos, contar_avisos());

    liberar_arvore(arvore);
    liberar_tabela_simbolos();
    liberar_tokens(&tokens);
    free(codigo);

    /* código de saída: 0 = sem erros, 1 = com erros */
    return (erros_lexicos + erros_sintaticos + erros_semanticos) > 0 ? 1 : 0;
}
