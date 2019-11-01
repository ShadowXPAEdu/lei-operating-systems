

void cl_exit(int return_val);

typedef struct {
    int cl_fifo_fd;
    char cl_fifo[MAX_FIFO];
} CL_CFG;

CL_CFG cl_cfg;
