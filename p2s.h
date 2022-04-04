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

struct thread {
	pthread_t pthread;
	int32_t tid;
};

/* FIFO */
struct Msg wrReqs[NUM_FILES][NUM_CLIENTS];
static int32_t deferred[NUM_CLIENTS];
static uint32_t pos[NUM_FILES];

static int32_t sockets[NUM_PORTS];
static uint32_t id;
static int32_t fds[NUM_FILES];
static struct thread enqRepliers[NUM_CLIENTS];
static struct thread invokers[NUM_CLIENTS];
static struct thread writers[NUM_FILES];

/* exclude self */
static struct thread counters[NUM_SERVERS - 1];
static struct thread wrRepliers[NUM_SERVERS - 1];

static pthread_mutex_t mutexes[NUM_MUTEXES];
static pthread_cond_t conds[NUM_CONDS];
