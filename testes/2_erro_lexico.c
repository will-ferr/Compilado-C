/* Teste 2 - ERROS LÉXICOS: caracteres e literais que não formam tokens válidos. */
int main()
{
    int preco@ = 5;          // erro: '@' não pertence à linguagem
    int valor = 12abc;       // erro: número malformado
    float taxa = 1.2.3;      // erro: número malformado
    char letra = 'abc';      // erro: caractere com mais de um símbolo
    int $total = 0;          // erro: '$' não pertence à linguagem
    printf("texto sem fim);  // erro: texto não foi fechado
    return 0;
}
