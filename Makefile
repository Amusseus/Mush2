SHELL = /bin/sh
CC = gcc
CFLAGS = -Wall -g
LD = gcc
LDFLAGS = -Wall -g
SCRIPT = fooScript

all: mush2

mush2: mush2.o util.o
	$(LD) $(LDFLAGS) -L ~pn-cs357/Given/Mush/lib64 -o mush2 mush2.o util.o -lmush

mush2.o: mush2.c
	$(CC) $(CFLAGS) -I ~pn-cs357/Given/Mush/include -c -o mush2.o mush2.c

util.o: util.c
	$(CC) $(CFLAGS) -c -o util.o util.c

nico: 
	~pn-cs357/demos/mush2

nicoscript: 
	~pn-cs357/demos/mush2 $(SCRIPT)	


clean: 
	rm *.o
	rm mush2

