# Analisador de C — versão básica

Trabalho de **Compiladores** — Ciência da Computação, FUMEC.
Professora: Amanda Danielle Lima de Oliveira.

**Integrantes:** Bernardo Simões Melo, Bruno Soares Jordão Lopes,
Franco Couto Alves Pinto, William Ferreira de Brito.

Este programa, escrito em C, lê um arquivo de código C e executa as **três
primeiras fases de um compilador**: análise léxica, sintática e semântica.
Ele não gera código executável: verifica se o programa está correto e
mostra o que encontrou.

---

## 1. Visão geral: o caminho do código

```
 arquivo.c ──► [ LÉXICO ] ──► tokens ──► [ SINTÁTICO ] ──► árvore ──► [ SEMÂNTICO ] ──► ok / erros
               lexer.c                   parser.c          (AST)       semantic.c
```

Cada fase só roda se a anterior não teve erros.

### Exemplo acompanhado passo a passo

Vamos seguir esta linha pelas três fases:

```c
int total = preco * 2;
```

**Fase 1 — Léxica** (`lexer.c`): separa o texto em *tokens*, as "palavras" da linguagem.

| Lexema | Categoria |
|--------|-----------|
| `int` | PALAVRA_CHAVE |
| `total` | IDENTIFICADOR |
| `=` | OPERADOR |
| `preco` | IDENTIFICADOR |
| `*` | OPERADOR |
| `2` | NUMERO |
| `;` | DELIMITADOR |

O léxico só olha caracteres. Ele não sabe se `preco` existe, apenas que é um
nome válido. Um caractere como `@` seria **erro léxico**.

**Fase 2 — Sintática** (`parser.c`): confere se os tokens estão numa ordem
permitida pela gramática e monta a **árvore sintática (AST)**:

```
Declaração int total
|-- Operação *
|   |-- Variável preco
|   |-- Inteiro 2
```

Se faltasse o `;` (`int total = preco * 2`), seria **erro sintático**: as
palavras existem, mas a frase está mal formada.

**Fase 3 — Semântica** (`semantic.c`): percorre a árvore e verifica se o
programa faz sentido, usando a **tabela de símbolos**:

- `preco` foi declarado antes? Se não foi, **erro semântico**.
- `preco * 2` dá um número e pode ir para um `int`? Se `preco` fosse um texto, seria erro de tipo.
- `total` já existe neste mesmo escopo? Se sim, é redeclaração.

---

## 2. Como compilar e executar

### Windows (PowerShell)

```powershell
# 1. entrar na pasta do projeto
cd "C:\Users\willi\OneDrive\Documentos\FUMEC\Compiladores\AAI_Basico"

# 2. colocar o gcc no PATH (o MinGW vem junto com o CLion)
$env:Path = "D:\CLion 2026.2.1\bin\mingw\bin;" + $env:Path

# 3. compilar
mingw32-make

# 4. executar sobre um arquivo
.\compilador.exe testes\1_valido.c
```

Sem `make`, também funciona: `gcc -std=c99 -Wall -o compilador *.c`

### Linux / macOS

```sh
make
./compilador testes/1_valido.c
```

### Outros comandos

| Comando | O que faz |
|---------|-----------|
| `make testes` (`mingw32-make testes`) | roda os 4 testes |
| `make limpar` (`mingw32-make limpar`) | apaga os arquivos gerados |

---

## 3. O que o programa imprime

1. **Lista de tokens:** posição (linha:coluna), categoria e texto de cada token.
2. **Tabela de símbolos inicial:** cada identificador encontrado pelo léxico.
3. **Resultado sintático:** se o programa segue ou não a gramática.
4. **Árvore sintática (AST):** com o tipo calculado de cada expressão.
5. **Tabela de símbolos final:** nome, categoria, tipo, escopo e linha.
6. **Tabela de erros:** fase, posição e mensagem de cada erro ou aviso.

---

## 4. Organização dos arquivos

| Arquivo | Função |
|---------|--------|
| `main.c` | lê o arquivo e chama as três fases em ordem |
| `lexer.c` / `.h` | **análise léxica**: transforma texto em tokens |
| `parser.c` / `.h` | **análise sintática**: transforma tokens em árvore |
| `ast.c` / `.h` | estrutura da árvore (criar, imprimir e liberar nós) |
| `semantic.c` / `.h` | **análise semântica**: percorre a árvore verificando o significado |
| `tabela_simbolos.c` / `.h` | tabela hash + pilha de escopos |
| `erros.c` / `.h` | guarda e imprime a tabela de erros |
| `testes/` | arquivos de teste |

### Estruturas de dados usadas (todas feitas à mão)

- **Vetor dinâmico** (`realloc`): lista de tokens e filhos de cada nó da árvore.
- **Árvore**: a AST. Cada nó tem uma lista de filhos.
- **Tabela hash** com encadeamento: cada escopo da tabela de símbolos.
  A função `hash()` transforma o nome em um número, que indica a "gaveta"
  onde o símbolo fica.
- **Pilha** (lista encadeada): os escopos. Entrar em `{` empilha e sair de `}` desempilha.

---

