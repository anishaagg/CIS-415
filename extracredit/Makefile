CFLAGS=-W -Wall -g
OBJECTS= linkedlist.o include_crawler.o tshashmap.o tslinkedlist.o iterator.o tsiterator.o hashmap.o

include_crawler: $(OBJECTS)
	gcc -o $@ $(CFLAGS) $(OBJECTS) -pthread

clean:
	rm $(OBJECTS) 

