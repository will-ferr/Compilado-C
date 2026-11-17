/*
 * parser.c - ANÁLISE SINTÁTICA (2ª fase)
 *
 * Técnica: DESCENDENTE RECURSIVA (LL).
 *   - Cada regra da gramática (veja o README) virou uma função em C.
 *   - A função olha o token atual para decidir qual regra aplicar.
 *   - Uma regra que usa outra regra simplesmente chama a função dela.
 *
 * Exemplo:  regra      comando_while -> 'while' '(' expressao ')' comando
 *           função     comando_while() { esperar(WHILE); esperar('(');
 *                                        expressao(); esperar(')'); comando(); }
 *
 * Tratamento de erros (MODO PÂNICO):
 *   ao achar um erro, ele é registrado e o parser entra em "pânico": para de
 *   reportar erros (para não gerar uma avalanche de mensagens) e, ao fim do
 *   comando atual, descarta tokens até um ponto seguro (';' ou '}').
 *   Depois disso volta ao normal e continua procurando outros erros.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "parser.h"
#include "erros.h"

/* ------------------------------------------------------------------ */
/* Estado do parser                                                    */
/* ------------------------------------------------------------------ */

static Token *tokens;   /* tokens úteis (sem comentários e diretivas) */
static int qtd_tokens;
static int pos;         /* índice do token atual */
static int panico;      /* 1 = em modo pânico (não reporta novos erros) */
static int erros;

/* ------------------------------------------------------------------ */
/* Funções auxiliares                                                  */
/* ------------------------------------------------------------------ */

static Token *atual(void) { return &tokens[pos]; }

/* Olha k tokens à frente, sem consumir (lookahead). */
static Token *espiar(int k)
{
    int i = pos + k;
    if (i >= qtd_tokens) i = qtd_tokens - 1; /* o último é sempre T_FIM */
    return &tokens[i];
}

static int verificar(TipoToken tipo) { return atual()->tipo == tipo; }

static void avancar(void)
{
    if (atual()->tipo != T_FIM) pos++;
}

/* Se o token atual for do tipo pedido, consome e devolve 1. */
static int aceitar(TipoToken tipo)
{
    if (!verificar(tipo)) return 0;
    avancar();
    return 1;
}

static void erro_sintatico(int linha, int coluna, const char *formato, ...)
{
    char mensagem[256];
    va_list args;

    if (panico) return;       /* já estamos tratando um erro */
    panico = 1;
    erros++;

    va_start(args, formato);
    vsnprintf(mensagem, sizeof mensagem, formato, args);
    va_end(args);
    registrar_erro("Sintático", linha, coluna, "%s", mensagem);
}

/* Exige um token. Se não estiver lá, é erro sintático. */
static void esperar(TipoToken tipo, const char *contexto)
{
    Token *t = atual();
    if (aceitar(tipo)) return;

    /* Para ';' e ')' faltando, aponta o fim do token anterior
     * (onde o símbolo deveria estar), e não o token seguinte. */
    if ((tipo == T_PONTO_VIRGULA || tipo == T_FECHA_PAR) && pos > 0) {
        Token *ant = &tokens[pos - 1];
        erro_sintatico(ant->linha, ant->coluna + (int)strlen(ant->lexema),
                       "esperado '%s' %s, mas encontrado '%s'",
                       texto_do_token(tipo), contexto, t->tipo == T_FIM ? "fim do arquivo" : t->lexema);
    } else {
        erro_sintatico(t->linha, t->coluna, "esperado '%s' %s, mas encontrado '%s'",
                       texto_do_token(tipo), contexto, t->tipo == T_FIM ? "fim do arquivo" : t->lexema);
    }
}

static int eh_tipo(TipoToken t)
{
    return t == T_INT || t == T_FLOAT || t == T_CHAR || t == T_VOID;
}

/* Lê uma palavra de tipo (int, float...) e devolve o Tipo correspondente. */
static Tipo ler_tipo(void)
{
    TipoToken t = atual()->tipo;
    avancar();
    if (t == T_INT) return TIPO_INT;
    if (t == T_FLOAT) return TIPO_FLOAT;
    if (t == T_CHAR) return TIPO_CHAR;
    return TIPO_VOID;
}

/* Recuperação de erro: descarta tokens até um ponto seguro. */
static void sincronizar(void)
{
    while (!verificar(T_FIM)) {
        if (aceitar(T_PONTO_VIRGULA)) break;              /* fim do comando ruim */
        if (verificar(T_FECHA_CHAVE) || verificar(T_ABRE_CHAVE)) break;
        if (eh_tipo(atual()->tipo) || verificar(T_IF) || verificar(T_WHILE) ||
            verificar(T_FOR) || verificar(T_RETURN)) break; /* início de comando */
        avancar();
    }
    panico = 0;
}

