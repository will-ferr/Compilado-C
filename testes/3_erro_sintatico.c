/* Teste 3 - ERROS SINTÁTICOS: os tokens são válidos, mas a ordem não segue a gramática. */
int soma(int a, int b)
{
    return a + b             // erro: falta ';'
}

int main()
{
    int x = 5;
    int y = ;                // erro: falta o valor depois do '='

    if (x > 3 {              // erro: falta ')'
        x = x + 1;
    }

    while x < 10) {          // erro: falta '(' depois do while
        x++;
    }

    x = soma(1, 2;           // erro: falta ')' na chamada
    return 0;
}
