#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include "msgdist.h"
#include "msgdist_s.h"

int main(int argc, char* argv[], char** envp) {
    char *sv_fifo = "/tmp/msgsv";
    pthread_t td_cmd;

    if (ProgramIsAlreadyRunning(sv_fifo) != 0) {
        printf("Looks like the server is already running...\nClosing instance...\n");
        sv_exit(-1);
    } else {

        // Read cmd from named pipe

        pthread_create(&td_cmd, NULL, cmd_reader, (void*)(sv_fifo));

        //

        // Read admin cmds
        int i = 0;
        while (i == 0) {
            char adm_cmd[32];
            printf("Admin > ");
            fgets(adm_cmd, 32, stdin);
            if (strcmp(adm_cmd, "filter on\n") == 0) {
                // turn on filter
            } else if (strcmp(adm_cmd, "filter off\n") == 0) {
                // turn off filter
            } else if (strcmp(adm_cmd, "users\n") == 0) {
                // list users
            } else if (strcmp(adm_cmd, "topics\n") == 0) {
                // list topics
            } else if (strcmp(adm_cmd, "msg\n") == 0) {
                // list messages
            } else if (strcmp(adm_cmd, "topic ...\n") == 0) {
                // list topic x
            } else if (strcmp(adm_cmd, "del ...\n") == 0) {
                // del message x
            } else if (strcmp(adm_cmd, "kick ...\n") == 0) {
                // kick user x
            } else if (strcmp(adm_cmd, "prune\n") == 0) {
                // delete topics with no messages and cancel subscriptions
            } else if (strcmp(adm_cmd, "shutdown\n") == 0) {
                i = 1;
            } else {
                printf("Unknown or incomplete command...\n");
            }
        }

        pthread_join(td_cmd, NULL);
        sv_exit(0);
    }
}

void *cmd_reader(void *info) {
    char *sv_fifo = (char*)info;
    if (mkfifo(sv_fifo, 0666) == 0) {
        fprintf(stderr, "FIFO Created\n");
        int fd = open(sv_fifo, O_RDWR);
        if (fd == -1) {
            printf("Error opening FIFO!\n");
            sv_exit(-1);
        } else {
            printf("FIFO Opened\n");

            // do stuff

            close(fd);
            printf("FIFO Closed\n");
            unlink(sv_fifo);
            printf("FIFO Deleted\n");
        }
    } else {
        printf("An error occurred trying to open named pipe!\nClosing...\n");
        sv_exit(-1);
    }
    return NULL;
}

void sv_exit(int return_val) {


    exit(return_val);
}

int ProgramIsAlreadyRunning(const char *path) {
    FILE *fptr = fopen(path, "r");

    if (fptr == NULL)
        return 0;

    fclose(fptr);
    return -1;
}
