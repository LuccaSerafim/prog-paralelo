/*---------------------------------------------------
 Exercício Final - Coeficiente de Variação com OpenMP
-----------------------------------------------------
 Entrada: arquivo de texto com a estatura (cm) e o peso (kg) de 1000 pessoas,
          uma pessoa por linha, separados por espaço.

 Classes: estatura -> intervalo de 8 cm
          peso     -> intervalo de 4 kg

 Fórmulas (dados agrupados):
     xᵢ = ponto médio da classe i       fᵢ = frequência da classe i
     k  = número de classes             N  = número total de pessoas

 Média ponderada:
            k
       μ = ( Σ xᵢ · fᵢ ) / N
           i=1

 Desvio-padrão populacional:
                k
       σ = √( ( Σ fᵢ · (xᵢ - μ)² ) / N )
               i=1

 Coeficiente de variação:
       CV = (σ / μ) × 100%

 Assim como no 007_reduction_0.1, usamos o método das DUAS PASSADAS:
   Passada 1: reduction(+:soma)           -> μ
   Passada 2: reduction(+:soma_desvios2)  -> σ
 A segunda soma DEPENDE de μ, que só é conhecido depois da primeira.

 Três versões do cálculo são comparadas:
   1) Sequencial    : sem diretivas OpenMP.
   2) parallel for  : paralelismo de DADOS (as threads dividem os laços de cada variável).
   3) sections      : paralelismo de TAREFAS (uma thread calcula a estatura e
                      outra calcula o peso, ao mesmo tempo).

 Compilar e rodar (PowerShell):
   g++ -O2 -fopenmp cv.cpp -o cv.exe
   $env:OMP_NUM_THREADS = "4"
   .\cv.exe dados.txt
-----------------------------------------------------*/

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <omp.h>

// Resultado do cálculo de uma variável (estatura ou peso).
struct Resultado {
    double media;
    double desvio_padrao;
    double cv;
    int    num_classes;
};

/*---------------------------------------------------
 VERSÃO 1: SEQUENCIAL (sem nenhuma diretiva OpenMP)
-----------------------------------------------------*/
Resultado cv_sequencial(const std::vector<double>& dados, double largura) {
    const int N = static_cast<int>(dados.size());

    // Menor e maior valor: servem para montar as classes.
    double menor = dados[0];
    double maior = dados[0];
    for (int i = 0; i < N; ++i) {
        if (dados[i] < menor) menor = dados[i];
        if (dados[i] > maior) maior = dados[i];
    }

    const double limite_inf = std::floor(menor);                          // início da 1ª classe
    const int k = static_cast<int>((maior - limite_inf) / largura) + 1;   // número de classes

    // Frequência de cada classe (quantas pessoas caem em cada intervalo).
    std::vector<int> freq(k, 0);
    for (int i = 0; i < N; ++i) {
        const int classe = static_cast<int>((dados[i] - limite_inf) / largura);
        freq[classe]++;
    }

    // Passada 1: média ponderada (xi = ponto médio da classe).
    double soma = 0.0;
    for (int i = 0; i < k; ++i) {
        const double xi = limite_inf + (i + 0.5) * largura;
        soma += xi * freq[i];
    }
    const double mu = soma / static_cast<double>(N);

    // Passada 2: desvio-padrão populacional.
    double soma_desvios2 = 0.0;
    for (int i = 0; i < k; ++i) {
        const double xi = limite_inf + (i + 0.5) * largura;
        const double d = xi - mu;
        soma_desvios2 += freq[i] * d * d;
    }
    const double sigma = std::sqrt(soma_desvios2 / static_cast<double>(N));

    Resultado r;
    r.media = mu;
    r.desvio_padrao = sigma;
    r.cv = (sigma / mu) * 100.0;
    r.num_classes = k;
    return r;
}