/* As funções abaixo chamam umas às outras, por isso os protótipos. */
static No *comando(void);
static No *bloco(void);
static No *expressao(void);

/* ================================================================== */
/* EXPRESSÕES                                                          */
/* Cada nível de precedência é uma função. Quanto mais "funda" a       */
/* função, maior a precedência do operador:                            */
/*   ||  <  &&  <  == !=  <  < > <= >=  <  + -  <  * / %  <  unário    */
/* ================================================================== */

/* Cria um nó de operação binária: esquerda OP direita */
static No *operacao(Token *op, No *esquerda, No *direita)
{
    No *no = criar_no(NO_OPERACAO, op->lexema, op->linha);
    adicionar_filho(no, esquerda);
    adicionar_filho(no, direita);
    return no;
}

/* primaria -> NUM_INT | NUM_REAL | CARACTERE | TEXTO
 *           | ID | ID '(' [ argumentos ] ')' | '(' expressao ')'  */
static No *primaria(void)
{
    Token *t = atual();
    No *no;

    switch (t->tipo) {
    case T_NUM_INT:   avancar(); return criar_no(NO_INTEIRO, t->lexema, t->linha);
    case T_NUM_REAL:  avancar(); return criar_no(NO_REAL, t->lexema, t->linha);
    case T_CARACTERE: avancar(); return criar_no(NO_CARACTERE, t->lexema, t->linha);
    case T_TEXTO:     avancar(); return criar_no(NO_TEXTO, t->lexema, t->linha);

    case T_ID:
        avancar();
        if (aceitar(T_ABRE_PAR)) {
            /* chamada de função: nome(arg1, arg2, ...) */
            no = criar_no(NO_CHAMADA, t->lexema, t->linha);
            if (!verificar(T_FECHA_PAR)) {
                do {
                    adicionar_filho(no, expressao());
                } while (aceitar(T_VIRGULA));
            }
            esperar(T_FECHA_PAR, "para fechar a chamada da função");
            return no;
        }
        return criar_no(NO_VARIAVEL, t->lexema, t->linha);

    case T_ABRE_PAR:
        avancar();
        no = expressao();
        esperar(T_FECHA_PAR, "para fechar o parêntese");
        return no;

    default:
        erro_sintatico(t->linha, t->coluna, "expressão esperada, mas encontrado '%s'",
                       t->tipo == T_FIM ? "fim do arquivo" : t->lexema);
        /* devolve um nó qualquer para a árvore continuar inteira */
        return criar_no(NO_INTEIRO, "0", t->linha);
    }
}

/* posfixa -> primaria [ '++' | '--' ] */
static No *posfixa(void)
{
    No *no = primaria();
    Token *t = atual();
    if (verificar(T_INCREMENTO) || verificar(T_DECREMENTO)) {
        No *inc = criar_no(NO_INCREMENTO, t->lexema, t->linha);
        avancar();
        if (no->tipo != NO_VARIAVEL)
            erro_sintatico(t->linha, t->coluna, "'%s' só pode ser aplicado a uma variável", t->lexema);
        adicionar_filho(inc, no);
        return inc;
    }
    return no;
}

/* unaria -> ( '-' | '!' ) unaria | posfixa */
static No *unaria(void)
{
    Token *t = atual();
    if (verificar(T_MENOS) || verificar(T_NAO)) {
        No *no = criar_no(NO_OP_UNARIA, t->lexema, t->linha);
        avancar();
        adicionar_filho(no, unaria());
        return no;
    }
    return posfixa();
}

/* termo -> unaria { ( '*' | '/' | '%' ) unaria } */
static No *termo(void)
{
    No *esq = unaria();
    while (verificar(T_VEZES) || verificar(T_DIVIDIDO) || verificar(T_RESTO)) {
        Token *op = atual();
        avancar();
        esq = operacao(op, esq, unaria());
    }
    return esq;
}

/* soma -> termo { ( '+' | '-' ) termo } */
static No *soma(void)
{
    No *esq = termo();
    while (verificar(T_MAIS) || verificar(T_MENOS)) {
        Token *op = atual();
        avancar();
        esq = operacao(op, esq, termo());
    }
    return esq;
}

/* relacional -> soma { ( '<' | '>' | '<=' | '>=' ) soma } */
static No *relacional(void)
{
    No *esq = soma();
    while (verificar(T_MENOR) || verificar(T_MAIOR) ||
           verificar(T_MENOR_IGUAL) || verificar(T_MAIOR_IGUAL)) {
        Token *op = atual();
        avancar();
        esq = operacao(op, esq, soma());
    }
    return esq;
}

