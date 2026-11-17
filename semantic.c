/*
 * semantic.c - ANÁLISE SEMÂNTICA (3ª fase)
 *
 * Percorre a árvore recursivamente (como o imprimir_arvore) e, em cada nó,
 * faz as verificações:
 *
 *   1. Declaração antes do uso      -> buscar_simbolo() não encontra o nome
 *   2. Compatibilidade de tipos     -> analisar_expressao() calcula o tipo de
 *                                      cada expressão e compara
 *   3. Redeclaração no mesmo escopo -> buscar_no_escopo_atual() já encontra
 *   4. Escopos (blocos aninhados)   -> entrar_escopo() / sair_escopo()
 *   5. Chamadas de função           -> quantidade e tipo dos argumentos
 *
 * Regras de tipo adotadas (simplificadas):
 *   - int, float e char são NUMÉRICOS e podem ser misturados em contas;
 *     se algum operando for float, o resultado é float; senão é int.
 *   - guardar um float em um int é permitido, mas gera AVISO (perde a parte decimal).
 *   - texto ("...") só pode ser passado como argumento (ex.: printf).
 *   - uma função void não produz valor.
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "semantic.h"
#include "tabela_simbolos.h"
#include "erros.h"

static int erros;
static Tipo retorno_esperado;     /* tipo de retorno da função sendo analisada */
static char funcao_atual[64];
static int achou_return;

static void erro_semantico(int linha, const char *formato, ...)
{
    char mensagem[256];
    va_list args;
    va_start(args, formato);
    vsnprintf(mensagem, sizeof mensagem, formato, args);
    va_end(args);
    registrar_erro("Semântico", linha, 0, "%s", mensagem);
    erros++;
}

static int eh_numerico(Tipo t)
{
    return t == TIPO_INT || t == TIPO_FLOAT || t == TIPO_CHAR;
}

/* Verifica se um valor do tipo 'origem' pode ser guardado em 'destino'.
 * 'contexto' descreve onde isso acontece, para a mensagem de erro. */
static void verificar_atribuicao(Tipo destino, Tipo origem, int linha, const char *contexto)
{
    if (destino == TIPO_ERRO || origem == TIPO_ERRO) return; /* erro já foi reportado */

    if (origem == TIPO_VOID) {
        erro_semantico(linha, "%s: a função chamada é void e não devolve valor", contexto);
    } else if (destino == origem) {
        return; /* mesmo tipo: tudo certo */
    } else if (eh_numerico(destino) && eh_numerico(origem)) {
        if (origem == TIPO_FLOAT)
            registrar_aviso("Semântico", linha, 0,
                            "%s: valor float guardado em %s perde a parte decimal",
                            contexto, nome_do_tipo(destino));
    } else {
        erro_semantico(linha, "%s: tipo incompatível (esperado %s, recebido %s)",
                       contexto, nome_do_tipo(destino), nome_do_tipo(origem));
    }
}

/* ================================================================== */
/* EXPRESSÕES: calcula e devolve o tipo de cada expressão              */
/* ================================================================== */

static Tipo analisar_expressao(No *no);

/* Chamada de função: f(a, b) */
static Tipo analisar_chamada(No *no)
{
    Simbolo *f = buscar_simbolo(no->texto);
    Tipo tipos_args[64];
    int i, qtd_args = no->qtd_filhos;
    char contexto[128];

    /* primeiro calcula o tipo de cada argumento */
    for (i = 0; i < qtd_args; i++) {
        Tipo t = analisar_expressao(no->filhos[i]);
        if (i < 64) tipos_args[i] = t;
    }

    if (!f) {
        erro_semantico(no->linha, "função '%s' não foi declarada", no->texto);
        return TIPO_ERRO;
    }
    if (f->categoria != FUNCAO) {
        erro_semantico(no->linha, "'%s' é uma %s, não uma função", no->texto,
                       f->categoria == VARIAVEL ? "variável" : "parâmetro");
        return TIPO_ERRO;
    }

    /* ARIDADE: quantidade de argumentos */
    if (qtd_args < f->qtd_parametros ||
        (qtd_args > f->qtd_parametros && !f->aceita_mais_argumentos)) {
        erro_semantico(no->linha, "função '%s' espera %d argumento(s), mas recebeu %d",
                       no->texto, f->qtd_parametros, qtd_args);
    }

    /* TIPOS: cada argumento comparado com o parâmetro correspondente */
    for (i = 0; i < qtd_args && i < f->qtd_parametros && i < 64; i++) {
        snprintf(contexto, sizeof contexto, "argumento %d de '%.60s'", i + 1, no->texto);
        verificar_atribuicao(f->parametros[i], tipos_args[i], no->linha, contexto);
    }
    return f->tipo; /* o tipo da chamada é o tipo de retorno da função */
}

