#define _GNU_SOURCE /* mkostemp */
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "net.h"
#include "p2s.h"

#define NODE NULL /* wildcard address */
#define MODE S_IRWXU

void pthreads_create(pthread_t [], void *(*)(void *), size_t);
void broadcast(struct Msg);
void setTime(uint32_t);
void *enquireReply(void *);
void *p2write(void *);
void *invoke(void *);
void *count(void *);
void *writeReply(void *);

void
pthreads_create(pthread_t t[], void *(*f)(void *), size_t len)
{
	uint64_t i;
	for (i = 0; i < len; ++i)
		if (pthread_create(&t[i], NULL, f, (void *)&i))
			err(1, "pthread_create");
}

void
setTime(uint32_t time)
{
	pthread_mutex_lock(&mutexes[TIME]);
	p2time = (time > p2time) ? time + 1 : p2time + 1;
	pthread_mutex_unlock(&mutexes[TIME]);
}

int32_t
main(int32_t argc, char * const *argv)
{
	uint64_t i, tmpid;
	uint32_t j;
	char file[sizeof(j) * 2]; /* size in hex */
	char path[PATH_MAX] = { 0 };
	struct Msg req;

	if (argc != ARGS_LEN)
		usage(USAGE_STR);

	/* Get ID */
	if ((tmpid = strtoul(argv[ID], NULL, 10)) > NUM_SERVERS)
		errx(1, "id > %d", NUM_SERVERS);
	id = req.h.id = (uint32_t)tmpid;

	/* Get servers */
	getHostnames(argv[SERVER_FILE], addrs, LEN(addrs));

	if (strlen(argv[PATH]) + sizeof(file) >= sizeof(path))
		errx(1, "path too long");

	for (i = 0; argv[PATH][i] != '\0'; ++i)
		path[i] = argv[PATH][i];

	if (argv[PATH][i - 1] != '/')
		errx(1, "path must end with /");

	if (mkdir(path, MODE) && errno != EEXIST)
		err(1, "mkdir");

	/* Create files */
	for (j = 0; j < NUM_FILES; ++j) {
		if (snprintf(file, sizeof(file), "%x", j) < 0)
			err(1, "snprintf");
		strncpy(path + i, file, sizeof(file));
		if ((fds[i] = creat(path, S_IRWXU)) < 0)
			err(1, "creat");
	}

	/* Initialization */
	for (i = 0; i < NUM_SERVERS - 1; ++i) {
		for (j = 0; j < NUM_FILES; ++j) {
			if (sem_init(&srvs[i].sem[j], 0, 0))
				err(1, "sem_init");
		}
	}

	for (i = 0; i < NUM_MUTEXES; ++i)
		if (pthread_mutex_init(&mutexes[i], NULL))
			err(1, "pthread_mutex_init");

	for (i = 0; i < NUM_PORTS; ++i)
		sockets[i] = listenSocket(addrs[id], PORT_STR[i]);

	pthreads_create(enqRepliers, enquireReply, LEN(enqRepliers));
	pthreads_create(invokers, invoke, LEN(invokers));
	pthreads_create(writers, p2write, LEN(writers));
	pthreads_create(counters, count, LEN(counters));
	pthreads_create(wrRepliers, writeReply, LEN(wrRepliers));

	for (;;)
		;
}

/* reply to enquiries */
void *
enquireReply(void *i)
{
	int32_t afd;
	struct Msg msg, reply;

	reply.data = ENQ_REP;
	reply.h.id = id;

	for (;;) {
		if ((afd = acceptSocket(sockets[EREQ_PORT])) < 0)
			err(1, "accept: %d", *(int32_t *)i);
		if (readSocket(afd, &msg) < sizeof(struct Msg))
			err(1, "recv");
		/* time is not updated because client time is not used */
		printf("Enquiry from %d\n", msg.h.id);
		writeSocket(afd, reply);
	}

	return NULL;
}

/* invoke critical section */
void *
invoke(void *i)
{
	int32_t afd;
	struct Msg msg;

	for (;;) {
		if ((afd = acceptSocket(sockets[C_WREQ_PORT])) < 0)
			err(1, "accept: %d", *(int32_t *)i);
		if (readSocket(afd, &msg) < sizeof(struct Msg))
			err(1, "recv");

		/* time is not updated because client time is not used */
		printf("Req %d %d %d\n", msg.data, msg.h.id, msg.h.time);

		if (msg.data > NUM_FILES)
			errx(1, "invalid file");

		/* client.num_requests <= 1 */
		if (pos[msg.data] >= NUM_CLIENTS)
			errx(1, "Too many requests");

		deferred[msg.h.id] = afd;

		/* add to queue */
		if (pthread_mutex_lock(&mutexes[FIFO]))
			err(1, "pthread_mutex_lock");

		wrReqs[msg.data][pos[msg.data]++] = msg;

		if (pthread_cond_broadcast(&conds[ENTER]))
			err(1, "pthread_cond_signal");

		if (pthread_mutex_unlock(&mutexes[FIFO]))
			err(1, "pthread_mutex_unlock");

	}
	return NULL;
}

