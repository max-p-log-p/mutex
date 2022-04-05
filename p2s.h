#include <pthread.h>

#include "proc.h"

enum Args { PROG_NAME = 0, ID, PATH, SERVER_FILE, ARGS_LEN };
enum Mutexes { TIME, FIFO, _FILE, COND, EMPTY, SOCK, REPLY, NUM_MUTEXES };
enum Conds { ENTER, WRITE, EXIT, _EMPTY, NUM_CONDS };

#define USAGE_STR "p2s id path serverFile"

struct thread {
	pthread_t pthread;
	int32_t tid;
};

struct Fifo {
	uint32_t pos;
	struct Msg msgs[NUM_CLIENTS];
} fifos[NUM_FILES];

static uint32_t numReplies[NUM_FILES];
static uint32_t id, p2time;

static int32_t fds[NUM_FILES];
static int32_t lsocks[NUM_PORTS];
static int32_t csocks[NUM_CLIENTS];

static pthread_mutex_t mutexes[NUM_MUTEXES];

static pthread_cond_t conds[NUM_CONDS];

static char addrs[NUM_SERVERS][HOST_NAME_MAX];
