#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "net.h"
#include "p2s.h"

#define SERVICE "1025"
#define NODE NULL /* wildcard address */

void usage();
int main(int, char * const *);
void broadcast(struct server [], int8_t [], size_t);

void
usage()
{
	fprintf(stderr, "usage: p2s address0 address1 address2 path\n");
	exit(1);
}

int
main(int argc, char * const *argv)
{
	int32_t afd, sockfd;
	uint64_t i;
	uint8_t j;
	char path[PATH_MAX] = { 0 };
	int8_t buf[sizeof(writeReq)];

	if (argc != ARGS_LEN)
		usage();

	if (strlen(argv[PATH]) + FILENAME_LEN >= sizeof(path))
		errx(1, "path too long");

	for (i = 0; argv[PATH][i] != '\0'; ++i)
		path[i] = argv[PATH][i];

	if (mkdir(path, S_IRWXU) && errno != EEXIST)
		err(1, "mkdir");

	/* Create files */
	path[i++] = '/';
	for (j = 0; j < NUM_FILES; ++j) {
		path[i] = j + '0';
		if ((files[j] = creat(path, S_IRWXU)) < 0)
			err(1, "open");
	}

	for (i = 0; i < NUM_SERVERS; ++i)
		servers[i].addr = argv[i + 1];

	if ((sockfd = createSocket(servers[0].addr, SERVICE, 1)) < 0)
		err(1, "createSocket");

	net_listen(sockfd);

	for (;;) {
		afd = net_accept(sockfd);
		puts("Accept");
		/* sockfd, buf, min, max */
		switch (readSocket(afd, buf, sizeof(enqReq), sizeof(writeReq))) {
		case sizeof(enqReq):
			puts("ENQUIRY");
			if (buf[0] == enqReq[0])
				writeSocket(afd, enqReply, sizeof(enqReply));
			break;
		case sizeof(writeReq):
			puts("WRITE");

			/* check filename */
			if (buf[FILENAME] < 0 || buf[FILENAME] >= NUM_FILES) 
				continue;

			/* broadcast write */
			if (buf[PEERTYPE] == CLIENT) {
				buf[PEERTYPE] = SERVER;
				broadcast(servers, buf, sizeof(buf));
			}

			/* append to file */
			if ((write(files[buf[FILENAME]], buf + CLIENT_ID, WRITE_DATA_LEN)) != WRITE_DATA_LEN)
				err(1, "write");

			/* write reply */
			writeSocket(afd, writeReply, sizeof(writeReply));
			break;
		default:
			printf("buf: %s\n", buf);
			break;
		}

		/* Reset buf */
		for (i = 0; i < sizeof(buf); ++i)
			buf[i] = 0;
	}

	return 0;
}

void
broadcast(struct server servers[], int8_t buf[], size_t len)
{
	uint8_t i;

	i = 1; /* exclude self */
	while (i < NUM_SERVERS) {
		servers[i].fd = createSocket(servers[i].addr, SERVICE, 0);
		if (servers[i].fd < 0)
			continue;
		writeSocket(servers[i++].fd, buf, len);
	}
}
