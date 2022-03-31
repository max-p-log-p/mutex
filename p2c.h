#include "proc.h"

enum Args { PROG_NAME = 0, ADDRESS1, ADDRESS2, ADDRESS3, ID, ARGS_LEN };

#define NUM_SERVERS 3

/* ENQUIRY */
struct server {
	int32_t fd;
} servers[NUM_SERVERS];

int32_t maxFile;