/* Operação binária: a + b, a < b, a && b ... */
static Tipo analisar_operacao(No *no)
{
    Tipo esq = analisar_expressao(no->filhos[0]);
    Tipo dir = analisar_expressao(no->filhos[1]);
    const char *op = no->texto;

    if (esq == TIPO_ERRO || dir == TIPO_ERRO) return TIPO_ERRO;

    if (!eh_numerico(esq) || !eh_numerico(dir)) {
        erro_semantico(no->linha, "operador '%s' não pode ser usado com %s e %s",
                       op, nome_do_tipo(esq), nome_do_tipo(dir));
        return TIPO_ERRO;
    }
    if (strcmp(op, "%") == 0 && (esq == TIPO_FLOAT || dir == TIPO_FLOAT)) {
        erro_semantico(no->linha, "operador '%%' só aceita inteiros (recebeu %s e %s)",
                       nome_do_tipo(esq), nome_do_tipo(dir));
        return TIPO_ERRO;
    }

    /* comparações e lógicos (<, ==, &&...) resultam em int (0 ou 1) */
    if (strcmp(op, "+") != 0 && strcmp(op, "-") != 0 && strcmp(op, "*") != 0 &&
        strcmp(op, "/") != 0 && strcmp(op, "%") != 0)
        return TIPO_INT;

    /* contas: float se algum lado for float, senão int */
    if (esq == TIPO_FLOAT || dir == TIPO_FLOAT) return TIPO_FLOAT;
    return TIPO_INT;
}

static Tipo analisar_expressao(No *no)
{
    Tipo t = TIPO_ERRO;
    Simbolo *s;

    switch (no->tipo) {
    case NO_INTEIRO:   t = TIPO_INT; break;
    case NO_REAL:      t = TIPO_FLOAT; break;
    case NO_CARACTERE: t = TIPO_CHAR; break;
    case NO_TEXTO:     t = TIPO_TEXTO; break;

    case NO_VARIAVEL:
        s = buscar_simbolo(no->texto);
        if (!s)
            erro_semantico(no->linha, "variável '%s' não foi declarada", no->texto);
        else if (s->categoria == FUNCAO)
            erro_semantico(no->linha, "'%s' é uma função; para chamá-la use %s(...)", no->texto, no->texto);
        else
            t = s->tipo;
        break;

    case NO_ATRIBUICAO: {
        char contexto[128];
        Tipo valor = analisar_expressao(no->filhos[0]);
        s = buscar_simbolo(no->texto);
        snprintf(contexto, sizeof contexto, "atribuição a '%.60s'", no->texto);
        if (!s)
            erro_semantico(no->linha, "variável '%s' não foi declarada", no->texto);
        else if (s->categoria == FUNCAO)
            erro_semantico(no->linha, "não é possível atribuir valor à função '%s'", no->texto);
        else {
            verificar_atribuicao(s->tipo, valor, no->linha, contexto);
            t = s->tipo;
        }
        break;
    }

    case NO_OPERACAO:
        t = analisar_operacao(no);
        break;

    case NO_OP_UNARIA:  /* -x  ou  !x */
        t = analisar_expressao(no->filhos[0]);
        if (t != TIPO_ERRO && !eh_numerico(t)) {
            erro_semantico(no->linha, "operador '%s' não pode ser usado com %s", no->texto, nome_do_tipo(t));
            t = TIPO_ERRO;
        } else if (t == TIPO_CHAR || strcmp(no->texto, "!") == 0) {
            t = TIPO_INT;
        }
        break;

    case NO_INCREMENTO: /* x++  ou  x-- */
        t = analisar_expressao(no->filhos[0]);
        if (t != TIPO_ERRO && !eh_numerico(t)) {
            erro_semantico(no->linha, "'%s' não pode ser usado com %s", no->texto, nome_do_tipo(t));
            t = TIPO_ERRO;
        }
        break;

    case NO_CHAMADA:
        t = analisar_chamada(no);
        break;

    default:
        break;
    }

    no->tipo_calculado = t;   /* guarda o tipo na árvore (aparece na impressão) */
    return t;
}

