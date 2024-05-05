CC = g++ -g -c -Wall -ansi -pedantic -std=c++20

LN = g++

OBJS = scraper.o

exe: $(OBJS)
	$(LN) -o exe $(OBJS) -lcurl -lxml2
 
scraper.o: scraper.cpp
	$(CC) scraper.cpp

clean:
	/bin/rm -f exe *.o *~ \#*