## 5. A linguagem aceita (subconjunto de C)

- Tipos: `int`, `float`, `char`, `void`.
- Variáveis globais e locais, com uma variável por declaração (`int x = 5;`).
- Funções com parâmetros e recursão.
- Comandos: `if`/`else`, `while`, `for`, `return`, blocos `{ }`.
- Expressões: `+ - * / %`, `< > <= >= == !=`, `&& || !`, `x++`, `x--`, `=`.
- Literais: números inteiros e reais, caracteres `'a'` e textos `"..."`.
- Comentários `//` e `/* */`. Diretivas `#include` são reconhecidas e ignoradas.
- `printf` já vem declarada, como se o `#include <stdio.h>` funcionasse.

### Gramática (EBNF)

`{ X }` significa repetir zero ou mais vezes; `[ X ]` significa opcional.

```ebnf
programa       ::= { funcao | declaracao } FIM

funcao         ::= tipo ID "(" [ parametros ] ")" bloco
parametros     ::= "void" | tipo ID { "," tipo ID }
tipo           ::= "int" | "float" | "char" | "void"

declaracao     ::= tipo ID [ "=" expressao ] ";"
bloco          ::= "{" { comando } "}"

comando        ::= declaracao | bloco | comando_if | comando_while
                 | comando_for | comando_return | expressao ";"
comando_if     ::= "if" "(" expressao ")" comando [ "else" comando ]
comando_while  ::= "while" "(" expressao ")" comando
comando_for    ::= "for" "(" ( declaracao | expressao ";" ) expressao ";" expressao ")" comando
comando_return ::= "return" [ expressao ] ";"

expressao      ::= ID "=" expressao | ou_logico
ou_logico      ::= e_logico   { "||" e_logico }
e_logico       ::= igualdade  { "&&" igualdade }
igualdade      ::= relacional { ( "==" | "!=" ) relacional }
relacional     ::= soma       { ( "<" | ">" | "<=" | ">=" ) soma }
soma           ::= termo      { ( "+" | "-" ) termo }
termo          ::= unaria     { ( "*" | "/" | "%" ) unaria }
unaria         ::= ( "-" | "!" ) unaria | posfixa
posfixa        ::= primaria [ "++" | "--" ]
primaria       ::= NUM_INT | NUM_REAL | CARACTERE | TEXTO
                 | ID | ID "(" [ expressao { "," expressao } ] ")"
                 | "(" expressao ")"
```

**Como a precedência funciona:** cada nível da gramática chama o nível de
baixo. Em `2 + 3 * 4`, a regra `soma` chama `termo`, e `termo` consome
`3 * 4` inteiro antes de a soma continuar. Por isso a multiplicação fica mais
funda na árvore e é calculada primeiro.

### Por que análise descendente (LL)?

- Cada regra da gramática vira **uma função em C** com o mesmo nome
  (`comando_if()`, `soma()`, `termo()`...), o que torna o código fácil de
  ler e de comparar com a gramática.
- A gramática não tem recursão à esquerda (usa `{ }` no lugar), então basta
  olhar o token atual para decidir qual regra usar. Só há dois pontos em que
  olhamos à frente:
  - `int soma (` → é uma função; `int soma =` → é uma variável.
  - `x =` → é uma atribuição; `x +` → é uma expressão.
- Não precisa de ferramenta geradora: é escrito à mão, como pede o trabalho.

### Recuperação de erros sintáticos (modo pânico)

Ao achar um erro, o parser registra a mensagem e passa a ignorar novos erros.
Depois descarta tokens até um ponto seguro (`;`, `{`, `}` ou o início de um
comando) e volta ao normal. Assim ele encontra vários erros numa execução
só, sem gerar uma avalanche de mensagens repetidas.

---

## 6. Verificações semânticas

| Verificação | Exemplo de erro |
|-------------|-----------------|
| Declaração antes do uso | `y = 5;` sem declarar `y` |
| Compatibilidade de tipos | `int x = "texto";` |
| Redeclaração no mesmo escopo | `int x; int x;` |
| Escopos aninhados | usar fora do bloco uma variável declarada dentro dele |
| Quantidade de argumentos | `soma(1)` quando `soma` tem 2 parâmetros |
| Tipo dos argumentos | `soma(1, "dois")` |
| Retorno da função | `return 5;` em função `void`, ou `return;` em função `int` |
| Uso de `%` | `2.5 % 2` (só aceita inteiros) |

Avisos (não impedem a compilação): guardar `float` em `int` e função não-void sem `return`.

---

## 7. Testes

| Arquivo | Resultado esperado |
|---------|--------------------|
| `testes/1_valido.c` | passa nas 3 fases |
| `testes/2_erro_lexico.c` | 6 erros léxicos |
| `testes/3_erro_sintatico.c` | 5 erros sintáticos |
| `testes/4_erro_semantico.c` | 11 erros semânticos e 1 aviso |

## 8. Limitações

Não há suporte a vetores, ponteiros, `struct`, `switch`, `do-while`,
`break`/`continue`, várias variáveis na mesma declaração (`int a, b;`) nem
protótipos de função. Uma função precisa ser definida antes de ser chamada.
