OBJS = xylib.o testmain.o
CC = g++
CFLAGS = -Wall -O -g

all : $(OBJS)
	$(CC) $(OBJS) -o testmain

xylib.o : xylib.cpp xylib.h
	$(CC) $(CFLAGS) -c xylib.cpp -o xylib.o

testmain.o : testmain.cpp testmain.h
	$(CC) $(CFLAGS) -c testmain.cpp -o testmain.o

clean:
	rm -rf *.o testmain
	