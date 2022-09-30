.PHONY: all clean

all:
	g++ -std=c++11 -march=native converter.cpp -o converter -O3 -fopenmp
	
clean:
	rm -rf converter
