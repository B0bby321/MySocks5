

CC=g++
FLAGS=-std=c++11

all: main.o connectionPool.h entryPoint.o socksCore.o
	$(CC) -oserver.out main.o entryPoint.o socksCore.o -pthread 


entryPoint.o: entryPoint.cpp entryPoint.h
	$(CC) -c entryPoint.cpp $(FLAGS)

socksCore.o: socksCore.cpp socksCore.h
	$(CC) -c socksCore.cpp $(FLAGS)

main.o: main.cpp
	$(CC) -c main.cpp $(FLAGS)

clean:
	rm *.o