/* igualdade -> relacional { ( '==' | '!=' ) relacional } */
static No *igualdade(void)
{
    No *esq = relacional();
    while (verificar(T_IGUAL) || verificar(T_DIFERENTE)) {
        Token *op = atual();
        avancar();
        esq = operacao(op, esq, relacional());
    }
    return esq;
}

/* e_logico -> igualdade { '&&' igualdade } */
static No *e_logico(void)
{
    No *esq = igualdade();
    while (verificar(T_E)) {
        Token *op = atual();
        avancar();
        esq = operacao(op, esq, igualdade());
    }
    return esq;
}

/* ou_logico -> e_logico { '||' e_logico } */
static No *ou_logico(void)
{
    No *esq = e_logico();
    while (verificar(T_OU)) {
        Token *op = atual();
        avancar();
        esq = operacao(op, esq, e_logico());
    }
    return esq;
}

/* expressao -> ID '=' expressao | ou_logico
 * Para saber se é atribuição, olhamos 1 token à frente: "x =" */
static No *expressao(void)
{
    if (verificar(T_ID) && espiar(1)->tipo == T_ATRIBUICAO) {
        Token *var = atual();
        No *no = criar_no(NO_ATRIBUICAO, var->lexema, var->linha);
        avancar(); /* nome */
        avancar(); /* '='  */
        adicionar_filho(no, expressao());
        return no;
    }
    return ou_logico();
}

/* ================================================================== */
/* COMANDOS                                                            */
/* ================================================================== */

/* declaracao -> tipo ID [ '=' expressao ] ';' */
static No *declaracao(void)
{
    int linha = atual()->linha;
    Tipo tipo = ler_tipo();
    No *no;

    if (!verificar(T_ID)) {
        erro_sintatico(atual()->linha, atual()->coluna,
                       "esperado o nome da variável, mas encontrado '%s'", atual()->lexema);
        return NULL;
    }
    no = criar_no(NO_DECLARACAO, atual()->lexema, linha);
    no->tipo_declarado = tipo;
    avancar();

    if (aceitar(T_ATRIBUICAO))
        adicionar_filho(no, expressao());
    esperar(T_PONTO_VIRGULA, "após a declaração");
    return no;
}

/* comando_if -> 'if' '(' expressao ')' comando [ 'else' comando ] */
static No *comando_if(void)
{
    No *no = criar_no(NO_IF, "", atual()->linha);
    avancar(); /* if */
    esperar(T_ABRE_PAR, "após 'if'");
    adicionar_filho(no, expressao());
    esperar(T_FECHA_PAR, "para fechar a condição do 'if'");
    adicionar_filho(no, comando());
    if (aceitar(T_ELSE))
        adicionar_filho(no, comando());
    return no;
}

/* comando_while -> 'while' '(' expressao ')' comando */
static No *comando_while(void)
{
    No *no = criar_no(NO_WHILE, "", atual()->linha);
    avancar(); /* while */
    esperar(T_ABRE_PAR, "após 'while'");
    adicionar_filho(no, expressao());
    esperar(T_FECHA_PAR, "para fechar a condição do 'while'");
    adicionar_filho(no, comando());
    return no;
}

/* comando_for -> 'for' '(' ( declaracao | expressao ';' ) expressao ';' expressao ')' comando */
static No *comando_for(void)
{
    No *no = criar_no(NO_FOR, "", atual()->linha);
    avancar(); /* for */
    esperar(T_ABRE_PAR, "após 'for'");

    if (eh_tipo(atual()->tipo)) {       /* for (int i = 0; ...) */
        adicionar_filho(no, declaracao());
    } else {                            /* for (i = 0; ...) */
        adicionar_filho(no, expressao());
        esperar(T_PONTO_VIRGULA, "após o início do 'for'");
    }
    adicionar_filho(no, expressao());   /* condição */
    esperar(T_PONTO_VIRGULA, "após a condição do 'for'");
    adicionar_filho(no, expressao());   /* passo */
    esperar(T_FECHA_PAR, "para fechar o 'for'");
    adicionar_filho(no, comando());     /* corpo */
    return no;
}

/* comando_return -> 'return' [ expressao ] ';' */
static No *comando_return(void)
{
    No *no = criar_no(NO_RETURN, "", atual()->linha);
    avancar(); /* return */
    if (!verificar(T_PONTO_VIRGULA))
        adicionar_filho(no, expressao());
    esperar(T_PONTO_VIRGULA, "após o 'return'");
    return no;
}

/* comando -> declaracao | bloco | comando_if | comando_while
 *          | comando_for | comando_return | expressao ';'      */
