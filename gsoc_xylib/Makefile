CC = gcc
CXX = g++
CFLAGS = -Wall -W -O -g -I ./3rdparty
TARGET= xyconv

EXISTED = $(shell test -e output && echo $$?)

%.o:%.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

LIB_SOURCES = $(wildcard ds_*.cpp) xylib.cpp util.cpp
LIB_OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(LIB_SOURCES)))

all : xyconv.o $(LIB_OBJS) 
	 $(CXX) $< $(LIB_OBJS) -o $(TARGET)

xyconv: xyconv.o $(LIB_OBJS) 
	 $(CXX) $< $(LIB_OBJS) -o $@

clean:
	rm -rf *.o xyconv
	# remove the exported files in the output/ dir, if exists
ifeq (0,${EXISTED})
	find output/ -name "*.txt" -type f -print | xargs /bin/rm -f
	rmdir output/*
	rmdir output
endif
	@echo 'cleaned'
