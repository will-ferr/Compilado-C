/*
 * lexer.c - ANÁLISE LÉXICA (1ª fase)
 *
 * Ideia geral: olhamos o caractere atual e decidimos que tipo de token
 * está começando:
 *
 *   letra ou '_'   -> identificador ou palavra-chave   (ler_identificador)
 *   dígito         -> número inteiro ou real           (ler_numero)
 *   aspas  "       -> texto (string)                   (ler_texto)
 *   apóstrofo '    -> caractere                        (ler_caractere)
 *   // ou / *      -> comentário
 *   #              -> diretiva (#include), ignorada
 *   outro          -> operador ou delimitador          (ler_operador)
 *
 * Se nada servir, é um ERRO LÉXICO (ex.: '@'), registrado com linha e coluna.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "erros.h"

/* ------------------------------------------------------------------ */
/* Estado do scanner (variáveis globais deste arquivo)                 */
/* ------------------------------------------------------------------ */

static const char *codigo;   /* texto do arquivo-fonte */
static int pos;              /* posição do caractere atual */
static int linha, coluna;    /* posição atual para mensagens de erro */
static ListaTokens lista;    /* tokens produzidos */

/* Tabela de símbolos inicial: cada identificador distinto encontrado. */
#define MAX_IDENTIFICADORES 500
typedef struct {
    char nome[64];
    int linha;           /* primeira vez que apareceu */
    int ocorrencias;
} EntradaInicial;
static EntradaInicial tabela_inicial[MAX_IDENTIFICADORES];
static int qtd_identificadores = 0;

/* Palavras-chave da linguagem e o token de cada uma. */
static const struct {
    const char *palavra;
    TipoToken tipo;
} palavras_chave[] = {
    { "int", T_INT },     { "float", T_FLOAT }, { "char", T_CHAR },
    { "void", T_VOID },   { "if", T_IF },       { "else", T_ELSE },
    { "while", T_WHILE }, { "for", T_FOR },     { "return", T_RETURN },
};
#define QTD_PALAVRAS_CHAVE (int)(sizeof palavras_chave / sizeof palavras_chave[0])

/* ------------------------------------------------------------------ */
/* Funções básicas de leitura                                          */
/* ------------------------------------------------------------------ */

static char atual(void)  { return codigo[pos]; }
static char proximo(void) { return codigo[pos] ? codigo[pos + 1] : '\0'; }

/* Anda um caractere, atualizando linha e coluna. */
static void avancar(void)
{
    if (codigo[pos] == '\0') return;
    if (codigo[pos] == '\n') {
        linha++;
        coluna = 1;
    } else if ((codigo[pos] & 0xC0) != 0x80) {
        /* letras acentuadas ocupam 2 bytes em UTF-8; só o 1º conta como coluna */
        coluna++;
    }
    pos++;
}

/* Cria um token com o texto que vai de 'inicio' até a posição atual. */
static void adicionar_token(TipoToken tipo, int inicio, int lin, int col)
{
    Token *t;
    int tamanho = pos - inicio;

    if (lista.quantidade == lista.capacidade) { /* vetor cheio: dobra o tamanho */
        lista.capacidade = lista.capacidade ? lista.capacidade * 2 : 64;
        lista.itens = realloc(lista.itens, lista.capacidade * sizeof(Token));
    }
    t = &lista.itens[lista.quantidade++];
    t->tipo = tipo;
    t->linha = lin;
    t->coluna = col;
    if (tamanho > 255) tamanho = 255;          /* corta lexemas muito longos */
    memcpy(t->lexema, codigo + inicio, tamanho);
    t->lexema[tamanho] = '\0';
}

/* Guarda o identificador na tabela de símbolos inicial (sem repetir). */
static void registrar_identificador(const char *nome, int lin)
{
    int i;
    for (i = 0; i < qtd_identificadores; i++) {
        if (strcmp(tabela_inicial[i].nome, nome) == 0) {
            tabela_inicial[i].ocorrencias++;
            return;
        }
    }
    if (qtd_identificadores < MAX_IDENTIFICADORES) {
        snprintf(tabela_inicial[qtd_identificadores].nome, 64, "%s", nome);
        tabela_inicial[qtd_identificadores].linha = lin;
        tabela_inicial[qtd_identificadores].ocorrencias = 1;
        qtd_identificadores++;
    }
}

