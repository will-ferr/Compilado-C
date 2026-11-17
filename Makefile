# Makefile - versão básica
#
#   make          compila (gera o executável "compilador")
#   make testes   roda os 4 arquivos de teste
#   make limpar   apaga os arquivos gerados
#
# No Windows use "mingw32-make" no lugar de "make".

CC     = gcc
CFLAGS = -std=c99 -Wall -Wextra

# cada .c vira um .o, e os .o são juntados no executável
OBJETOS = main.o lexer.o parser.o ast.o semantic.o tabela_simbolos.o erros.o

ifeq ($(OS),Windows_NT)
  EXECUTAVEL = compilador.exe
  APAGAR     = del /Q *.o compilador.exe 2>nul
  RODAR      = .\compilador.exe
  # se estiver usando o Git Bash (sh), os comandos são os do Linux
  ifneq ($(shell echo "x"),"x")
    APAGAR = rm -f *.o compilador.exe
    RODAR  = ./compilador.exe
  endif
else
  EXECUTAVEL = compilador
  APAGAR     = rm -f *.o compilador
  RODAR      = ./compilador
endif

$(EXECUTAVEL): $(OBJETOS)
	$(CC) $(CFLAGS) -o $(EXECUTAVEL) $(OBJETOS)

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $<

# o "-" na frente ignora o código de saída 1 (esperado nos testes com erro)
testes: $(EXECUTAVEL)
	-$(RODAR) testes/1_valido.c
	-$(RODAR) testes/2_erro_lexico.c
	-$(RODAR) testes/3_erro_sintatico.c
	-$(RODAR) testes/4_erro_semantico.c

limpar:
	-$(APAGAR)

.PHONY: testes limpar