/* ================================================================== */
/* COMANDOS                                                            */
/* ================================================================== */

static void analisar_comando(No *no);

static void analisar_declaracao(No *no)
{
    Simbolo *anterior;

    if (no->tipo_declarado == TIPO_VOID)
        erro_semantico(no->linha, "variável '%s' não pode ser do tipo void", no->texto);

    if (no->qtd_filhos > 0) {   /* tem valor inicial: int x = ...; */
        char contexto[128];
        Tipo valor = analisar_expressao(no->filhos[0]);
        snprintf(contexto, sizeof contexto, "inicialização de '%.60s'", no->texto);
        verificar_atribuicao(no->tipo_declarado, valor, no->linha, contexto);
    }

    /* REDECLARAÇÃO: o nome já existe NESTE escopo? */
    anterior = buscar_no_escopo_atual(no->texto);
    if (anterior) {
        erro_semantico(no->linha, "'%s' já foi declarado neste escopo (na linha %d)",
                       no->texto, anterior->linha);
        return;
    }
    inserir_simbolo(no->texto, VARIAVEL, no->tipo_declarado, no->linha);
}

/* A condição de if/while/for precisa ser numérica (0 = falso). */
static void analisar_condicao(No *cond, const char *comando)
{
    Tipo t = analisar_expressao(cond);
    if (t != TIPO_ERRO && !eh_numerico(t))
        erro_semantico(cond->linha, "a condição do '%s' deve ser numérica, mas é %s",
                       comando, nome_do_tipo(t));
}

/* Nome legível para o escopo de um bloco, ex.: "main/bloco L12" */
static void entrar_escopo_bloco(const char *tipo, int linha)
{
    char nome[64];
    snprintf(nome, sizeof nome, "%.30s/%s L%d", funcao_atual, tipo, linha);
    entrar_escopo(nome);
}

static void analisar_comando(No *no)
{
    int i;
    if (!no) return;

    switch (no->tipo) {
    case NO_DECLARACAO:
        analisar_declaracao(no);
        break;

    case NO_BLOCO:   /* { ... } cria um escopo novo */
        entrar_escopo_bloco("bloco", no->linha);
        for (i = 0; i < no->qtd_filhos; i++)
            analisar_comando(no->filhos[i]);
        sair_escopo();
        break;

    case NO_IF:
        analisar_condicao(no->filhos[0], "if");
        analisar_comando(no->filhos[1]);                       /* então */
        if (no->qtd_filhos > 2) analisar_comando(no->filhos[2]); /* senão */
        break;

    case NO_WHILE:
        analisar_condicao(no->filhos[0], "while");
        analisar_comando(no->filhos[1]);
        break;

    case NO_FOR:     /* a variável declarada no for só existe dentro dele */
        entrar_escopo_bloco("for", no->linha);
        if (no->filhos[0]->tipo == NO_DECLARACAO)
            analisar_declaracao(no->filhos[0]);
        else
            analisar_expressao(no->filhos[0]);
        analisar_condicao(no->filhos[1], "for");
        analisar_expressao(no->filhos[2]);
        analisar_comando(no->filhos[3]);
        sair_escopo();
        break;

    case NO_RETURN:
        achou_return = 1;
        if (no->qtd_filhos > 0) {
            Tipo t = analisar_expressao(no->filhos[0]);
            if (retorno_esperado == TIPO_VOID) {
                erro_semantico(no->linha, "a função '%s' é void e não pode retornar valor", funcao_atual);
            } else {
                char contexto[128];
                snprintf(contexto, sizeof contexto, "retorno da função '%s'", funcao_atual);
                verificar_atribuicao(retorno_esperado, t, no->linha, contexto);
            }
        } else if (retorno_esperado != TIPO_VOID) {
            erro_semantico(no->linha, "a função '%s' deve retornar um valor do tipo %s",
                           funcao_atual, nome_do_tipo(retorno_esperado));
        }
        break;

    case NO_COMANDO_EXPR:
        analisar_expressao(no->filhos[0]);
        break;

    default:
        break;
    }
}