/*---------------------------------------------------
 VERSÃO 2: PARALLEL FOR (paralelismo de DADOS)
-----------------------------------------------------
 Mesmo cálculo da versão sequencial, com as diretivas nos laços principais.
-----------------------------------------------------*/
Resultado cv_paralelo(const std::vector<double>& dados, double largura) {
    const int N = static_cast<int>(dados.size());

    // Menor e maior valor (compartilhados: o resultado final fica aqui).
    double menor = dados[0];
    double maior = dados[0];

    #pragma omp parallel
    {
        // Cópias PRIVADAS de cada thread (declaradas dentro da região paralela).
        double menor_local = dados[0];
        double maior_local = dados[0];

        // Cada thread procura o menor/maior só na sua parte do vetor.
        // nowait: não precisa esperar as outras, pois o critical abaixo
        // já sincroniza a junção dos resultados.
        #pragma omp for nowait
        for (int i = 0; i < N; ++i) {
            if (dados[i] < menor_local) menor_local = dados[i];
            if (dados[i] > maior_local) maior_local = dados[i];
        }

        // Junção: UMA thread por vez atualiza o menor/maior compartilhados.
        #pragma omp critical
        {
            if (menor_local < menor) menor = menor_local;
            if (maior_local > maior) maior = maior_local;
        }
    } // barreira implícita no fim do parallel: menor e maior estão prontos.

    const double limite_inf = std::floor(menor);
    const int k = static_cast<int>((maior - limite_inf) / largura) + 1;

    // Frequências: várias threads podem cair na MESMA classe ao mesmo tempo
    // e incrementar freq[classe] juntas (condição de corrida).
    // O critical deixa UMA thread por vez fazer o incremento.
    std::vector<int> freq(k, 0);
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        const int classe = static_cast<int>((dados[i] - limite_inf) / largura);
        #pragma omp critical
        freq[classe]++;
    }

    // Passada 1: média ponderada.
    double soma = 0.0;
    #pragma omp parallel for reduction(+:soma)
    for (int i = 0; i < k; ++i) {
        const double xi = limite_inf + (i + 0.5) * largura;
        soma += xi * freq[i];
    }
    const double mu = soma / static_cast<double>(N);

    // Passada 2: desvio-padrão populacional. Aqui μ já é conhecido e é apenas LIDO.
    double soma_desvios2 = 0.0;
    #pragma omp parallel for reduction(+:soma_desvios2)
    for (int i = 0; i < k; ++i) {
        const double xi = limite_inf + (i + 0.5) * largura;
        const double d = xi - mu;
        soma_desvios2 += freq[i] * d * d;
    }
    const double sigma = std::sqrt(soma_desvios2 / static_cast<double>(N));

    Resultado r;
    r.media = mu;
    r.desvio_padrao = sigma;
    r.cv = (sigma / mu) * 100.0;
    r.num_classes = k;
    return r;
}

/*---------------------------------------------------
 VERSÃO 3: SECTIONS (paralelismo de TAREFAS)
-----------------------------------------------------
 Calcular a estatura e calcular o peso são tarefas DIFERENTES e INDEPENDENTES.
 Cada section escreve numa variável diferente -> não há conflito.

 Dentro de cada section usamos cv_sequencial: o OpenMP não cria threads
 dentro de threads por padrão, então as diretivas de cv_paralelo seriam ignoradas.
-----------------------------------------------------*/
void cv_sections(const std::vector<double>& estatura, const std::vector<double>& peso,
                 Resultado& r_estatura, Resultado& r_peso) {
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            r_estatura = cv_sequencial(estatura, 8.0);
        }

        #pragma omp section
        {
            r_peso = cv_sequencial(peso, 4.0);
        }
    } // Barreira implícita: espera as duas sections terminarem.
}

/*---------------------------------------------------
 Validação dos dados de entrada (reduction com &&, como no 007_reduction_0.2)
 Todos os valores precisam ser positivos; um zero ou negativo estragaria as classes.
-----------------------------------------------------*/
bool dados_ok(const std::vector<double>& dados) {
    const int N = static_cast<int>(dados.size());
    bool dados_validos = true;   // valor neutro do &&

    #pragma omp parallel for reduction(&&:dados_validos)
    for (int i = 0; i < N; ++i) {
        if (dados[i] <= 0) {
            dados_validos = false;   // a thread "vota": nem tudo está OK!
        }
    }
    return dados_validos;
}

