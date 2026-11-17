/*
 * ast.h - ÁRVORE SINTÁTICA ABSTRATA (AST)
 *
 * O parser transforma a lista de tokens em uma árvore. Cada nó representa
 * uma construção do programa (uma função, um if, uma soma...) e seus filhos
 * são as partes dessa construção.
 *
 * Exemplo:   x = 2 + 3 * y;
 *
 *      Atribuição x
 *      └── Operação +
 *          ├── Inteiro 2
 *          └── Operação *
 *              ├── Inteiro 3
 *              └── Variável y
 *
 * Note que a precedência (* antes de +) já está na forma da árvore.
 */
#ifndef AST_H
#define AST_H

/* Tipos de dados da linguagem. */
typedef enum {
    TIPO_NENHUM,   /* ainda não calculado */
    TIPO_INT,
    TIPO_FLOAT,
    TIPO_CHAR,
    TIPO_VOID,
    TIPO_TEXTO,    /* literal "..." (só pode ser argumento do printf) */
    TIPO_ERRO      /* expressão com erro: evita mensagens repetidas */
} Tipo;

/* Tipos de nó da árvore. */
typedef enum {
    NO_PROGRAMA,     /* raiz: filhos = funções e declarações globais       */
    NO_FUNCAO,       /* texto = nome; filhos = parâmetros..., bloco        */
    NO_PARAMETRO,    /* texto = nome                                       */
    NO_DECLARACAO,   /* texto = nome; filho opcional = valor inicial       */
    NO_BLOCO,        /* filhos = comandos                                  */
    NO_IF,           /* filhos = condição, então, [senão]                  */
    NO_WHILE,        /* filhos = condição, corpo                           */
    NO_FOR,          /* filhos = início, condição, passo, corpo            */
    NO_RETURN,       /* filho opcional = valor                             */
    NO_COMANDO_EXPR, /* filho = expressão (ex.: chamada printf(...);)      */
    NO_ATRIBUICAO,   /* texto = variável; filho = valor                    */
    NO_OPERACAO,     /* texto = operador; filhos = esquerda, direita       */
    NO_OP_UNARIA,    /* texto = operador (- ou !); filho = operando        */
    NO_INCREMENTO,   /* texto = ++ ou --; filho = variável                 */
    NO_CHAMADA,      /* texto = nome da função; filhos = argumentos        */
    NO_VARIAVEL,     /* texto = nome                                       */
    NO_INTEIRO,      /* texto = valor                                      */
    NO_REAL,
    NO_CARACTERE,
    NO_TEXTO
} TipoNo;

typedef struct No {
    TipoNo tipo;
    char texto[256];        /* nome, operador ou valor (depende do tipo do nó) */
    Tipo tipo_declarado;    /* em declarações, parâmetros e funções            */
    Tipo tipo_calculado;    /* preenchido pela análise semântica (expressões)  */
    int linha;
    struct No **filhos;     /* vetor dinâmico de filhos (cresce com realloc)   */
    int qtd_filhos;
    int capacidade;
} No;

No *criar_no(TipoNo tipo, const char *texto, int linha);
void adicionar_filho(No *pai, No *filho);
void imprimir_arvore(const No *no, int nivel);
void liberar_arvore(No *no);
const char *nome_do_tipo(Tipo tipo);

#endif