void *
p2write(void *arg)
{
	uint32_t i;
	int32_t fnum;
	struct Header h;
	struct Msg msg;

	fnum = *(int32_t *)arg;

	/* block until queue is not empty */
	if (pthread_mutex_lock(&mutexes[COND]))
		err(1, "pthread_mutex_lock");

	while (pos[fnum] == 0)
		if (pthread_cond_wait(&conds[ENTER], &mutexes[COND]))
			err(1, "pthread_cond_wait");

	if (pthread_mutex_unlock(&mutexes[COND]))
		err(1, "pthread_mutex_unlock");

	puts("broadcast");
	/* broadcast write (request critical section) */
	msg.data = fnum;
	msg.h.id = id;
	msg.h.time = p2time;
	broadcast(msg);

	/* enter critical section after all servers reply */
	for (i = 0; i < NUM_SERVERS - 1; ++i)
		sem_wait(&srvs[i].sem[fnum]);

	if (pthread_mutex_lock(&mutexes[FIFO]))
		err(1, "pthread_mutex_lock");

	/* append to file and empty FIFO (optimization) */
	for (i = 0; i < pos[fnum]; ++i) {
		msg = wrReqs[fnum][i];
		h = msg.h;

		/* append to file */
		if (write(fds[fnum], (void *)&h, sizeof(h)) != sizeof(h))
			err(1, "write");

		/* write deferred reply to client */
		msg.data = WRITE_REP;
		writeSocket(deferred[msg.h.id], msg);
	}

	/* Clear FIFO */
	pos[fnum] = 0;

	if (pthread_cond_broadcast(&conds[EXIT]))
		err(1, "pthread_cond_broadcast");

	if (pthread_mutex_unlock(&mutexes[FIFO]))
		err(1, "pthread_mutex_unlock");

	return NULL;
}

/* Count replies */
void *
count(void *i)
{
	int32_t afd;
	struct Msg msg;

	for (;;) {
		if ((afd = acceptSocket(sockets[S_WREQ_PORT])) < 0)
			err(1, "accept: %d", *(int32_t *)i);

		if (readSocket(afd, &msg) < sizeof(struct Msg))
			err(1, "recv");

		setTime(msg.h.time);

		printf("Reply Recv %d %d %d\n", msg.data, msg.h.id, msg.h.time);

		if (msg.data >= NUM_FILES)
			errx(1, "invalid file");

		/* assume server sends <= 1 reply */
		if (sem_post(&srvs[msg.h.id].sem[msg.data]))
			err(1, "sem_post");
	}

	return NULL;
}

/* write replies */
void *
writeReply(void *i)
{
	int32_t afd;
	struct Msg msg, reply;

	for (;;) {
		if ((afd = acceptSocket(sockets[WREP_PORT])) < 0)
			err(1, "accept: %d", *(int32_t *)i);
		if (readSocket(afd, &msg) < sizeof(struct Msg))
			err(1, "recv");

		setTime(msg.h.time);
		printf("Reply Sent %d %d %d\n", msg.data, msg.h.id, msg.h.time);

		/* lower ids have higher priority */
		if (pos[msg.data] == 0 || (p2time > msg.h.time) || 
			(p2time == msg.h.time && id > msg.h.id)) {

			reply.data = WRITE_REP;
			writeSocket(afd, reply);
			continue;
		}

		/* Defer reply */
		if (pthread_mutex_lock(&mutexes[COND]))
			err(1, "pthread_mutex_lock");

		while (pos[msg.data] > 0)
			if (pthread_cond_wait(&conds[EXIT], &mutexes[COND]))
				err(1, "pthread_cond_wait");

		if (pthread_mutex_unlock(&mutexes[COND]))
			err(1, "pthread_mutex_unlock");
	}

	return NULL;
}

void
broadcast(struct Msg req)
{
	uint64_t i;

	for (i = 0; i < LEN(srvs); ++i) {
		if (i == id)
			continue;
		srvs[i].sfd = createSocket(addrs[i], PORT_STR[S_WREQ_PORT], 0);
		writeSocket(srvs[i].sfd, req);
	}
}
