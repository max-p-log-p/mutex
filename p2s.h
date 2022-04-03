#include <limits.h>
#include <pthread.h>
#include <semaphore.h>

#include "proc.h"

enum Args { PROG_NAME = 0, ID, PATH, SERVER_FILE, ARGS_LEN };
enum Mutexes { TIME, FIFO, COND, NUM_MUTEXES };
enum Conds { ENTER, EXIT, NUM_CONDS };

#define USAGE_STR "p2s id path serverFile"

struct server {
	int32_t sfd;
	sem_t sem[NUM_FILES];
} srvs[NUM_SERVERS - 1]; /* exclude self */

/* FIFO */
struct Msg wrReqs[NUM_FILES][NUM_CLIENTS];
static int32_t deferred[NUM_CLIENTS];
static uint32_t pos[NUM_FILES];

static int32_t sockets[NUM_PORTS];
static uint32_t id;
static int32_t fds[NUM_FILES];
static pthread_t enqRepliers[NUM_CLIENTS];
static pthread_t invokers[NUM_CLIENTS];
static pthread_t writers[NUM_FILES];

/* exclude self */
static pthread_t counters[NUM_SERVERS - 1];
static pthread_t wrRepliers[NUM_SERVERS - 1];

static pthread_mutex_t mutexes[NUM_MUTEXES];
static pthread_cond_t conds[NUM_CONDS];