int main(int argc, char** argv) {
    // ---------------------------
    // Leitura do arquivo
    // ---------------------------
    const char* arquivo = (argc > 1) ? argv[1] : "dados.txt";
    std::ifstream entrada(arquivo);
    if (!entrada) {
        std::cerr << "Nao foi possivel abrir " << arquivo << " (rode o gerar antes)\n";
        return 1;
    }

    std::vector<double> estatura;
    std::vector<double> peso;
    double e, p;
    while (entrada >> e >> p) {
        estatura.push_back(e);
        peso.push_back(p);
    }
    if (estatura.empty()) {
        std::cerr << "Arquivo vazio\n";
        return 1;
    }
    std::cout << estatura.size() << " pessoas lidas de " << arquivo << "\n";

    if (!dados_ok(estatura) || !dados_ok(peso)) {
        std::cout << "[ALERTA] Existe estatura ou peso menor ou igual a zero.\n";
        return 1;
    }
    std::cout << "[OK] Todos os registros sao validos (positivos).\n\n";

    // ---------------------------
    // Resultados (calculados com sections)
    // ---------------------------
    Resultado r_estatura, r_peso;
    cv_sections(estatura, peso, r_estatura, r_peso);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Estatura (classes de 8 cm, k = " << r_estatura.num_classes << ")\n";
    std::cout << "  Media         : " << r_estatura.media << " cm\n";
    std::cout << "  Desvio-padrao : " << r_estatura.desvio_padrao << " cm\n";
    std::cout << "  CV            : " << r_estatura.cv << " %\n\n";

    std::cout << "Peso (classes de 4 kg, k = " << r_peso.num_classes << ")\n";
    std::cout << "  Media         : " << r_peso.media << " kg\n";
    std::cout << "  Desvio-padrao : " << r_peso.desvio_padrao << " kg\n";
    std::cout << "  CV            : " << r_peso.cv << " %\n\n";

    // ---------------------------
    // Quantas threads foram realmente usadas
    // ---------------------------
    int threads_usadas = 1;
    #pragma omp parallel
    {
        #pragma omp single
        threads_usadas = omp_get_num_threads();   // só UMA thread escreve
    }

    // ---------------------------
    // Análise de desempenho
    // ---------------------------
    // Com só 1000 pessoas, uma execução leva microssegundos. Por isso repetimos
    // o cálculo REPS vezes para o tempo ficar mensurável.
    const int REPS = 20000;

    // 'acumulado' usa os resultados, para o compilador não descartar o cálculo.
    double acumulado = 0.0;

    // --- Sequencial ---
    double start_time = omp_get_wtime();
    for (int r = 0; r < REPS; ++r) {
        const Resultado re = cv_sequencial(estatura, 8.0);
        const Resultado rp = cv_sequencial(peso, 4.0);
        acumulado += re.cv + rp.cv;
    }
    const double tempo_seq = omp_get_wtime() - start_time;

    // --- parallel for ---
    start_time = omp_get_wtime();
    for (int r = 0; r < REPS; ++r) {
        const Resultado re = cv_paralelo(estatura, 8.0);
        const Resultado rp = cv_paralelo(peso, 4.0);
        acumulado += re.cv + rp.cv;
    }
    const double tempo_for = omp_get_wtime() - start_time;

    // --- sections ---
    start_time = omp_get_wtime();
    for (int r = 0; r < REPS; ++r) {
        Resultado re, rp;
        cv_sections(estatura, peso, re, rp);
        acumulado += re.cv + rp.cv;
    }
    const double tempo_sec = omp_get_wtime() - start_time;

    std::cout << "Desempenho (" << REPS << " repeticoes, " << threads_usadas << " threads):\n";
    std::cout << std::setprecision(4);
    std::cout << "  Sequencial   : " << tempo_seq << " s\n";
    std::cout << "  parallel for : " << tempo_for << " s  (speedup " << tempo_seq / tempo_for << "x)\n";
    std::cout << "  sections     : " << tempo_sec << " s  (speedup " << tempo_seq / tempo_sec << "x)\n";
    std::cout << "  (controle: " << acumulado << ")\n";

    return 0;
}

/*
 Saída esperada (os tempos variam de máquina para máquina):
   1000 pessoas lidas de dados.txt
   [OK] Todos os registros sao validos (positivos).

   Estatura (classes de 8 cm, k = ...)
     CV            : em torno de 6 %
   Peso (classes de 4 kg, k = ...)
     CV            : em torno de 17 %

 (Os dados foram gerados com média 170 cm / 70 kg e desvio 10 cm / 12 kg,
  então os CVs ficam perto de 10/170 e 12/70. O agrupamento em classes
  muda um pouco o valor exato.)

 Observações sobre o desempenho:

 1.  Com apenas 1000 pessoas, o trabalho é pequeno e o custo de criar e
     sincronizar as threads pode ser maior que o ganho. Por isso o parallel for
     pode empatar ou até perder do sequencial.

 2.  No laço das frequências, o critical faz as threads esperarem umas
     pelas outras, o que reduz o ganho. Os laços da média e do desvio-padrão
     têm só k iterações (uma por classe, algo em torno de 10 a 15) e quase
     não se beneficiam de paralelismo.

 3.  O sections tem apenas 2 tarefas (estatura e peso). Com mais de 2 threads
     o ganho não aumenta: o speedup máximo fica perto de 2x.
*/