#include <limits.h>

#include "proc.h"

enum Args { PROG_NAME = 0, ID, SERVER_FILE, ARGS_LEN };

#define MAX_SLEEP 2
#define STOP_PROB 100 /* 1 / 100 */
#define USAGE_STR "p2c id serverFile"
