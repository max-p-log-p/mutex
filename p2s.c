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

#define MODE S_IRWXU
#define FLAGS (O_CREAT|O_WRONLY|O_TRUNC|O_APPEND|O_SYNC)

void pthreads_create(struct thread [], void *(*)(void *), size_t);
void pthreads_join(struct thread [], size_t);
void broadcast(struct Msg);
void broadcastLog(struct Msg [], uint32_t);
void setTime(uint32_t);
void setMaxTime(uint32_t);
void *enquireReply(void *);
void *append(void *);
void *appendLog(void *);
void *queue(void *);
void *count(void *);
void *writeReply(void *);

void
pthreads_create(struct thread t[], void *(*f)(void *), size_t len)
{
	uint64_t i;
	for (i = 0; i < len; ++i) {
		t[i].tid = i;
		if (pthread_create(&t[i].pthread, NULL, f, (void *)&t[i].tid))
			err(1, "pthread_create");
	}
}

void
pthreads_join(struct thread t[], size_t len)
{
	uint64_t i;
	for (i = 0; i < len; ++i) {
		if (pthread_join(t[i].pthread, NULL))
			warn("pthread_join");
	}
}

void
setTime(uint32_t time)
{
	pthread_mutex_lock(&mutexes[TIME]);
	p2time = (time > p2time) ? time + 1 : p2time + 1;
	pthread_mutex_unlock(&mutexes[TIME]);
}

void
setMaxTime(uint32_t time)
{
	pthread_mutex_lock(&mutexes[MAX]);
	maxTime = (time > maxTime) ? time : maxTime;
	pthread_mutex_unlock(&mutexes[MAX]);
}

int32_t
main(int32_t argc, char * const *argv)
{
	uint64_t i, tmpid;
	uint32_t j;
	char file[sizeof(j) * 2]; /* size in hex */
	char path[PATH_MAX] = { 0 };
	struct Msg req;

	struct thread writers[NUM_FILES];
	struct thread enqRepliers[NUM_CLIENTS];
	struct thread queuers[NUM_CLIENTS];
	struct thread loggers[NUM_SERVERS - 1];
	struct thread counters[NUM_SERVERS - 1];
	struct thread wrRepliers[NUM_SERVERS - 1];

	if (argc != ARGS_LEN)
		usage(USAGE_STR);

	/* Get ID */
	if ((tmpid = strtoul(argv[ID], NULL, 10)) > NUM_SERVERS)
		errx(1, "id > %d", NUM_SERVERS);
	id = req.id = (uint32_t)tmpid;

	/* Get servers */
	getHosts(argv[SERVER_FILE], addrs, LEN(addrs));

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
		if ((fds[j] = open(path, FLAGS, MODE)) < 0)
			err(1, "open");
	}

	/* Initialization */
	for (i = 0; i < NUM_MUTEXES; ++i)
		if (pthread_mutex_init(&mutexes[i], NULL))
			err(1, "pthread_mutex_init");

	for (i = 0; i < NUM_PORTS; ++i)
		lsocks[i] = listenSocket(addrs[id], PORT_STR[i]);

	pthreads_create(enqRepliers, enquireReply, LEN(enqRepliers));
	pthreads_create(queuers, queue, LEN(queuers));
	pthreads_create(loggers, appendLog, LEN(loggers));
	pthreads_create(writers, append, LEN(writers));
	pthreads_create(counters, count, LEN(counters));
	pthreads_create(wrRepliers, writeReply, LEN(wrRepliers));

	/* wait for threads */
	pthreads_join(enqRepliers, LEN(enqRepliers));
	pthreads_join(queuers, LEN(queuers));
	pthreads_join(loggers, LEN(loggers));
	pthreads_join(writers, LEN(writers));
	pthreads_join(counters, LEN(counters));
	pthreads_join(wrRepliers, LEN(wrRepliers));

	/* close sockets */
	for (i = 0; i < NUM_PORTS; ++i)
		close(lsocks[i]);

	return 0;
}

void *
enquireReply(void *arg)
{
	int32_t afd, tid;
	struct Msg msg, reply;

	tid = *(int32_t *)arg;
	reply.data = ENQ_REP;
	reply.id = id;

	for (;;) {
		if ((afd = acceptSocket(lsocks[EREQ_PORT])) < 0)
			err(1, "accept: %d", tid);

		if (readMsg(afd, &msg))
			err(1, "recv: %d", tid);

		printMsg("Enquiry from", msg);

		if (writeMsg(afd, reply))
			err(1, "writeMsg: %d", tid);

		close(afd);
	}

	return NULL;
}

