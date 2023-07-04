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
void sv_exit(int return_val);                       // Server exit function (makes sure everything is closed)
int adm_cmd_reader();                               // Administrator command reader function (administrator "menu"/command reader)
void set_signal();                                  // Initialize signals (sets the function the signal should trigger)
void received_signal(int i);                        // Received signal function (closes the server)
void shutdown();                                    // Server shutdown function (closes pipes, warns users, etc.)
void init_config();                                 // Initialize server configuration (environment variables, allocation of memory)
void init_verificador();                            // Initialize 'verificador' program (opens anonymous pipes and connects to the program)
void printerr(const char *str, int val, int adm);   // Server printing messages to administrator

// Thread functions
void *cmd_reader();                                 // Main function for reading server FIFO
void *handle_connection(void *command);             // Handles connections from users
void *msg_duration();                               // Main function for controlling message duration on server
void *heartbeat();                                  // Main function for controlling users who are still connected and not timed out

// Command handling functions
void f_CMD_CON(COMMAND r_cmd);                  // Function to handle a user connection attempt
void f_CMD_ALIVE(COMMAND r_cmd);                // Function to handle a user declaring it's still connected
void f_CMD_HEARTBEAT(COMMAND r_cmd);            // Function to handle a user wanting to know if server is still online
void f_CMD_DC(COMMAND r_cmd);                   // Function to handle a user disconnecting
void f_CMD_NEWMSG(COMMAND r_cmd);               // Function to handle a new message being received
void f_CMD_GETTOPICS(COMMAND r_cmd);            // Function to handle a request for a list of Topics
void f_CMD_GETTITLES(COMMAND r_cmd);            // Function to handle a request for a list of Titles
void f_CMD_GETMSG(COMMAND r_cmd);               // Function to handle a request to view a Message
void f_CMD_SUB(COMMAND r_cmd);                  // Function to handle a subscription request
void f_CMD_UNSUB(COMMAND r_cmd);                // Function to handle a canceled subscription request
void f_CMD_default(COMMAND r_cmd);              // Function to handle any other request (sends error message)

// Admin command functions
void adm_cmd_help();                            // Function to handle an administrator command 'help' (shows command information)
void adm_cmd_users();                           // Function to handle an administrator command 'users' (shows users currently online)
void adm_cmd_topics();                          // Function to handle an administrator command 'topics' (shows topics currently on the server memory)
void adm_cmd_msg();                             // Function to handle an administrator command 'msg' (shows messages currently on the server memory)
void adm_cmd_prune();                           // Function to handle an administrator command 'prune' (deletes any topic which have no messages associated with it, and cancels subscriptions to those topics)
void adm_cmd_filter(int on);                    // Function to handle an administrator command 'filter' (turns bad words filter on or off, or shows the status of the filter)
void adm_cmd_topic(const char *ptr);            // Function to handle an administrator command 'topic' (shows messages with a certain topic)
void adm_cmd_del(const char *ptr);              // Function to handle an administrator command 'del' (deletes a message with a certain ID)
void adm_cmd_kick(const char *ptr);             // Function to handle an administrator command 'kick' (kicks a user with a certain ID)
int adm_cmd_verify(const char *ptr, int sv);    // Function to handle an administrator command 'verify' (verifies weather or not a message passes the filter or not)
void adm_cmd_cfg();                             // Function to handle an administrator command 'cfg' (shows the server configuration)
void adm_cmd_view(const char *ptr);             // Function to handle an administrator command 'view' (shows message with a certain ID)
void adm_cmd_subs(const char *ptr);             // Function to handle an administrator command 'subs' (shows topics subscribed by a user with a certain ID)

// Types
// Server message
typedef struct {
	int id;                         // Message id
	MESSAGE msg;                    // Message
} SV_MSG;

// Server topic
typedef struct {
	int id;                         // Topic id
	char topic[MAX_TPCTTL];         // Topic
} SV_TOPIC;

// Server user
typedef struct {
	int id;                         // User id
	int user_fd;                    // User file descriptor
	USER user;                      // User
	int alive;                      // User alive = 1/dead = 0
	int sub_size;                   // Malloc size for topics
	int *topic_ids;                 // Subscribed topic ids
} SV_USER;

// Server configuration
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
	char wordsnot[PATH_MAX];        // File name/path of the file of banned words
} SV_CFG;

// Variables
SV_CFG sv_cfg;
// Mutexes
pthread_mutex_t mtx_user = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_msg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_topic = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_wait_for_init_td = PTHREAD_MUTEX_INITIALIZER;

// User functions
int count_users();                                      // Counts number of users online (ID != -1)
void resize_users();                                    // Reallocates memory if necessary
void sort_users();                                      // Sorts users putting valid IDs first
int add_user(USER user);                                // Adds a user to the server
void rem_user(const char *FIFO);                        // Removes a user from the server
void rem_user2(int ind);                                // Removes a user from the server based on the index
int get_uindex_by_id(int id);                           // Get user index by ID
int get_uindex_by_FIFO(const char *FIFO);               // Get user index by FIFO
int get_uindex_by_username(const char *username);       // Get user index by Username
int get_uindex_by_fd(int fd);                           // Get user index by File Descriptor
int get_next_u_index();                                 // Get next user index (user whose ID is -1)
int *get_ufd_subbed_to(const char *topic, int *num);    // Get user file descriptors who are subscribed to a topic

// Message functions
void add_msg(MESSAGE msg);                              // Adds a message to the server
int rem_msg(int id);                                    // Removes a message from the server
int get_mindex_by_message(MESSAGE msg);                 // Get message index by Message content (title, author, body, topic)
int get_mindex_by_id(int id);                           // Get message index by ID
int count_msg();                                        // Counts number of messages on server (ID != -1)

// Topic functions
void add_topic(const char *topic);                      // Adds a topic to the server
void rem_topic(int id);                                 // Removes a topic from the server
int get_tindex_by_id(int id);                           // Get topic index by ID
int get_tindex_by_topic_name(const char *t_name);       // Get topic index by Topic Name
void resize_topics();                                   // Reallocates memory if necessary
void sort_topics();                                     // Sorts topics putting valid IDs first
int count_topics();                                     // Counts number of topics on the server (ID != -1)


int count_subs(int uid);                                // Counts subscriptions by User ID (uid)
void resize_subs(int uid);                              // Reallocates memory if necessary for subscriptions of the User (uid)
void sort_subs(int uid);                                // Sorts subscriptions putting valid IDs first for the User (uid)
int is_user_already_subbed(int uid, int tid);           // Checks if user (uid) is already subscribed to a topic (tid)
int get_next_st_index(int uid);                         // Get next subscription topic index for the User (uid)
void subscribe(const char *FIFO, int topic_id);         // Subscribe a user (FIFO) to a topic (topic_id)
void unsubscribe(const char *FIFO, int topic_id);       // Unsubscribe a user (FIFO) from a topic (topic_id)

#endif // MSGDIST_S