static No *comando(void)
{
    No *no;
    if (eh_tipo(atual()->tipo)) return declaracao();

    switch (atual()->tipo) {
    case T_ABRE_CHAVE: return bloco();
    case T_IF:         return comando_if();
    case T_WHILE:      return comando_while();
    case T_FOR:        return comando_for();
    case T_RETURN:     return comando_return();
    default:
        /* comando de expressão: x = 1;  printf("oi");  i++; */
        no = criar_no(NO_COMANDO_EXPR, "", atual()->linha);
        adicionar_filho(no, expressao());
        esperar(T_PONTO_VIRGULA, "após o comando");
        return no;
    }
}

/* bloco -> '{' { comando } '}' */
static No *bloco(void)
{
    No *no = criar_no(NO_BLOCO, "", atual()->linha);
    esperar(T_ABRE_CHAVE, "para abrir o bloco");

    while (!verificar(T_FECHA_CHAVE) && !verificar(T_FIM)) {
        int antes = pos;
        No *cmd = comando();
        if (cmd) adicionar_filho(no, cmd);

        if (panico) sincronizar();       /* houve erro: pula até lugar seguro */
        if (pos == antes) avancar();     /* garante que nunca fica parado */
    }
    esperar(T_FECHA_CHAVE, "para fechar o bloco");
    return no;
}

/* ================================================================== */
/* FUNÇÕES E PROGRAMA                                                  */
/* ================================================================== */

/* funcao     -> tipo ID '(' [ parametros ] ')' bloco
 * parametros -> 'void' | tipo ID { ',' tipo ID }                     */
static No *funcao(void)
{
    int linha = atual()->linha;
    Tipo tipo = ler_tipo();
    No *no = criar_no(NO_FUNCAO, atual()->lexema, linha);
    no->tipo_declarado = tipo;
    avancar(); /* nome (já conferido em programa()) */
    avancar(); /* '(' */

    if (verificar(T_VOID) && espiar(1)->tipo == T_FECHA_PAR) {
        avancar(); /* f(void): sem parâmetros */
    } else if (!verificar(T_FECHA_PAR)) {
        do {
            No *param;
            Tipo tipo_param;
            if (!eh_tipo(atual()->tipo)) {
                erro_sintatico(atual()->linha, atual()->coluna,
                               "esperado o tipo do parâmetro, mas encontrado '%s'", atual()->lexema);
                break;
            }
            tipo_param = ler_tipo();
            param = criar_no(NO_PARAMETRO, atual()->lexema, atual()->linha);
            param->tipo_declarado = tipo_param;
            esperar(T_ID, "(nome do parâmetro)");
            adicionar_filho(no, param);
        } while (aceitar(T_VIRGULA));
    }
    esperar(T_FECHA_PAR, "para fechar a lista de parâmetros");

    /* Se o cabeçalho teve erro, pula até o '{' e analisa o corpo mesmo assim. */
    if (panico) {
        while (!verificar(T_ABRE_CHAVE) && !verificar(T_FIM)) avancar();
        panico = 0;
    }
    adicionar_filho(no, bloco()); /* o corpo é sempre o ÚLTIMO filho */
    return no;
}

/* programa -> { funcao | declaracao } FIM
 * Para diferenciar, olhamos 2 tokens à frente:  "int soma (" é função. */
static No *programa(void)
{
    No *raiz = criar_no(NO_PROGRAMA, "", 1);

    while (!verificar(T_FIM)) {
        int antes = pos;
        Token *t = atual();

        if (eh_tipo(t->tipo) && espiar(1)->tipo == T_ID && espiar(2)->tipo == T_ABRE_PAR) {
            adicionar_filho(raiz, funcao());
        } else if (eh_tipo(t->tipo)) {
            No *decl = declaracao();
            if (decl) adicionar_filho(raiz, decl);
        } else {
            erro_sintatico(t->linha, t->coluna,
                           "esperada uma declaração de variável ou função, mas encontrado '%s'", t->lexema);
        }

        if (panico) sincronizar();
        if (pos == antes) avancar();
    }
    return raiz;
}

/* ------------------------------------------------------------------ */
/* Ponto de entrada                                                    */
/* ------------------------------------------------------------------ */

No *analisar_sintatico(const ListaTokens *lista, int *qtd_erros)
{
    No *raiz;
    int i;

    /* Copia só os tokens que interessam ao parser:
     * comentários e diretivas (#include) são ignorados aqui. */
    tokens = malloc(lista->quantidade * sizeof(Token));
    qtd_tokens = 0;
    for (i = 0; i < lista->quantidade; i++) {
        TipoToken t = lista->itens[i].tipo;
        if (t != T_COMENTARIO && t != T_DIRETIVA)
            tokens[qtd_tokens++] = lista->itens[i];
    }
    pos = 0;
    panico = 0;
    erros = 0;

    raiz = programa();

    free(tokens);
    *qtd_erros = erros;
    return raiz;
}
