SOURCES = uspsv1.c uspsv2.c uspsv3.c uspsv4.c p1fxns.c cpubound.c iobound.c
OBJECTS = $(SOURCES: .c=.o)
HEADERS = p1fxns.h

RM = rm
CC = gcc -W -Wall

%.o: %.c
	$(CC) -c $<

all: uspsv1 uspsv2 uspsv3 uspsv4 cpubound iobound

uspsv1: uspsv1.o p1fxns.o
	$(CC) -o $@ uspsv1.o p1fxns.o

uspsv2: uspsv2.o p1fxns.o
	$(CC) -o $@ uspsv2.o p1fxns.o

uspsv3: uspsv3.o p1fxns.o
	$(CC) -o $@ uspsv3.o p1fxns.o

uspsv4: uspsv4.o p1fxns.o
	$(CC) -o $@ uspsv4.o p1fxns.o

cpubound: cpubound.o
	$(CC) -o $@ cpubound.o

iobound: iobound.o 
	$(CC) -o $@ iobound.o

clean:
	$(RM) uspsv1 uspsv2 uspsv3 uspsv4 cpubound iobound *.o 