/* ------------------------------------------------------------------ */
/* Reconhecedores de cada tipo de token                                */
/* ------------------------------------------------------------------ */

/* identificador:  (letra | _) { letra | dígito | _ }  */
static void ler_identificador(int inicio, int lin, int col)
{
    int i;
    Token *t;

    while (isalnum((unsigned char)atual()) || atual() == '_')
        avancar();
    adicionar_token(T_ID, inicio, lin, col);

    /* Se o texto for uma palavra-chave, troca o tipo do token. */
    t = &lista.itens[lista.quantidade - 1];
    for (i = 0; i < QTD_PALAVRAS_CHAVE; i++) {
        if (strcmp(t->lexema, palavras_chave[i].palavra) == 0) {
            t->tipo = palavras_chave[i].tipo;
            return;
        }
    }
    registrar_identificador(t->lexema, lin);
}

/* número inteiro:  dígito { dígito }
 * número real:     dígito { dígito } '.' dígito { dígito }          */
static void ler_numero(int inicio, int lin, int col)
{
    TipoToken tipo = T_NUM_INT;

    while (isdigit((unsigned char)atual())) avancar();

    if (atual() == '.' && isdigit((unsigned char)proximo())) {
        tipo = T_NUM_REAL;
        avancar(); /* o ponto */
        while (isdigit((unsigned char)atual())) avancar();
    }
    if (tipo == T_NUM_REAL && (atual() == 'f' || atual() == 'F'))
        avancar(); /* sufixo opcional: 2.5f */

    /* Letra ou ponto grudado no número: "12abc", "1.2.3" -> erro léxico */
    if (isalpha((unsigned char)atual()) || atual() == '_' || atual() == '.') {
        while (isalnum((unsigned char)atual()) || atual() == '_' || atual() == '.')
            avancar();
        adicionar_token(T_ERRO, inicio, lin, col);
        registrar_erro("Léxico", lin, col, "número malformado '%s'",
                       lista.itens[lista.quantidade - 1].lexema);
        return;
    }
    adicionar_token(tipo, inicio, lin, col);
}

/* texto:  '"' { qualquer caractere exceto '"' ou quebra de linha } '"' */
static void ler_texto(int inicio, int lin, int col)
{
    avancar(); /* aspas de abertura */
    while (atual() != '"') {
        if (atual() == '\0' || atual() == '\n') {
            adicionar_token(T_ERRO, inicio, lin, col);
            registrar_erro("Léxico", lin, col, "texto (string) não foi fechado com aspas");
            return;
        }
        if (atual() == '\\') avancar(); /* \" ou \n: pula a barra e o próximo */
        avancar();
    }
    avancar(); /* aspas de fechamento */
    adicionar_token(T_TEXTO, inicio, lin, col);
}

/* caractere:  '\'' ( caractere | '\\' caractere ) '\'' */
static void ler_caractere(int inicio, int lin, int col)
{
    avancar(); /* apóstrofo de abertura */

    if (atual() == '\'') { /* '' */
        avancar();
        adicionar_token(T_ERRO, inicio, lin, col);
        registrar_erro("Léxico", lin, col, "caractere vazio ''");
        return;
    }
    if (atual() == '\\') avancar(); /* sequência de escape: '\n' */
    if (atual() != '\0' && atual() != '\n') avancar();

    if (atual() != '\'') {
        /* 'abc' ou sem fechamento: consome até o ' ou o fim da linha */
        while (atual() != '\'' && atual() != '\n' && atual() != '\0') avancar();
        if (atual() == '\'') avancar();
        adicionar_token(T_ERRO, inicio, lin, col);
        registrar_erro("Léxico", lin, col, "caractere malformado %s (use apenas um caractere entre apóstrofos)",
                       lista.itens[lista.quantidade - 1].lexema);
        return;
    }
    avancar(); /* apóstrofo de fechamento */
    adicionar_token(T_CARACTERE, inicio, lin, col);
}

/* Lê um operador que pode ter 1 ou 2 caracteres.
 * Ex.: '=' sozinho é atribuição; "==" é comparação. */
static TipoToken operador_duplo(char segundo, TipoToken se_duplo, TipoToken se_simples)
{
    avancar();
    if (atual() == segundo) {
        avancar();
        return se_duplo;
    }
    return se_simples;
}

