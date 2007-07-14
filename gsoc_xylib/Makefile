CC = gcc
XX = g++
CFLAGS = -Wall -W -O -g
TARGET = testmain

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o:%.cpp
	$(XX) $(CFLAGS) -c $< -o $@

SOURCES = $(wildcard *.c *.cpp)
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

all : $(OBJS)
	 $(XX) $(OBJS) -o $(TARGET)

	 chmod a+x $(TARGET)

clean:
	rm -rf *.o testmain
	# remove the exported files in the out/ dir
	find out/ -name "*.txt" -type f -print | xargs /bin/rm -f
