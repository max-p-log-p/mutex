#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "net.h"
#include "p2c.h"

#define SERVICE "1025"

void usage();
int32_t main(int32_t, char * const *);

void
usage()
{
	fprintf(stderr, "usage: p2c address1 address2 address3 id\n");
	exit(1);
}

int32_t
main(int32_t argc, char * const *argv)
{
	uint8_t r;
	uint64_t i;
	int8_t buf[WRITE_REQ_LEN];

	if (argc != ARGS_LEN)
		usage();

	srand(time(NULL));

	writeReq[PEERTYPE] = CLIENT;
	writeReq[CLIENT_ID] = argv[ID][0] - '0';

	for (i = 0; i < 100; ++i) {
		/* Enquiry */
		r = rand() % NUM_SERVERS;
		if ((servers[r].fd = createSocket(argv[r + 1], SERVICE, 0)) < 0)
			err(1, "createSocket");
		if (writeSocket(servers[r].fd, enqReq, sizeof(enqReq)) != sizeof(enqReq))
			err(1, "writeSocket");
		readSocket(servers[r].fd, buf, sizeof(enqReply), sizeof(enqReply));

		maxFile = buf[MAX_FILE];

		/* Write */
		r = rand() % NUM_SERVERS;
		if ((servers[r].fd = createSocket(argv[r + 1], SERVICE, 0)) < 0)
			err(1, "createSocket");
		writeReq[FILENAME] = rand() % maxFile;
		writeReq[CLOCK] = p2clock++;
		if (writeSocket(servers[r].fd, writeReq, sizeof(writeReq)) != sizeof(writeReq))
			err(1, "writeSocket");
		readSocket(servers[r].fd, buf, sizeof(writeReply), sizeof(writeReply));
		fileCount[writeReq[FILENAME]]++;
		
		usleep(rand() % 150);
	}

	for (i = 0; i < NUM_FILES; ++i)
		printf("%ld: %ld\n", i, fileCount[i] * 9);

	return 0;
}
