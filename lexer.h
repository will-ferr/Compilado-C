/*
 * lexer.h - ANÁLISE LÉXICA (1ª fase)
 *
 * O analisador léxico (scanner) lê o código-fonte caractere por caractere
 * e agrupa os caracteres em TOKENS (as "palavras" da linguagem).
 *
 * Exemplo:   int x = 10;
 * vira:      [int] [x] [=] [10] [;]
 *            palavra-chave, identificador, operador, número, delimitador
 */
#ifndef LEXER_H
#define LEXER_H

/* Todos os tipos de token que a linguagem possui. */
typedef enum {
    T_FIM,          /* fim do arquivo */
    T_ERRO,         /* token inválido (erro léxico) */

    /* identificadores e literais */
    T_ID,           /* nomes: x, soma, contador */
    T_NUM_INT,      /* 10, 0, 42 */
    T_NUM_REAL,     /* 3.14, 0.5 */
    T_CARACTERE,    /* 'a', '\n' */
    T_TEXTO,        /* "olá mundo" */

    /* palavras-chave (reservadas) */
    T_INT, T_FLOAT, T_CHAR, T_VOID,
    T_IF, T_ELSE, T_WHILE, T_FOR, T_RETURN,

    /* operadores */
    T_MAIS, T_MENOS, T_VEZES, T_DIVIDIDO, T_RESTO,  /* + - * / %  */
    T_INCREMENTO, T_DECREMENTO,                     /* ++ --      */
    T_ATRIBUICAO,                                   /* =          */
    T_IGUAL, T_DIFERENTE,                           /* == !=      */
    T_MENOR, T_MAIOR, T_MENOR_IGUAL, T_MAIOR_IGUAL, /* < > <= >=  */
    T_E, T_OU, T_NAO,                               /* && || !    */

    /* delimitadores */
    T_ABRE_PAR, T_FECHA_PAR,       /* ( ) */
    T_ABRE_CHAVE, T_FECHA_CHAVE,   /* { } */
    T_PONTO_VIRGULA, T_VIRGULA,    /* ; , */

    /* reconhecidos, mas ignorados pelo parser */
    T_COMENTARIO,   /* // ... ou  / * ... * / */
    T_DIRETIVA      /* #include ... */
} TipoToken;

/* Um token: o tipo, o texto original (lexema) e onde ele aparece. */
typedef struct {
    TipoToken tipo;
    char lexema[256];
    int linha;
    int coluna;
} Token;

/* Lista (vetor dinâmico) com todos os tokens do arquivo. */
typedef struct {
    Token *itens;
    int quantidade;
    int capacidade;
} ListaTokens;

/* Executa a análise léxica do código inteiro. */
ListaTokens analisar_lexico(const char *codigo);

/* Texto de um tipo de token, usado nas mensagens: "int", "+", ";"... */
const char *texto_do_token(TipoToken tipo);

void imprimir_tokens(const ListaTokens *lista);
void imprimir_tabela_inicial(void);   /* identificadores encontrados */
void liberar_tokens(ListaTokens *lista);

#endif
