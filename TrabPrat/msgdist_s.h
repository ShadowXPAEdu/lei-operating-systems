// Printerr commands
#define PERR_NORM 0
#define PERR_ERROR 1
#define PERR_WARNING 2
#define PERR_INFO 3

// Environment defaults
#define MAXMSG 50
#define MAXNOT 5
#define WORDSNOT "bannedwords"

// Initial malloc size
#define INIT_MALLOC_SIZE 10

// CMD_READER Thread controller
int cmd_reader_bool = 0;

// Functions
void sv_exit(int return_val);
int adm_cmd_reader();
void *cmd_reader();
void set_signal();
void received_signal(int i);
void shutdown();
void init_config();
void init_verificador();
void printerr(const char *str, int val);

// Admin command functions
void adm_cmd_help();
void adm_cmd_users();
void adm_cmd_filter(int on);
int adm_cmd_verify(const char* ptr, int sv);
void adm_cmd_cfg();

// Types
typedef struct {
    int id;                         // Message id
    MESSAGE msg;                    // Message
} SV_MSG;

typedef struct {
    int id;                         // Topic id
    char topic[MAX_TPCTTL];         // Topic
} SV_TOPIC;

typedef struct {
    int id;                         // User id
    int user_fd;                    // User file descriptor
    USER user;                      // User
    int sub_size;                   // Malloc size for topics
    SV_TOPIC *topics;               // Subscribed topics
} SV_USER;

typedef struct {
    int sv_fifo_fd;                 // Server FIFO file descriptor
    int sv_verificador_pid;         // Verificador PID
    int sv_verificador_pipes[2];    // Anonymous pipes Read 0/Write 1

    int n_users;                    // Current number of users
    int next_uid;                   // Next user id
    int users_size;                 // Malloc size for users
    SV_USER *users;                 // Users

    int n_msgs;                     // Current number of messages
    int next_mid;                   // Next message id
    SV_MSG *msgs;                   // Messages

    int n_topics;                   // Current number of topics
    int next_tid;                   // Next topic id
    int topic_size;                 // Malloc size for topics
    SV_TOPIC *topics;               // Topics

    int use_filter;                 // Server configuration to use filter

    int maxmsg;                     // Maximum number of messages the server can hold
    int maxnot;                     // Maximum number of banned words
    char *wordsnot;                 // File name/path of the file of banned words
} SV_CFG;

SV_CFG sv_cfg;
