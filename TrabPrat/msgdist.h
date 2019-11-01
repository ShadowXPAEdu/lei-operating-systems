// Disconnect command
#define CMD_DC 100
// Server disconnect command
#define CMD_SDC 150
// Connect command
#define CMD_CON 1
// OK command
#define CMD_OK 200
// Error command
#define CMD_ERR 400
// Force Disconnect command
#define CMD_FDC 600
// Send new message command
#define CMD_NEWMSG 10
// Invalid message command
#define CMD_INVMSG 15
// Get topics command
#define CMD_GETTOPICS 20
// Get titles command
#define CMD_GETTITLES 30
// Get message command
#define CMD_GETMSG 35
// Subscribe command
#define CMD_SUB 50
// Alert subscriber command
#define CMD_ALERTSUB 55
// Unsubscribe command
#define CMD_UNSUB 60
// Heartbeat command
#define CMD_HEARTBEAT 322
// Alive command
#define CMD_ALIVE 223
// Ignore command
#define CMD_IGN 2

#define MAX_BODY 1000
#define MAX_TPCTTL 50
#define MAX_USER 20
#define MAX_FIFO 10

typedef struct {
    char Username[MAX_USER];                    // Username
    char FIFO[MAX_FIFO];                        // User FIFO
} USER;

typedef struct {
    int Duration;                               // Message duration
    char Author[MAX_USER];                      // Message author
    char Topic[MAX_TPCTTL];                     // Message topic
    char Title[MAX_TPCTTL];                     // Message title
    char Body[MAX_BODY];                        // Message body
} MESSAGE;

typedef union {
    USER us;                                    // Sent user
    MESSAGE msg;                                // Sent message
    char Topic[MAX_TPCTTL];                     // Sent topic
    char *TT[MAX_TPCTTL];                       // Sent topics or titles
} CMD_UN;

typedef struct {
    int cmd;                                    // Command id
    char From[MAX_FIFO];                        // FIFO from message
    CMD_UN Body;                                // One of the above types (user, message, topic, topics or titles)
} COMMAND;

char sv_fifo[] = "/tmp/msgsv";

int IsServerRunning(const char *path);
