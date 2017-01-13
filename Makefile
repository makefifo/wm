#CC = gcc 
#CFLAGS = -g -Wall -O0 

a.out: main.o log.o
	gcc main.o log.o

forward: forward.o parse.o log.o
	gcc forward.o parse.o log.o -o forward

main.o: main.c main.h
	gcc -c main.c

log.o: log.c main.h
	gcc -c log.c

forward.o: forward.c forward.h
	gcc -c forward.c 

parse.o: parse.c
	gcc -c parse.c

clean:
	rm main.o log.o forward.o parse.o
