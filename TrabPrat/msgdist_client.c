#include "msgdist.h"
#include "msgdist_c.h"

int main(int argc, char* argv[], char** envp) {
    if (argc != 2) {
        printf("You need to specify a username!\nTry %s [username]\n", argv[0]);
        cl_exit(-1);
    } else {
        if (IsServerRunning(sv_fifo) == 0) {
            printf("Server is not online...\nPlease try another time.\n");
            cl_exit(-1);
        } else {
            // Server is online.. try to connect
            cl_cfg.sv_fifo_fd = open(sv_fifo, O_WRONLY);
            char snp[10];
            sprintf(snp, "%d", getpid());

            COMMAND cl_cmd;
            cl_cmd.cmd = CMD_CON;
            strncpy(cl_cmd.From, "/tmp/", MAX_FIFO);
            strncat(cl_cmd.From, snp, MAX_FIFO);
            strncpy(cl_cfg.cl_fifo, cl_cmd.From, strlen(cl_cmd.From));
            CMD_UN un;

            USER cl;
            strncpy(cl.Username, argv[1], MAX_USER);
            strncpy(cl.FIFO, cl_cmd.From, MAX_FIFO);

            printf("User: %s\nFifo: %s\n", cl.Username, cl.FIFO);

            un.un_user = cl;
            cl_cmd.Body = un;


            if (mkfifo(cl_cfg.cl_fifo, 0666) == 0) {
                cl_cfg.cl_fifo_fd = open(cl_cfg.cl_fifo, O_RDWR);
                write(cl_cfg.sv_fifo_fd, &cl_cmd, sizeof(COMMAND));
                COMMAND s_cmd;
                read(cl_cfg.cl_fifo_fd, &s_cmd, sizeof(COMMAND));
                printf("\ns_cmd: %d\nfrom: %s\n", s_cmd.cmd, s_cmd.From);
                unlink(cl_cfg.cl_fifo);
            }


            char lala[1000];
            char *l;
            fgets(lala, 1000, stdin);

            cl_cmd.cmd = CMD_DC;

            write(cl_cfg.sv_fifo_fd, &cl_cmd, sizeof(cl_cmd));

            close(cl_cfg.sv_fifo_fd);
        }
    }

    cl_exit(0);
}

void cl_exit(int return_val) {


    exit(return_val);
}
