# Exercício Final - Coeficiente de Variação com OpenMP

Programação Paralela (CCO085) - IESB - 2026/2
Prof. Rodrigo Gonçalves Pinto
Aluno: Lucca di Stefanno Brito Serafim - Matrícula 2322130021

Cálculo do coeficiente de variação da **estatura** (classes de 8 cm) e do **peso**
(classes de 4 kg) de 1000 pessoas, para dados agrupados em classes, usando OpenMP em C++.

## Resultados

| Variável | Média | Desvio-padrão | CV |
|---|---|---|---|
| Estatura | 170,39 cm | 10,21 cm | **5,99%** |
| Peso | 70,34 kg | 12,53 kg | **17,81%** |

A análise de desempenho (sequencial x `parallel for` x `sections`) está no `relatorio.pdf`.

## Arquivos

- `cv.cpp`: cálculo (versão sequencial, `parallel for` e `sections`)
- `gerar.cpp`: gera o `dados.txt` (estatura e peso de 1000 pessoas, semente fixa)
- `dados.txt`: dados usados nos testes
- `relatorio.pdf`: relatório com o que foi paralelizado, resultados e desempenho
- `Makefile`, `Dockerfile`, `docker-compose.yml`: compilação e ambiente da disciplina

## Como rodar no container da disciplina

```bash
docker compose up -d --build
docker exec -it pp-lab-cv bash
make run
```

O `make run` compila, gera o `dados.txt` se ele não existir e executa o `cv`.

Para testar com diferentes números de threads:

```bash
OMP_NUM_THREADS=1 ./cv dados.txt
OMP_NUM_THREADS=2 ./cv dados.txt
OMP_NUM_THREADS=4 ./cv dados.txt
```

## Como rodar sem Docker

Precisa de `g++` com suporte a OpenMP.

```bash
g++ -O2 gerar.cpp -o gerar
g++ -O2 -fopenmp cv.cpp -o cv
./gerar
./cv dados.txt
```
