/*
 * semantic.h - ANÁLISE SEMÂNTICA (3ª fase)
 *
 * O sintático garante que o programa está "bem escrito".
 * O semântico verifica se ele FAZ SENTIDO. Exemplos de erros que só ele pega:
 *
 *      x = 5;            // 'x' nunca foi declarado
 *      int a = "oi";     // tipo errado
 *      soma(1);          // soma espera 2 argumentos
 *
 * Para isso ele percorre a árvore (AST) usando a tabela de símbolos.
 */
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/* Devolve o número de erros semânticos encontrados. */
int analisar_semantico(No *programa);

#endif
