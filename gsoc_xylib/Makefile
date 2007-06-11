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
# remove the exported files in the out/ dir
        find out/ -name "*.txt" -type f -print | xargs /bin/rm -f
