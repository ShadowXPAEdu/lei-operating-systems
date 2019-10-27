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
            char *ptr = strtok(adm_cmd, "\n ");
            if (ptr != NULL) {
                ptr = strtok(NULL, "\n ");
                if (ptr == NULL) {
                    if (strcmp(adm_cmd, "filter") == 0) {
                        // filter help
                        printf("Incomplete command.\nTo use '%s', try '%s [on/off]'\n", adm_cmd, adm_cmd);
                    } else if (strcmp(adm_cmd, "users") == 0) {
                        // list users
                    } else if (strcmp(adm_cmd, "topics") == 0) {
                        // list topics
                    } else if (strcmp(adm_cmd, "msg") == 0) {
                        // list messages
                    } else if (strcmp(adm_cmd, "topic") == 0) {
                        // topic help
                        printf("Incomplete command.\nTo use '%s', try '%s [topic_name]'\n", adm_cmd, adm_cmd);
                    } else if (strcmp(adm_cmd, "del") == 0) {
                        // del help
                        printf("Incomplete command.\nTo use '%s', try '%s [message_id]'\n", adm_cmd, adm_cmd);
                    } else if (strcmp(adm_cmd, "kick") == 0) {
                        // kick help
                        printf("Incomplete command.\nTo use '%s', try '%s [user_id]'\n", adm_cmd, adm_cmd);
                    } else if (strcmp(adm_cmd, "prune") == 0) {
                        // delete topics with no messages and cancel subscriptions
                    } else if (strcmp(adm_cmd, "shutdown") == 0) {
                        i = 1;
                    } else if (strcmp(adm_cmd, "help") == 0) {
                        printf("Here's a list of all commands currently available:\n");
                        printf("filter [on/off] - Turns message filtering on or off.\n");
                        printf("users - Lists all users currently connected to the server.\n");
                        printf("topics - Lists all topics currently stored on the server.\n");
                        printf("msg - Lists all messages currently stored on the server.\n");
                        printf("topic [topic_name] - Lists all messages with the topic_name.\n");
                        printf("del [message_id] - Deletes the message associated with the message_id.\n");
                        printf("kick [user_id] - Kicks the user associated with the user_id.\n");
                        printf("prune - Deletes topics with no messages associated and cancel subscriptions to those topics.\n");
                        printf("shutdown - Shuts the server down.\n");
                        printf("help - Shows this.\n");
                    } else {
                        printf("Unknown command...\n");
                    }
                } else {
                    if (strcmp(adm_cmd, "filter") == 0 && strcmp(ptr, "on") == 0) {
                        // turn filter on
                    } else if (strcmp(adm_cmd, "filter") == 0 && strcmp(ptr, "off") == 0) {
                        // turn filter off
                    } else if (strcmp(adm_cmd, "topic") == 0) {
                        // show messages with topic [ptr]
                    } else if (strcmp(adm_cmd, "del") == 0) {
                        // delete message with msg_id [ptr]
                    } else if (strcmp(adm_cmd, "kick") == 0) {
                        // kick user with user_id [ptr]
                    } else {
                        printf("Unknown command...\n");
                    }
                }
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
