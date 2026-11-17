/*
 * erros.h - Tabela de erros.
 *
 * As três fases (léxica, sintática e semântica) guardam seus erros aqui.
 * No final, o main imprime todos eles em forma de tabela.
 */
#ifndef ERROS_H
#define ERROS_H

/* Registra um erro. fase = "Léxico", "Sintático" ou "Semântico".
 * O formato funciona igual ao printf. Use coluna = 0 se não souber. */
void registrar_erro(const char *fase, int linha, int coluna, const char *formato, ...);

/* Registra um aviso (algo suspeito, mas que não impede a compilação). */
void registrar_aviso(const char *fase, int linha, int coluna, const char *formato, ...);

/* Conta quantos erros (sem contar avisos) a fase teve. */
int contar_erros(const char *fase);
int contar_avisos(void);

void imprimir_tabela_erros(void);

#endif
