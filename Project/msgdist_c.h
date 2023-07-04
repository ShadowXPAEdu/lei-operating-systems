#ifndef MSGDIST_C
#define MSGDIST_C

#include <ncurses.h>

// Ncurses doesn't have KEY_RETURN
#define KEY_RETURN 10
#define KEY_F9 273
#define KEY_F10 274
#define KEY_F11 275
#define KEY_F12 276

// Window Numbers
#define WIN_HUD 0
#define WIN_SVMSG 1
#define WIN_MAIN 2
// Number of windows
#define NUM_WIN 3
// Window header height
#define WIN_H_H 5
// Window header width
#define WIN_H_W 0.6f

// Init colors
// Null
#define C_NONE 0
// Yellow Black
#define C_YB 1
// Red Black
#define C_RB 2
// Black White
#define C_BW 3
// White Blue
#define C_WBL 4

// CL_MENU options
#define CL_MENU_MIN 1
#define CL_MENU_MAX 7
#define CL_MENU_NEWMSG 1
#define CL_MENU_TOPICS 2
#define CL_MENU_TITLES 3
#define CL_MENU_MSG 4
#define CL_MENU_SUB 5
#define CL_MENU_UNSUB 6
#define CL_MENU_EXIT 7

int cmd_reader_bool = 0;
int cmd_heartbeat = 0;
int shutdown_init = 0;
int sv_shutdown = 0;
int init_win = 0;
int sv_kick = 0;

// Mutexes
pthread_mutex_t mtx_tt = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_win = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_hb = PTHREAD_MUTEX_INITIALIZER;
// --

void init_config();
void set_signal();
void received_signal(int i);
void received_sigusr1(int i);
void init_client();
void win_print(int w, int x, int y, char *str);
void win_color(int w, int c_pair, int on);
void win_print2(int w, int y, int x, char *str, int redraw, int c_pair);
void win_erase(int w);
void win_refresh(int w);
void win_draw(int w);
void win_draw2(int w);
void shutdown();
void cl_exit(int return_val);

int cl_menu(int *menu_op, int *ch);
void cl_menu_newmsg();
void cl_menu_topics();
void cl_menu_titles();
void cl_menu_msg();
void cl_menu_sub();
void cl_menu_unsub();
void draw_cl_menu(int *menu_op);
void draw_cl_menu_option(int index, char *str, int attr);
void draw_cl_menu_help(char *str);
void cl_reset_HUD_unrmsg();

void cl_cmd_newmsg(int duration, const char *topic, const char *title, const char *body);
void cl_cmd_gettopics();
void cl_cmd_gettitles();
void cl_cmd_getmsg(int message_id);
void cl_cmd_sub(int topic_id);
void cl_cmd_unsub(int topic_id);

void *cmd_reader();
void *handle_connection(void* p_cmd);
void *heartbeat();

void f_CMD_HEARTBEAT(COMMAND r_cmd);
void f_CMD_SDC(COMMAND r_cmd);
void f_CMD_FDC(COMMAND r_cmd);
void f_CMD_GETMSG(COMMAND r_cmd);
void f_CMD_GETTITLES(COMMAND r_cmd);
void f_CMD_GETTOPICS(COMMAND r_cmd);
void f_CMD_SUB(COMMAND r_cmd);
void f_CMD_ERR(COMMAND r_cmd);
void f_CMD_ALIVE(COMMAND r_cmd);

typedef struct {
    WINDOW *w;
    int height;
    int width;
} CL_WIN;

typedef struct {
    int sv_fifo_fd;
    int sv_offline;

    int cl_fifo_fd;
    int cl_fifo_tt_fd;
    int cl_unr_msg;
    char cl_fifo[MAX_FIFO];
    char cl_fifo_tt[MAX_FIFO];
    char cl_username[MAX_USER];

    CL_WIN win[NUM_WIN];
} CL_CFG;

CL_CFG cl_cfg;

char menu_options[][MAX_TPCTTL] = {
    { " [1] - New message " },
    { " [2] - View topics " },
    { " [3] - View titles " },
    { " [4] - View message " },
    { " [5] - Subscribe to topic " },
    { " [6] - Unsubscribe from topic " },
    { " [7] - Exit " }
};

#endif // MSGDIST_C
