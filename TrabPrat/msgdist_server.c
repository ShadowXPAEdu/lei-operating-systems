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
    pthread_t td_cmd;

    if (IsServerRunning(sv_fifo) != 0) {
        printf("Looks like the server is already running...\nClosing instance...\n");
        sv_exit(-1);
    } else {
        printerr("Opening server.", PERR_NORM);

        // Read cmd from named pipe

        pthread_create(&td_cmd, NULL, cmd_reader, NULL);

        //

        // Read admin cmds
        printerr("Accepting admin commands.", PERR_NORM);
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

        pthread_cancel(td_cmd);
        sv_exit(0);
    }
}

void *cmd_reader() {
    COMMAND cmd;

    if (mkfifo(sv_fifo, 0666) == 0) {
        printerr("Named pipe created.", PERR_INFO);
        int fd = open(sv_fifo, O_RDWR);
        if (fd == -1) {
            printerr("Error opening named pipe.", PERR_ERROR);
            sv_exit(-1);
        } else {
            printerr("Named pipe opened.", PERR_INFO);

            // do stuff
            read(fd, &cmd, sizeof(COMMAND));
            USER *us;

            us = &(cmd.Body);

            printf("CMD: %d\nFROM: %s\nUSER: %s\nFIFO: %s\n", cmd.cmd, cmd.From, us->Username, us->NamedFIFO);

            close(fd);
            printerr("Named pipe closed.", PERR_INFO);
            unlink(sv_fifo);
            printerr("Named pipe deleted.", PERR_INFO);
        }
    } else {
        printerr("An error occurred trying to open named pipe!\nClosing...", PERR_ERROR);
        sv_exit(-1);
    }
    return NULL;
}

void sv_exit(int return_val) {
    unlink(sv_fifo);
    exit(return_val);
}

void printerr(const char *str, int val) {
    switch (val) {
        case PERR_NORM:
            fprintf(stderr, "[Server] > ");
            break;
        case PERR_ERROR:
            fprintf(stderr, "[ERROR] > ");
            break;
        case PERR_WARNING:
            fprintf(stderr, "[WARNING] > ");
            break;
        case PERR_INFO:
            fprintf(stderr, "[INFO] > ");
            break;
        default:
            break;
    }
    fprintf(stderr, str);
    fprintf(stderr, "\n");
}
