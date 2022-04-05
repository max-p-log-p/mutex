#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "net.h"
#include "p2c.h"

int32_t
main(int32_t argc, char * const *argv)
{
	uint32_t p2time;
	int32_t numFiles, sfds[NUM_SERVERS];
	uint64_t i, id, r;
	struct Msg req, reply;
	char addrs[NUM_SERVERS][HOST_NAME_MAX];

	if (argc != ARGS_LEN)
		usage(USAGE_STR);

	/* when tloc is NULL, time() can't fail */
	srand(time(NULL));

	if ((id = strtoul(argv[ID], NULL, 10)) > NUM_CLIENTS)
		errx(1, "id > %d", NUM_CLIENTS);
	req.id = (uint32_t)id;

	getHosts(argv[SERVER_FILE], addrs, LEN(addrs));

	req.data = ENQ_REQ;

	/* Enquiry Request */
	r = rand() % NUM_SERVERS;
	p2time = 0;

	if ((sfds[r] = createSocket(addrs[r], PORT_STR[EREQ_PORT], 0)) < 0)
		err(1, "createSocket");

	if (writeMsg(sfds[r], req))
		err(1, "writeMsg");

	if (readMsg(sfds[r], &reply))
		err(1, "recv");

	numFiles = reply.data;
	printMsg("reply:", reply);

	sleep(rand() % PRESLEEP + MIN_PRESLEEP);

	do {
		r = rand();
		i = r % NUM_SERVERS;

		/* Write Request */
		if ((sfds[i] = createSocket(addrs[i], PORT_STR[C_WREQ_PORT], 0)) < 0)
			err(1, "createSocket");

		req.data = rand() % numFiles;
		req.time = ++p2time;

		printf("server %ld ", i);
		printMsg("req:", req);

		if (writeMsg(sfds[i], req))
			err(1, "writeMsg");

		if (readMsg(sfds[i], &reply))
			err(1, "recv");

		printMsg("reply:", reply);

		usleep(rand() % MAX_SLEEP);
	} while (r > RAND_MAX / STOP_PROB);

	/* close sockets */
	for (i = 0; i < NUM_SERVERS; ++i)
		close(sfds[i]);

	return 0;
}
