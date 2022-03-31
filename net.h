#include <err.h>
#include <limits.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern int readSocket(int, int8_t [], size_t, size_t);
extern ssize_t writeSocket(int, const int8_t *, size_t);
extern int createSocket(const char *, const char *, int);
extern void net_listen(int);
extern int net_accept(int);
