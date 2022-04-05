#include <stdint.h>

struct Msg {
	uint32_t id;
	uint32_t time;
	uint32_t data;
} __attribute__((packed));

extern int32_t readMsg(int32_t, struct Msg *);
extern int32_t writeMsg(int32_t, struct Msg);
extern int32_t createSocket(const char *, const char *, int32_t);
extern int32_t listenSocket(const char *, const char *);
extern int32_t acceptSocket(int32_t);
extern void printMsg(const char *, struct Msg);
