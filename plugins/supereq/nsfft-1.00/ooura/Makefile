CC=gcc
BASEOPT=-Wall -g
OPT=$(BASEOPT) -O3

all : fftsg.o

clean :
	rm -f *~ *.o a.out

fftsg.o : fftsg.c
	$(CC) $(OPT) -c fftsg.c
