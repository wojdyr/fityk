OBJS = xylib.o testmain.o
CC = g++
CFLAGS = -Wall -O -g

# NOTE: these paths may need to be changed
BOOST_PATH = /usr/include/boost
BOOST_REGEX_LIB = /usr/lib64/libboost_regex.a

all : $(OBJS)
	$(CC) $(OBJS) -o testmain  $(BOOST_REGEX_LIB)

xylib.o : xylib.cpp xylib.h
	$(CC) $(CFLAGS) -I $(BOOST_PATH) -c xylib.cpp -o xylib.o $(BOOST_REGEX_LIB)
	#g++ xylib.o testmain.o -o testmain  /usr/lib64/libboost_regex.a

testmain.o : testmain.cpp testmain.h
	$(CC) $(CFLAGS) -c testmain.cpp -o testmain.o

clean:
	rm -rf *.o testmain
	# remove the exported files in the out/ dir
	find out/ -name "*.txt" -type f -print | xargs /bin/rm -f
