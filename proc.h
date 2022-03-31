#include <stdint.h>

#define NUM_CLIENTS 5
#define NUM_FILES NUM_CLIENTS

uint64_t p2clock;

enum PeerTypes { CLIENT, SERVER };
enum WriteRequest { PEERTYPE = 0, FILENAME, CLIENT_ID, CLOCK };
enum EnquireReply { MAX_FILE };

#define WRITE_DATA_LEN (sizeof(uint8_t) + sizeof(uint64_t))
#define WRITE_REQ_LEN (2 * sizeof(uint8_t) + WRITE_DATA_LEN)

static int8_t enqReq[1] = { 0 };
static int8_t writeReq[WRITE_REQ_LEN] = { 0 };

/* all files are named from [0, NUM_FILES) */
static int8_t enqReply[1] = { NUM_FILES };
static int8_t writeReply[1] = { 1 };
