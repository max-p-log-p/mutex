#include <limits.h>

#include "proc.h"

enum Args { PROG_NAME = 0, ID, SERVER_FILE, ARGS_LEN };

#define MAX_SLEEP 2
#define USAGE_STR "p2c id serverFile"

struct server {
	int32_t sfd, numFiles;
} servers[NUM_SERVERS];

static int32_t fileCount[NUM_FILES];
