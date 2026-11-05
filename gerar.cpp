/*---------------------------------------------------
 gerar.cpp - Gera o arquivo dados.txt com a estatura (cm) e o peso (kg)
             de 1000 pessoas, uma pessoa por linha, separados por espaço.

 Estatura ~ Normal(média 170, desvio 10)
 Peso     ~ Normal(média 70,  desvio 12)
 A semente fixa (42) garante que o arquivo é sempre o mesmo.

 Compilar e rodar:
   g++ gerar.cpp -o gerar
   ./gerar
-----------------------------------------------------*/

#include <fstream>
#include <iomanip>
#include <random>

int main() {
    const int N = 1000;

    std::mt19937 gerador(42);
    std::normal_distribution<double> estatura(170.0, 10.0);
    std::normal_distribution<double> peso(70.0, 12.0);

    std::ofstream saida("dados.txt");
    saida << std::fixed << std::setprecision(2 - 1);

    for (int i = 0; i < N; ++i) {
        saida << estatura(gerador) << " " << peso(gerador) << "\n";
    }

    return 0;
}