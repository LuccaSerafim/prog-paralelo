CXX      = g++
CXXFLAGS = -O2 -Wall
OMPFLAG  = -fopenmp

all: gerar cv

gerar: gerar.cpp
	$(CXX) $(CXXFLAGS) gerar.cpp -o gerar

cv: cv.cpp
	$(CXX) $(CXXFLAGS) $(OMPFLAG) cv.cpp -o cv

dados.txt: gerar
	./gerar

run: cv dados.txt
	./cv dados.txt

clean:
	rm -f gerar cv

.PHONY: all run clean