void *
queue(void *arg)
{
	int32_t afd, tid;
	struct Msg msg;
	struct Fifo *fifo;

	tid = *(int32_t *)arg;

	for (;;) {
		if ((afd = acceptSocket(lsocks[C_WREQ_PORT])) < 0)
			err(1, "accept: %d", tid);

		if (readMsg(afd, &msg))
			err(1, "recv: %d", tid);

		if (msg.data > NUM_FILES)
			errx(1, "invalid file");

		fifo = &fifos[msg.data];

		if (fifo->pos >= NUM_CLIENTS)
			errx(1, "Too many requests");

		if (pthread_mutex_lock(&mutexes[FIFO]))
			err(1, "pthread_mutex_lock");

		fifo->msgs[fifo->pos++] = msg;

		if (pthread_mutex_unlock(&mutexes[FIFO]))
			err(1, "pthread_mutex_unlock");

		printMsg("added to queue:", msg);

		if (pthread_cond_broadcast(&conds[WRITE]))
			err(1, "pthread_cond_broadcast");

		/* block until queue is empty */
		if (pthread_mutex_lock(&mutexes[EMPTY]))
			err(1, "pthread_mutex_lock");

		while (fifo->pos > 0) {
			if (pthread_cond_wait(&conds[EXIT], &mutexes[EMPTY]))
				err(1, "pthread_cond_wait");
		}

		if (pthread_mutex_unlock(&mutexes[EMPTY]))
			err(1, "pthread_mutex_unlock");

		if (writeMsg(afd, msg))
			err(1, "writeMsg: %d", tid);
	}

	return NULL;
}

void *
append(void *arg)
{
	uint64_t i;
	int32_t fnum;
	struct Msg msg, req;
	struct Fifo *fifo;

	fnum = *(int32_t *)arg;
	fifo = &fifos[fnum];

	req.id = id;
	req.data = fnum;

	for (;;) {
		/* block until queue is not empty */
		if (pthread_mutex_lock(&mutexes[COND]))
			err(1, "pthread_mutex_lock");

		while (fifo->pos == 0) {
			if (pthread_cond_wait(&conds[WRITE], &mutexes[COND]))
				err(1, "pthread_cond_wait");
		}

		if (pthread_mutex_unlock(&mutexes[COND]))
			err(1, "pthread_mutex_unlock");

		setTime(maxTime);
		req.time = p2time;
		printMsg("broadcast", req);
		broadcast(req);

		/* enter critical section after all servers reply */
		if (pthread_mutex_lock(&mutexes[ENTER]))
			err(1, "pthread_mutex_lock");

		while (numReplies[fnum] != NUM_SERVERS - 1) {
			if (pthread_cond_wait(&conds[_ENTER], &mutexes[ENTER]))
				err(1, "pthread_cond_wait");
		}

		if (pthread_mutex_unlock(&mutexes[ENTER]))
			err(1, "pthread_mutex_unlock");

		/* Reset numReplies */
		numReplies[fnum] = 0;

		/* appends to file from FIFO (optimization) */
		for (i = 0; i < fifo->pos; ++i) {
			msg = fifo->msgs[i];

			printf("write %d %d %d\n", fnum, msg.id, msg.time);

			if (pthread_mutex_lock(&mutexes[_FILE]))
				err(1, "pthread_mutex_lock");

			if (dprintf(fds[fnum], "%d %d\n", msg.id, msg.time) < 0)
				err(1, "dprintf");

			if (pthread_mutex_unlock(&mutexes[_FILE]))
				err(1, "pthread_mutex_unlock");
		}

		/* send fifo to other servers */
		broadcastLog(fifo->msgs, fifo->pos);

		puts("Clear FIFO");

		if (pthread_mutex_lock(&mutexes[FIFO]))
			err(1, "pthread_mutex_lock");

		/* Clear FIFO */
		fifo->pos = 0;

		if (pthread_mutex_unlock(&mutexes[FIFO]))
			err(1, "pthread_mutex_unlock");

		if (pthread_mutex_lock(&mutexes[EMPTY]))
			err(1, "pthread_mutex_lock");
		if (pthread_mutex_lock(&mutexes[DEFER]))
			err(1, "pthread_mutex_lock");

		/* send deferred replies */
		if (pthread_cond_broadcast(&conds[EXIT]))
			err(1, "pthread_cond_broadcast");

		if (pthread_mutex_unlock(&mutexes[DEFER]))
			err(1, "pthread_mutex_unlock");
		if (pthread_mutex_unlock(&mutexes[EMPTY]))
			err(1, "pthread_mutex_unlock");
	}

	return NULL;
}