static void ler_operador(int inicio, int lin, int col)
{
    TipoToken tipo;
    char c = atual();

    switch (c) {
    case '+': tipo = operador_duplo('+', T_INCREMENTO, T_MAIS); break;
    case '-': tipo = operador_duplo('-', T_DECREMENTO, T_MENOS); break;
    case '=': tipo = operador_duplo('=', T_IGUAL, T_ATRIBUICAO); break;
    case '!': tipo = operador_duplo('=', T_DIFERENTE, T_NAO); break;
    case '<': tipo = operador_duplo('=', T_MENOR_IGUAL, T_MENOR); break;
    case '>': tipo = operador_duplo('=', T_MAIOR_IGUAL, T_MAIOR); break;
    case '&': tipo = operador_duplo('&', T_E, T_ERRO); break;   /* só && existe */
    case '|': tipo = operador_duplo('|', T_OU, T_ERRO); break;  /* só || existe */
    case '*': avancar(); tipo = T_VEZES; break;
    case '/': avancar(); tipo = T_DIVIDIDO; break;
    case '%': avancar(); tipo = T_RESTO; break;
    case '(': avancar(); tipo = T_ABRE_PAR; break;
    case ')': avancar(); tipo = T_FECHA_PAR; break;
    case '{': avancar(); tipo = T_ABRE_CHAVE; break;
    case '}': avancar(); tipo = T_FECHA_CHAVE; break;
    case ';': avancar(); tipo = T_PONTO_VIRGULA; break;
    case ',': avancar(); tipo = T_VIRGULA; break;
    default:
        /* caractere que não pertence à linguagem (@, $, ç fora de texto...) */
        avancar();
        while ((atual() & 0xC0) == 0x80) avancar(); /* resto de um caractere UTF-8 */
        tipo = T_ERRO;
        break;
    }

    adicionar_token(tipo, inicio, lin, col);
    if (tipo == T_ERRO)
        registrar_erro("Léxico", lin, col, "caractere inválido '%s'",
                       lista.itens[lista.quantidade - 1].lexema);
}

/* ------------------------------------------------------------------ */
/* Laço principal                                                      */
/* ------------------------------------------------------------------ */

ListaTokens analisar_lexico(const char *texto)
{
    codigo = texto;
    pos = 0;
    linha = 1;
    coluna = 1;
    lista.itens = NULL;
    lista.quantidade = 0;
    lista.capacidade = 0;

    for (;;) {
        int inicio, lin, col;
        char c;

        /* 1) pula espaços, tabs e quebras de linha */
        while (isspace((unsigned char)atual())) avancar();

        /* 2) guarda onde o token começa */
        inicio = pos;
        lin = linha;
        col = coluna;
        c = atual();

        /* 3) decide o tipo de token pelo primeiro caractere */
        if (c == '\0') {
            adicionar_token(T_FIM, inicio, lin, col);
            break;
        } else if (c == '#') {                     /* #include <stdio.h> */
            while (atual() != '\n' && atual() != '\0') avancar();
            adicionar_token(T_DIRETIVA, inicio, lin, col);
        } else if (c == '/' && proximo() == '/') { /* comentário de linha */
            while (atual() != '\n' && atual() != '\0') avancar();
            adicionar_token(T_COMENTARIO, inicio, lin, col);
        } else if (c == '/' && proximo() == '*') { /* comentário de bloco */
            avancar();
            avancar();
            while (atual() != '\0' && !(atual() == '*' && proximo() == '/')) avancar();
            if (atual() == '\0') {
                registrar_erro("Léxico", lin, col, "comentário de bloco não foi fechado com */");
                adicionar_token(T_ERRO, inicio, lin, col);
            } else {
                avancar();
                avancar();
                adicionar_token(T_COMENTARIO, inicio, lin, col);
            }
        } else if (isalpha((unsigned char)c) || c == '_') {
            ler_identificador(inicio, lin, col);
        } else if (isdigit((unsigned char)c)) {
            ler_numero(inicio, lin, col);
        } else if (c == '"') {
            ler_texto(inicio, lin, col);
        } else if (c == '\'') {
            ler_caractere(inicio, lin, col);
        } else {
            ler_operador(inicio, lin, col);
        }
    }
    return lista;
}

/* ------------------------------------------------------------------ */
/* Impressão                                                           */
/* ------------------------------------------------------------------ */

