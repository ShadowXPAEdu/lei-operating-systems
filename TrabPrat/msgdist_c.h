#ifndef MSGDIST_C
#define MSGDIST_C

#include <ncurses.h>

// Window Numbers
#define WIN_HUD 0
#define WIN_SVMSG 1
#define WIN_MAIN 2
// Number of windows
#define NUM_WIN 3
// Window header height
#define WIN_H_H 5
// Window header width
#define WIN_H_W 60

// Init colors
// Yellow Black
#define C_YB 1
#define C_RB 2
#define C_BW 3

int cmd_reader_bool = 0;
int shutdown_init = 0;
int sv_shutdown = 0;
int init_win = 0;
int sv_kick = 0;

// For GETTITLES and GETTOPICS
int getting_titles = 0;
int getting_topics = 0;
int tt_index;

typedef struct {
    int ID;
    char Name[MAX_TPCTTL];
} TT;

TT *tito;

pthread_mutex_t mtx_tt = PTHREAD_MUTEX_INITIALIZER;
// --

void init_config();
void set_signal();
void received_signal(int i);
void received_sigusr1(int i);
void init_client();
void win_print(int w, int x, int y, char *str);
void win_color(int w, int c_pair, int on);
void win_erase(int w);
void win_refresh(int w);
void win_draw(int w);
void shutdown();
void cl_exit(int return_val);

void *cmd_reader();
void *handle_connection(void* p_cmd);

void f_CMD_HEARTBEAT(COMMAND r_cmd);
void f_CMD_SDC(COMMAND r_cmd);
void f_CMD_FDC(COMMAND r_cmd);
void f_CMD_GETMSG(COMMAND r_cmd);
void f_CMD_GETTITLES(COMMAND r_cmd);
void f_CMD_GETTOPICS(COMMAND r_cmd);
void f_CMD_SUB(COMMAND r_cmd);
void f_CMD_ERR(COMMAND r_cmd);

typedef struct {
    WINDOW *w;
    int height;
    int width;
} CL_WIN;

typedef struct {
    int sv_fifo_fd;
    int cl_fifo_fd;
    int cl_fifo_tt_fd;
    int cl_unr_msg;
    char cl_fifo[MAX_FIFO];
    char cl_fifo_tt[MAX_FIFO];
    char cl_username[MAX_USER];

    CL_WIN win[NUM_WIN];
} CL_CFG;

CL_CFG cl_cfg;

#endif // MSGDIST_C
