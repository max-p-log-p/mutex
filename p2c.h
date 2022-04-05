#include <limits.h>

#include "proc.h"

enum Args { PROG_NAME = 0, ID, SERVER_FILE, ARGS_LEN };

#define MIN_PRESLEEP 2
#define PRESLEEP 3
#define MAX_SLEEP 2000
#define STOP_PROB 64
#define USAGE_STR "p2c id serverFile"
