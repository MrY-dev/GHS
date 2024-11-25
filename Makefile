CC = mpic++

all : main

main: main.cpp
	$(CC) main.cpp -o main

debug: main.cpp
	$(CC) -DDEBUG main.cpp -o main


