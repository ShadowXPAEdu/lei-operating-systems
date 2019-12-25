#ifndef MSGDIST_S
#define MSGDIST_S

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
#define THREADS 3                   // 0 = cmd_reader / 1 = msg_duration / 2 = heartbeat

// Thread bool controller
int cmd_reader_bool = 0;
int msg_duration_bool = 0;
int heartbeat_bool = 0;
int shutdown_init = 0;

// Functions
void sv_exit(int return_val);
int adm_cmd_reader();
void set_signal();
void received_signal(int i);
void test_signal(int i);
void shutdown();
void init_config();
void init_verificador();
void printerr(const char *str, int val, int adm);

// Thread functions
void *cmd_reader();
void *handle_connection(void *command);
void *msg_duration();
void *heartbeat();

// Command handling functions
void f_CMD_CON(COMMAND r_cmd);                  // Done
void f_CMD_ALIVE(COMMAND r_cmd);                // Done
void f_CMD_DC(COMMAND r_cmd);                   // Done
void f_CMD_NEWMSG(COMMAND r_cmd);               // Done
void f_CMD_GETTOPICS(COMMAND r_cmd);            // --
void f_CMD_GETTITLES(COMMAND r_cmd);            // --
void f_CMD_GETMSG(COMMAND r_cmd);               // Done
void f_CMD_SUB(COMMAND r_cmd);                  // Done
void f_CMD_UNSUB(COMMAND r_cmd);                // Done
void f_CMD_default(COMMAND r_cmd);              // Done

// Admin command functions
void adm_cmd_help();                            // Done
void adm_cmd_users();                           // Done
void adm_cmd_topics();                          // Done
void adm_cmd_msg();                             // Done
void adm_cmd_prune();                           // Done
void adm_cmd_filter(int on);                    // Done
void adm_cmd_topic(const char *ptr);            // Done
void adm_cmd_del(const char *ptr);              // Done
void adm_cmd_kick(const char *ptr);             // Done
int adm_cmd_verify(const char *ptr, int sv);    // Done
void adm_cmd_cfg();                             // Done

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
	int alive;                      // User alive = 1/dead = 0
	int sub_size;                   // Malloc size for topics
	int *topic_ids;                 // Subscribed topic ids
} SV_USER;

typedef struct {
	int sv_fifo_fd;                 // Server FIFO file descriptor
	int sv_verificador_pid;         // Verificador PID
	int sv_verificador_pipes[2];    // Anonymous pipes Read 0/Write 1

	int next_uid;                   // Next user id
	int users_size;                 // Malloc size for users
	SV_USER *users;                 // Users

	int n_msgs;                     // Current number of messages
	int next_mid;                   // Next message id
	SV_MSG *msgs;                   // Messages

	int next_tid;                   // Next topic id
	int topic_size;                 // Malloc size for topics
	SV_TOPIC *topics;               // Topics

	int use_filter;                 // Server configuration to use filter

	int maxmsg;                     // Maximum number of messages the server can hold
	int maxnot;                     // Maximum number of banned words
	char *wordsnot;                 // File name/path of the file of banned words
} SV_CFG;

// Variables
SV_CFG sv_cfg;
pthread_mutex_t mtx_user = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_msg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_topic = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_wait_for_init_td = PTHREAD_MUTEX_INITIALIZER;

// User functions
int count_users();
void resize_users();                // Reallocates memory
void sort_users();                  // Puts users with valid ID's in front
int add_user(USER user);
void rem_user(const char *FIFO);
void rem_user2(int ind);
int get_uindex_by_id(int id);
int get_uindex_by_FIFO(const char *FIFO);
int get_uindex_by_username(const char *username);
int get_uindex_by_fd(int fd);
int get_next_u_index();
int *get_ufd_subbed_to(const char *topic, int *num);

// Message functions
void add_msg(MESSAGE msg);
int rem_msg(int id);
int get_mindex_by_message(MESSAGE msg);
int get_mindex_by_id(int id);
int count_msg();

// Topic functions
void add_topic(const char *topic);
void rem_topic(int id);
int get_tindex_by_id(int id);
int get_tindex_by_topic_name(const char *t_name);
void resize_topics();
void sort_topics();
int count_topics();

// Probably to delete
//SV_USER *get_user_by_id(int id);
//SV_USER *get_user_by_FIFO(const char *FIFO);
//SV_USER *get_user_by_username(const char *username);
//

// To test
int count_subs(int uid);
void resize_subs(int uid);
void sort_subs(int uid);
int is_user_already_subbed(int uid, int tid);
int get_next_st_index(int uid);

// Not fully implemented
void subscribe(const char *FIFO, int topic_id);
void unsubscribe(const char *FIFO, int topic_id);
// Unimplemented

#endif // MSGDIST_S
