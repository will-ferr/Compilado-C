/*
 * parser.h - ANÁLISE SINTÁTICA (2ª fase)
 *
 * O analisador sintático (parser) verifica se a SEQUÊNCIA de tokens segue
 * a gramática da linguagem e monta a árvore sintática (AST).
 *
 * O léxico diz "isto é um if, isto é um parêntese...";
 * o sintático diz "um if precisa de '(' condição ')' e um comando".
 */
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

/* Recebe os tokens e devolve a raiz da árvore.
 * Em *qtd_erros devolve quantos erros sintáticos foram encontrados. */
No *analisar_sintatico(const ListaTokens *tokens, int *qtd_erros);

#endif
