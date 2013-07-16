
#CC=diet gcc
CC=gcc
#CFLAGS=-nostdinc -Wall -g -D_GNU_SOURCE -I/usr/include/diet -I/usr/local/include
CFLAGS=-g -std=c99 -pedantic -Wall -D_GNU_SOURCE -I/usr/local/include
LDFLAGS=-s -L/usr/local/lib -lowfat -lgit2

slcp: slcp.o
	$(CC) -o $@ slcp.o $(LDFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f slcp.o slcp