/* ================================================================== */
/* FUNÇÕES                                                             */
/* ================================================================== */

static void analisar_funcao(No *no)
{
    Simbolo *f, *anterior;
    No *corpo = no->filhos[no->qtd_filhos - 1];  /* o corpo é o último filho */
    int qtd_params = no->qtd_filhos - 1;         /* os outros são parâmetros */
    int i;

    /* registra a função no escopo global (antes do corpo: permite recursão) */
    anterior = buscar_no_escopo_atual(no->texto);
    if (anterior) {
        erro_semantico(no->linha, "'%s' já foi declarado (na linha %d)", no->texto, anterior->linha);
    } else {
        f = inserir_simbolo(no->texto, FUNCAO, no->tipo_declarado, no->linha);
        for (i = 0; i < qtd_params && i < MAX_PARAMETROS; i++)
            f->parametros[i] = no->filhos[i]->tipo_declarado;
        f->qtd_parametros = qtd_params;
    }

    snprintf(funcao_atual, sizeof funcao_atual, "%.63s", no->texto);
    retorno_esperado = no->tipo_declarado;
    achou_return = 0;

    /* escopo da função: parâmetros + variáveis do corpo */
    entrar_escopo(no->texto);
    for (i = 0; i < qtd_params; i++) {
        No *p = no->filhos[i];
        if (p->tipo_declarado == TIPO_VOID)
            erro_semantico(p->linha, "parâmetro '%s' não pode ser do tipo void", p->texto);
        if (buscar_no_escopo_atual(p->texto))
            erro_semantico(p->linha, "parâmetro '%s' repetido", p->texto);
        else
            inserir_simbolo(p->texto, PARAMETRO, p->tipo_declarado, p->linha);
    }
    /* Os comandos do corpo ficam no MESMO escopo dos parâmetros (regra do C),
     * por isso analisamos os filhos do bloco direto, sem criar outro escopo. */
    for (i = 0; i < corpo->qtd_filhos; i++)
        analisar_comando(corpo->filhos[i]);
    sair_escopo();

    if (retorno_esperado != TIPO_VOID && !achou_return && strcmp(no->texto, "main") != 0)
        registrar_aviso("Semântico", no->linha, 0, "a função '%s' não tem comando return", no->texto);
}

/* ------------------------------------------------------------------ */
/* Ponto de entrada                                                    */
/* ------------------------------------------------------------------ */

int analisar_semantico(No *programa)
{
    Simbolo *printf_sim;
    int i;

    erros = 0;
    snprintf(funcao_atual, sizeof funcao_atual, "global");
    entrar_escopo("global");

    /* printf não é declarado no arquivo (vem do #include <stdio.h>),
     * então o registramos aqui: printf(texto, ...) devolve int. */
    printf_sim = inserir_simbolo("printf", FUNCAO, TIPO_INT, 0);
    printf_sim->parametros[0] = TIPO_TEXTO;
    printf_sim->qtd_parametros = 1;
    printf_sim->aceita_mais_argumentos = 1;
    printf_sim->predefinida = 1;

    for (i = 0; i < programa->qtd_filhos; i++) {
        No *item = programa->filhos[i];
        if (item->tipo == NO_FUNCAO)
            analisar_funcao(item);
        else
            analisar_declaracao(item);   /* variável global */
    }

    sair_escopo();
    return erros;
}
