CC = cc
CFLAGS = -Wall -Wextra -Werror -O2 -fanalyzer

all: p2c p2s

p2c: p2c.o net.o
	$(CC) $(LDFLAGS) -pthread -o p2c p2c.o net.o

p2s: p2s.o net.o
	$(CC) $(LDFLAGS) -pthread -o p2s p2s.o net.o

fuzz: fuzz.o net.o
	$(CC) $(LDFLAGS) -pthread -o fuzz fuzz.o net.o

p2s.o: p2s.c net.h p2s.h proc.h
p2c.o: p2c.c net.h p2c.h proc.h
net.o: net.c net.h
fuzz.o: fuzz.c net.h

clean:
	rm -f fuzz p2s fuzz.o net.o p2s.o p2c p2c.o
