/* Teste 4 - ERROS SEMÂNTICOS: a escrita está correta, mas o significado não. */
int soma(int a, int b)
{
    return a + b;
}

void mostrar(int n)
{
    printf("%d\n", n);
    return n;                // erro: função void retornando valor
}

int main()
{
    int x = 10;
    int x = 20;              // erro: 'x' declarado duas vezes no mesmo escopo
    float f = 2.5;

    y = 5;                   // erro: 'y' não foi declarado
    x = "texto";             // erro: texto guardado em int
    x = soma(1);             // erro: soma espera 2 argumentos
    x = soma(1, "dois");     // erro: argumento 2 deveria ser int
    x = calcular(3);         // erro: função não declarada
    x = f % 2;               // erro: '%' só aceita inteiros
    x = mostrar(1);          // erro: mostrar é void e não devolve valor
    x = f;                   // aviso: float em int perde a parte decimal

    {
        int interna = 1;
        x = interna;
    }
    x = interna;             // erro: 'interna' só existia dentro do bloco

    return;                  // erro: main deve retornar um int
}
