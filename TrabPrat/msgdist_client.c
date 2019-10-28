#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
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
            int sv_fd = open(sv_fifo, O_WRONLY);
            char snp[10];
            sprintf(snp, "%d", getpid());

            COMMAND cl_cmd;
            cl_cmd.cmd = CMD_OK;
            strcpy(cl_cmd.From, "/tmp/");
            strcat(cl_cmd.From, snp);

            USER cl;
            strcpy(cl.Username, argv[1]);
            strcpy(cl.NamedFIFO, cl_cmd.From);

            printf("User: %s\nFifo: %s\n", cl.Username, cl.NamedFIFO);

            cl_cmd.Body = (void*)&cl;

            write(sv_fd, (void*)&cl_cmd, sizeof(cl_cmd));
            char* lala;
            scanf("%s", lala);

            close(sv_fd);
        }
    }

    cl_exit(0);
}

void cl_exit(int return_val) {


    exit(return_val);
}
