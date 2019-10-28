#define PERR_NORM 0
#define PERR_ERROR 1
#define PERR_WARNING 2
#define PERR_INFO 3

int cmd_reader_bool = 0;

void sv_exit(int return_val);
void *cmd_reader();

void printerr(const char *str, int val);
