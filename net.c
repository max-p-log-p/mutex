#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "net.h"

size_t
readSocket(int32_t sockfd, struct Msg *msg)
{
	ssize_t nr;
	size_t i;
	uint8_t buf[sizeof(struct Msg)];
	struct Msg *recvd;

	for (i = 0; i < sizeof(buf); i += nr) {
		if ((nr = recv(sockfd, buf + i, sizeof(buf) - i, 0)) > 0)
			continue;

		switch (errno) {
		case EAGAIN:
#if EWOULDBLOCK != EAGAIN
		case EWOULDBLOCK:
#endif
		case EINTR:
			warn("recv failed");
			nr = 0;
			break;
		default:
			return i;
		}
	}

	recvd = (struct Msg *)buf;
	/*
	msg->h.id = ntohl(recvd->h.id);
	msg->data = ntohl(recvd->data);
	msg->h.time = ntohl(recvd->h.time);
	*/

	msg->h.id = recvd->h.id;
	msg->data = recvd->data;
	msg->h.time = recvd->h.time;

	printf("readSocket %d %d %d\n", recvd->h.id, recvd->data, recvd->h.time);

	return i;
}

ssize_t
writeSocket(int sockfd, struct Msg req)
{
	/*
	uint32_t buf[sizeof(struct Msg)/ sizeof(uint32_t)];

	buf[0] = htonl(req.h.id);
	buf[1] = htonl(req.h.time);
	buf[2] = htonl(req.data);
	*/
	return send(sockfd, &req, sizeof(req), 0);
}

/* returns socket file descriptor */
int
createSocket(const char *node, const char *service, int32_t lflag)
{
	int ecode, optval, serrno, sockfd;
	struct addrinfo *cur, hints, *res;

	memset(&hints, 0, sizeof(struct addrinfo));

	if (lflag) {
		hints.ai_flags = AI_PASSIVE;
		/* default to more common ipv4 if wildcard address */
		hints.ai_family = (node == NULL ? AF_INET : AF_UNSPEC);
	} else
		hints.ai_family = AF_UNSPEC;

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if ((ecode = getaddrinfo(node, service, &hints, &res)))
		errx(1, "getaddrinfo: %s", gai_strerror(ecode));

	for (cur = res; cur; cur = cur->ai_next) {
		if ((sockfd = socket(cur->ai_family, cur->ai_socktype,
		    cur->ai_protocol)) == -1)
			continue;

		optval = 1;

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, 
		    sizeof(optval)))
			err(1, "setsockopt");

		if (lflag) {
			if (bind(sockfd, cur->ai_addr, cur->ai_addrlen) == 0)
				break;
		} else {
			if (connect(sockfd, cur->ai_addr, cur->ai_addrlen) == 0)
				break;
		}

		serrno = errno; /* errno can be modified by close */
		if (close(sockfd))
			warn("can't close socket %d", sockfd);
		errno = serrno;
		sockfd = -1;
	}

	freeaddrinfo(res);
	return sockfd;
}

int32_t
listenSocket(const char *node, const char *service)
{
	int32_t sockfd;

	if ((sockfd = createSocket(node, service, 1)) < 0)
		err(1, "createSocket");

	if (listen(sockfd, SOMAXCONN))
		err(1, "listen");

	return sockfd;
}

/* return the accepted file descriptor */
int32_t
acceptSocket(int32_t sockfd)
{
	int32_t afd;
	socklen_t addrlen;
	struct sockaddr_storage addr;

	addrlen = sizeof(addr);

	while ((afd = accept(sockfd, (struct sockaddr *)&addr, &addrlen)) < 0) {
		switch (errno) {
		case EAGAIN:
#if EWOULDBLOCK != EAGAIN
		case EWOULDBLOCK:
#endif
		case ECONNABORTED:
		case EINTR:
			warn("accept failed");
			break;
		default:
			return -1;
		}
	}

	return afd;
}