const char *texto_do_token(TipoToken tipo)
{
    switch (tipo) {
    case T_FIM:            return "fim do arquivo";
    case T_ERRO:           return "erro";
    case T_ID:             return "identificador";
    case T_NUM_INT:        return "número inteiro";
    case T_NUM_REAL:       return "número real";
    case T_CARACTERE:      return "caractere";
    case T_TEXTO:          return "texto";
    case T_INT:            return "int";
    case T_FLOAT:          return "float";
    case T_CHAR:           return "char";
    case T_VOID:           return "void";
    case T_IF:             return "if";
    case T_ELSE:           return "else";
    case T_WHILE:          return "while";
    case T_FOR:            return "for";
    case T_RETURN:         return "return";
    case T_MAIS:           return "+";
    case T_MENOS:          return "-";
    case T_VEZES:          return "*";
    case T_DIVIDIDO:       return "/";
    case T_RESTO:          return "%";
    case T_INCREMENTO:     return "++";
    case T_DECREMENTO:     return "--";
    case T_ATRIBUICAO:     return "=";
    case T_IGUAL:          return "==";
    case T_DIFERENTE:      return "!=";
    case T_MENOR:          return "<";
    case T_MAIOR:          return ">";
    case T_MENOR_IGUAL:    return "<=";
    case T_MAIOR_IGUAL:    return ">=";
    case T_E:              return "&&";
    case T_OU:             return "||";
    case T_NAO:            return "!";
    case T_ABRE_PAR:       return "(";
    case T_FECHA_PAR:      return ")";
    case T_ABRE_CHAVE:     return "{";
    case T_FECHA_CHAVE:    return "}";
    case T_PONTO_VIRGULA:  return ";";
    case T_VIRGULA:        return ",";
    case T_COMENTARIO:     return "comentário";
    case T_DIRETIVA:       return "diretiva";
    }
    return "?";
}

/* Categoria do token, para a listagem. */
static const char *categoria(TipoToken tipo)
{
    if (tipo == T_ID) return "IDENTIFICADOR";
    if (tipo == T_NUM_INT || tipo == T_NUM_REAL) return "NUMERO";
    if (tipo == T_CARACTERE) return "CARACTERE";
    if (tipo == T_TEXTO) return "TEXTO";
    if (tipo >= T_INT && tipo <= T_RETURN) return "PALAVRA_CHAVE";
    if (tipo >= T_MAIS && tipo <= T_NAO) return "OPERADOR";
    if (tipo >= T_ABRE_PAR && tipo <= T_VIRGULA) return "DELIMITADOR";
    if (tipo == T_COMENTARIO) return "COMENTARIO";
    if (tipo == T_DIRETIVA) return "DIRETIVA";
    if (tipo == T_FIM) return "FIM";
    return "ERRO";
}

void imprimir_tokens(const ListaTokens *l)
{
    int i;
    printf("  %-8s %-15s %s\n", "Lin:Col", "Categoria", "Lexema");
    printf("  -------- --------------- ------------------------------\n");
    for (i = 0; i < l->quantidade; i++) {
        const Token *t = &l->itens[i];
        char posicao[32];
        char lexema[48];
        int j;

        snprintf(posicao, sizeof posicao, "%d:%d", t->linha, t->coluna);
        /* comentários longos: mostra só o começo, numa linha */
        snprintf(lexema, sizeof lexema, "%.47s", t->lexema);
        for (j = 0; lexema[j]; j++)
            if (lexema[j] == '\n' || lexema[j] == '\r') lexema[j] = ' ';
        printf("  %-8s %-15s %s%s\n", posicao, categoria(t->tipo),
               t->tipo == T_FIM ? "<EOF>" : lexema,
               strlen(t->lexema) >= sizeof lexema ? "..." : "");
    }
}

void imprimir_tabela_inicial(void)
{
    int i;
    if (qtd_identificadores == 0) {
        printf("  (nenhum identificador)\n");
        return;
    }
    printf("  %-20s %-15s %s\n", "Identificador", "Primeira linha", "Ocorrências");
    printf("  -------------------- --------------- -----------\n");
    for (i = 0; i < qtd_identificadores; i++)
        printf("  %-20s %-15d %d\n", tabela_inicial[i].nome,
               tabela_inicial[i].linha, tabela_inicial[i].ocorrencias);
}

void liberar_tokens(ListaTokens *l)
{
    free(l->itens);
    l->itens = NULL;
    l->quantidade = l->capacidade = 0;
}
