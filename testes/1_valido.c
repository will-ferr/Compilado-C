/* Teste 1 - programa VÁLIDO: deve passar pelas 3 fases sem erros. */
#include <stdio.h>

int total = 0;

int soma(int a, int b)
{
    return a + b;
}

float media(int a, int b)
{
    return (a + b) / 2.0;
}

int fatorial(int n)
{
    if (n <= 1) {
        return 1;
    }
    return n * fatorial(n - 1);   // recursão
}

int main()
{
    int x = 10;
    int y = 3;
    float m;
    char letra = 'A';

    m = media(x, y);
    printf("Soma: %d\n", soma(x, y));
    printf("Media: %f\n", m);
    printf("Fatorial de 5: %d\n", fatorial(5));

    for (int i = 0; i < 3; i++) {
        total = total + i;
    }

    while (x > 0 && y != 0) {
        x = x - 2;
    }

    if (x % 2 == 0) {
        printf("%c par\n", letra);
    } else {
        printf("%c impar\n", letra);
    }
    return 0;
}
