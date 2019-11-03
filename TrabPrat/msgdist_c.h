#ifndef MSGDIST_C
#define MSGDIST_C

int cmd_reader_bool = 0;
int shutdown_init = 0;
int sv_shutdown = 0;

void init_config();
void set_signal();
void received_signal(int i);
void received_sigusr1(int i);
void shutdown();
void cl_exit(int return_val);

void *cmd_reader();
void *handle_connection(void* p_cmd);

void f_CMD_HEARTBEAT(COMMAND r_cmd);
void f_CMD_SDC(COMMAND r_cmd);

typedef struct {
    int sv_fifo_fd;
    int cl_fifo_fd;
    char cl_fifo[MAX_FIFO];
    char cl_username[MAX_USER];
} CL_CFG;

CL_CFG cl_cfg;

#endif // MSGDIST_C
