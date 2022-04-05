CC = cc
CFLAGS = -Wall -Wextra -Werror -O2

all: p2c p2s

p2c: p2c.o net.o proc.o
	$(CC) $(LDFLAGS) -pthread -o p2c p2c.o net.o proc.o

p2s: p2s.o net.o proc.o
	$(CC) $(LDFLAGS) -pthread -o p2s p2s.o net.o proc.o

p2s.o: p2s.c net.h p2s.h proc.h
p2c.o: p2c.c net.h p2c.h proc.h
net.o: net.c net.h
proc.o: proc.c proc.h

clean:
	rm -f p2s p2c net.o p2s.o p2c.o proc.o
