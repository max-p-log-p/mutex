#include "proc.h"

#define NUM_SERVERS 3 // /* exclude self */

enum Args { PROG_NAME = 0, ADDRESS0, ADDRESS1, ADDRESS2, PATH, ARGS_LEN };

/* ENQUIRY */
struct server {
	int32_t fd;
	char *addr;
} servers[NUM_SERVERS];

int32_t files[NUM_FILES]; /* fds */

/* '/' + [0-4] */
#define FILENAME_LEN (2 * sizeof(uint8_t))
