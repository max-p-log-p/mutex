#include <limits.h>
#include <stdint.h>

#define NUM_SERVERS 3
#define NUM_CLIENTS 5
#define NUM_FILES NUM_CLIENTS
#define LEN(array) (sizeof(array) / sizeof(array[0]))

/* prevent overflow */
#if NUM_FILES >= UINT32_MAX
#error "NUM_FILES is too large"
#endif

/* WRITE_REP, WRITE_REQ in [0, NUM_FILES) */
enum MsgData { ENQ_REP = NUM_FILES, ENQ_REQ };
enum Ports { EREQ_PORT, C_WREQ_PORT, S_WREQ_PORT, WREP_PORT, QPORT, NUM_PORTS };

void usage(const char *);
void getHosts(const char *, char [][HOST_NAME_MAX], size_t);

/* ports [0, 1024] require root privileges */
static const char *PORT_STR[NUM_PORTS] = { 
	[EREQ_PORT] = "1025", 
	[C_WREQ_PORT] = "1026", 
	[S_WREQ_PORT] = "1027", 
	[WREP_PORT] = "1028", 
	[QPORT] = "1029", 
};
