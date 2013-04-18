
CC=diet gcc
CFLAGS=-nostdinc -Wall -g -I/usr/include/diet -I/usr/local/include
LDFLAGS=-s -lowfat -ltermininfo

slcp: slcp.o
	$(CC) -o $@ slcp.o $(LDFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f slcp.o slcp
