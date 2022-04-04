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
	uint64_t i, id, r;
	struct Msg req, reply;

	if (argc != ARGS_LEN)
		usage(USAGE_STR);

	/* when tloc is NULL, time() can't fail */
	srand(time(NULL));

	if ((id = strtoul(argv[ID], NULL, 10)) > NUM_CLIENTS)
		errx(1, "id > %d", NUM_CLIENTS);
	req.h.id = (uint32_t)id;

	req.data = ENQ_REQ;

	getHostnames(argv[SERVER_FILE], addrs, LEN(addrs));

	/* Enquiry Request */
	for (i = 0; i < NUM_SERVERS; ++i) {
		/* increment clock */
		req.h.time = p2time++;
		if ((servers[i].sfd = createSocket(addrs[i], PORT_STR[EREQ_PORT], 0)) < 0)
			err(1, "createSocket");
		if (writeSocket(servers[i].sfd, req) < 0)
			err(1, "writeSocket");
		if (readSocket(servers[i].sfd, &reply) < sizeof(struct Msg))
			err(1, "recv");
		servers[i].numFiles = reply.data;
		printMsg("reply: ", reply);
	}

	/* probability of (1 / RAND_MAX) of terminating */
	do {
		r = rand();
		i = r % NUM_SERVERS;

		/* Write Request */
		if ((servers[i].sfd = createSocket(addrs[i], PORT_STR[C_WREQ_PORT], 0)) < 0)
			err(1, "createSocket");

		req.data = rand() % servers[i].numFiles;
		req.h.time = p2time++;

		printMsg("req: ", req);
		if (writeSocket(servers[i].sfd, req) < 0)
			err(1, "writeSocket");

		if (readSocket(servers[i].sfd, &reply) < sizeof(struct Msg))
			err(1, "recv");
		printMsg("reply: ", reply);

		fileCount[req.data] += printf("%d %d\n", req.h.id, req.h.time);
		
		sleep(rand() % MAX_SLEEP);
	} while (r > RAND_MAX / 128);

	puts("bytes written");
	for (i = 0; i < NUM_FILES; ++i)
		printf("%ld: %d\n", i, fileCount[i]);

	return 0;
}
