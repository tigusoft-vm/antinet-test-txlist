
.PHONY: run

a.bin: a.cpp
	g++  --std=c++11   -Wall a.cpp -o a.bin 

run: a.bin
	./a.bin

