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

typedef struct {
    char Username[20];
    char NamedFIFO[10];
} USER;

typedef struct {
    int id;
    char Topic[50];
    char Title[50];
    char Body[1000];
    int Duration;
} MESSAGE;

typedef union {
    USER us;
    MESSAGE msg;
} CMD_UN;

typedef struct {
    int cmd;
    char From[10];
    CMD_UN Body;
} COMMAND;

char *sv_fifo = "/tmp/msgsv";
int IsServerRunning(const char *path);
