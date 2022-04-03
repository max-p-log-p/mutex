#include <stdint.h>

#if NUM_SERVERS > UINTMAX_32 || NUM_CLIENTS > UINTMAX_32
#error "id too small (uint32_t)"
#endif

struct Header {
	uint32_t id;
	uint32_t time;
} __attribute__((packed));

struct Msg {
	struct Header h;
	uint32_t data;
} __attribute__((packed));

extern size_t readSocket(int32_t, struct Msg *);
extern ssize_t writeSocket(int32_t, struct Msg);
extern int32_t createSocket(const char *, const char *, int32_t);
extern int32_t listenSocket(const char *, const char *);
extern int32_t acceptSocket(int32_t);