/* Count replies */
void *
count(void *arg)
{
	int32_t afd, tid;
	struct Msg msg;

	tid = *(int32_t *)arg;

	for (;;) {
		if ((afd = acceptSocket(lsocks[WREP_PORT])) < 0)
			err(1, "accept: %d", tid);

		if (readMsg(afd, &msg))
			err(1, "recv: %d", tid);

		printMsg("Server reply", msg);

		if (msg.data >= NUM_FILES)
			errx(1, "invalid file");

		if (pthread_mutex_lock(&mutexes[REPLY]))
			err(1, "pthread_mutex_lock");

		++numReplies[msg.data];

		if (pthread_mutex_unlock(&mutexes[REPLY]))
			err(1, "pthread_mutex_unlock");

		printf("numReplies %d\n", numReplies[msg.data]);

		if (numReplies[msg.data] == NUM_SERVERS - 1)
			pthread_cond_broadcast(&conds[_ENTER]);
	}

	return NULL;
}

/* write replies */
void *
writeReply(void *arg)
{
	int32_t afd, sfd, tid;
	struct Msg msg;
	struct Fifo *fifo;

	tid = *(int32_t *)arg;

	for (;;) {
		if ((afd = acceptSocket(lsocks[S_WREQ_PORT])) < 0)
			err(1, "accept: %d", tid);

		if (readMsg(afd, &msg))
			err(1, "recv: %d", tid);

		printMsg("Server request", msg);

		setMaxTime(msg.time);
		fifo = fifos + msg.data;

		/* lower ids have higher priority */
		if (fifo->pos && ((p2time < msg.time) ||
			(p2time == msg.time && id < msg.id))) {
			/* Defer reply */
			if (pthread_mutex_lock(&mutexes[DEFER]))
				err(1, "pthread_mutex_lock");

			while (fifo->pos > 0) {
				if (pthread_cond_wait(&conds[EXIT], &mutexes[DEFER]))
					err(1, "pthread_cond_wait");
			}

			if (pthread_mutex_unlock(&mutexes[DEFER]))
				err(1, "pthread_mutex_unlock");
		}

		sfd = createSocket(addrs[msg.id], PORT_STR[WREP_PORT], 0);

		msg.id = id;

		if (writeMsg(sfd, msg))
			err(1, "writeMsg");

		printMsg("Reply sent", msg);

		close(sfd);
	}

	return NULL;
}

void *
appendLog(void *arg)
{
	int32_t afd, tid;
	struct Msg msg;

	tid = *(int32_t *)arg;

	for (;;) {
		if ((afd = acceptSocket(lsocks[QPORT])) < 0)
			err(1, "accept: %d", tid);

		if (readMsg(afd, &msg))
			err(1, "recv");

		printMsg("log req:", msg);

		if (pthread_mutex_lock(&mutexes[_FILE]))
			err(1, "pthread_mutex_lock");

		/* append to file */
		if (dprintf(fds[msg.data], "%d %d\n", msg.id, msg.time) < 0)
			err(1, "dprintf");

		if (pthread_mutex_unlock(&mutexes[_FILE]))
			err(1, "pthread_mutex_unlock");

		msg.id = id;

		printMsg("log reply:", msg);

		/* send reply */
		if (writeMsg(afd, msg))
			err(1, "writeMsg");
	}

	return NULL;
}

void
broadcast(struct Msg req)
{
	uint64_t i;
	int32_t sfd;

	for (i = 0; i < LEN(addrs); ++i) {
		if (i == id)
			continue;

		sfd = createSocket(addrs[i], PORT_STR[S_WREQ_PORT], 0);

		if (writeMsg(sfd, req))
			err(1, "writeMsg: %ld", i);

		close(sfd);
	}
}

void
broadcastLog(struct Msg msgs[], uint32_t numMsgs)
{
	uint64_t i, j;
	int32_t sfd;
	struct Msg reply;

	for (i = 0; i < LEN(addrs); ++i) {
		if (i == id)
			continue;

		printf("broadcastLog %s %d\n", addrs[i], numMsgs);

		for (j = 0; j < numMsgs; ++j) {
			sfd = createSocket(addrs[i], PORT_STR[QPORT], 0);

			if (writeMsg(sfd, msgs[j]))
				err(1, "writeMsg: %ld", i);

			printMsg("queue:", msgs[j]);

			if (readMsg(sfd, &reply))
				errx(1, "readMsg");

			printMsg("reply:", msgs[j]);

			close(sfd);
		}
	}
	puts("broadcastLog done");
}
