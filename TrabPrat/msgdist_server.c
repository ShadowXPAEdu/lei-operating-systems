#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "msgdist.h"
#include "msgdist_s.h"

int main(int argc, char* argv[], char** envp) {
    if (argc == 2)
        if (strcmp(argv[1], "rst_fifo") == 0)
            unlink(sv_fifo);
    if (IsServerRunning(sv_fifo) != 0) {
        printerr("Looks like the server is already running...\nClosing instance...\n", PERR_ERROR);
        sv_exit(-3);
    } else {
        pthread_t td_cmd;
        // Initiate configuration
        init_config();
        // Sets default action when signal is received
        set_signal();

        // Open verificador
        init_verificador();

        printerr("Opening server.", PERR_NORM);

        // Read cmd from named pipe

        pthread_create(&td_cmd, NULL, cmd_reader, NULL);

        // Read admin cmds
        printerr("Accepting admin commands.", PERR_NORM);
        while (adm_cmd_reader() == 0);

        pthread_join(td_cmd, NULL);
        //pthread_cancel(td_cmd);
        sv_exit(0);
    }
}

void init_config() {
    // Initialize filter and environment variables
    sv_cfg.use_filter = 1;
    sv_cfg.maxmsg = MAXMSG;
    sv_cfg.maxnot = MAXNOT;
    sv_cfg.wordsnot = WORDSNOT;
    char* env;
    if ((env = getenv("MAXMSG")) != NULL) {
        sv_cfg.maxmsg = atoi(env);
    }
    if ((env = getenv("MAXNOT")) != NULL) {
        sv_cfg.maxnot = atoi(env);
    }
    if ((env = getenv("WORDSNOT")) != NULL) {
        sv_cfg.wordsnot = env;
    }
    fprintf(stderr, "[Server] MAXMSG: %d\n[Server] MAXNOT: %d\n[Server] WORDSNOT: %s\n", sv_cfg.maxmsg, sv_cfg.maxnot, sv_cfg.wordsnot);
    // Initialize users, messages and topics
    sv_cfg.next_uid = 0;
    sv_cfg.next_mid = 0;
    sv_cfg.next_tid = 0;
    sv_cfg.n_users = 0;
    sv_cfg.n_msgs = 0;
    sv_cfg.n_topics = 0;
    // Allocate memory for messages
    sv_cfg.msgs = malloc(sv_cfg.maxmsg * sizeof(SV_MSG));
    if (sv_cfg.msgs == NULL) {
        printerr("Couldn't allocate memory for Messages.", PERR_ERROR);
        sv_exit(-3);
    }
    // ID = -1 to tell the server that the [i]th index is not being used
    for (int i = 0; i < sv_cfg.maxmsg; i++) {
        sv_cfg.msgs[i].id = -1;
    }
    // Allocate memory for topics
    sv_cfg.topic_size = INIT_MALLOC_SIZE;
    sv_cfg.topics = malloc(sv_cfg.topic_size * sizeof(SV_TOPIC));
    if (sv_cfg.topics == NULL) {
        printerr("Couldn't allocate memory for Topics.", PERR_ERROR);
        free(sv_cfg.msgs);
        sv_exit(-3);
    }
    for (int i = 0; i < sv_cfg.maxmsg; i++) {
        sv_cfg.topics[i].id = -1;
    }
    // Allocate memory for users
    sv_cfg.users_size = INIT_MALLOC_SIZE;
    sv_cfg.users = malloc(sv_cfg.users_size * sizeof(SV_USER));
    if (sv_cfg.users == NULL) {
        printerr("Couldn't allocate memory for Users.", PERR_ERROR);
        free(sv_cfg.msgs);
        free(sv_cfg.topics);
        sv_exit(-3);
    }
    for (int i = 0; i < sv_cfg.users_size; i++) {
        sv_cfg.users[i].id = -1;
        sv_cfg.users[i].sub_size = INIT_MALLOC_SIZE;
        sv_cfg.users[i].topics = NULL;
    }
}

void init_verificador() {
    printerr("Opening verificador.", PERR_NORM);
    int fd1[2];
    int fd2[2];

    printerr("Opening anonymous pipes.", PERR_NORM);
    if (pipe(fd1) < 0) {
        printerr("Error opening anonymous pipe.", PERR_ERROR);
        sv_exit(-1);
    } else {
        if (pipe(fd2) < 0) {
            printerr("Error opening anonymous pipe.", PERR_ERROR);
            sv_exit(-1);
        } else {
            // Anonymous pipes opened
            printerr("Anonymous pipes opened.", PERR_NORM);
            printerr("Creating child process.", PERR_NORM);
            sv_cfg.sv_verificador_pid = fork();
            if (sv_cfg.sv_verificador_pid == -1) {
                printerr("Error creating child process.", PERR_ERROR);
                sv_exit(-1);
            } else if (sv_cfg.sv_verificador_pid == 0) {
                // Child
                // Read / scanf
                close(0);
                dup(fd1[0]);
                close(fd1[0]);
                close(fd1[1]);
                // Write / printf
                close(1);
                dup(fd2[1]);
                close(fd2[1]);
                close(fd2[0]);
                execlp("./verificador", "./verificador", sv_cfg.wordsnot, NULL);
            } else {
                // Parent
                printerr("Child process created.", PERR_NORM);
                close(fd1[0]);
                close(fd2[1]);
                sv_cfg.sv_verificador_pipes[0] = fd2[0];
                sv_cfg.sv_verificador_pipes[1] = fd1[1];
            }
        }
    }
}

int adm_cmd_reader() {
    char adm_cmd[1024];
    printf("Admin > ");
    fgets(adm_cmd, 1024, stdin);
    char *ptr = strtok(adm_cmd, "\n ");
    if (ptr != NULL) {
        ptr = strtok(NULL, "\n");
        if (ptr == NULL) {
            if (strcmp(adm_cmd, "filter") == 0) {
                // filter help
                adm_cmd_filter(2);
                printf("To use '%s', try '%s [on/off]'\n", adm_cmd, adm_cmd);
            } else if (strcmp(adm_cmd, "users") == 0) {
                // list users
                adm_cmd_users();
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
                shutdown();
                return 1;
            } else if (strcmp(adm_cmd, "verify") == 0) {
                // verify help
                printf("Incomplete command.\nTo use '%s', try '%s [text]'\n", adm_cmd, adm_cmd);
            } else if (strcmp(adm_cmd, "help") == 0) {
                // shows help
                adm_cmd_help();
            } else if (strcmp(adm_cmd, "cfg") == 0) {
                adm_cmd_cfg();
            } else {
                printf("Unknown command...\n");
            }
        } else {
            if (strcmp(adm_cmd, "filter") == 0 && strcmp(ptr, "on") == 0) {
                // turn filter on
                adm_cmd_filter(1);
            } else if (strcmp(adm_cmd, "filter") == 0 && strcmp(ptr, "off") == 0) {
                // turn filter off
                adm_cmd_filter(0);
            } else if (strcmp(adm_cmd, "topic") == 0) {
                // show messages with topic [ptr]
            } else if (strcmp(adm_cmd, "del") == 0) {
                // delete message with msg_id [ptr]
            } else if (strcmp(adm_cmd, "kick") == 0) {
                // kick user with user_id [ptr]
            } else if (strcmp(adm_cmd, "verify") == 0) {
                // verifies the message with 'verificador'
                adm_cmd_verify(ptr, 1);
            } else {
                printf("Unknown command...\n");
            }
        }
    }
    return 0;
}

void *cmd_reader() {
    if (mkfifo(sv_fifo, 0666) == 0) {
        printerr("Named pipe created.", PERR_INFO);
        sv_cfg.sv_fifo_fd = open(sv_fifo, O_RDWR);
        if (sv_cfg.sv_fifo_fd == -1) {
            printerr("Error opening named pipe.", PERR_ERROR);
            sv_exit(-1);
        } else {
            printerr("Named pipe opened.", PERR_INFO);

            while (cmd_reader_bool == 0) {
                COMMAND cmd;
                cmd.cmd = CMD_IGN;
                CMD_UN un;
                USER us;
                MESSAGE m;
                read(sv_cfg.sv_fifo_fd, &cmd, sizeof(COMMAND));
                un = (cmd.Body);

                // Take care of cmd
                switch (cmd.cmd) {
                case CMD_CON:
                    // User connecting
                    us = un.us;
                    printf("CMD: %d\nFROM: %s\nUSER: %s\nFIFO: %s\n", cmd.cmd, cmd.From, us.Username, us.FIFO);

                    sv_cfg.users[sv_cfg.next_uid].id = sv_cfg.next_uid;
                    strcpy(sv_cfg.users[sv_cfg.next_uid].user.Username, us.Username);
                    strcpy(sv_cfg.users[sv_cfg.next_uid].user.FIFO, us.FIFO);
                    sv_cfg.next_uid++;
                    sv_cfg.n_users++;
                    break;
                case CMD_DC:
                    // User disconnecting

                    sv_cfg.n_users--;
                    break;
                case CMD_NEWMSG:
                    // User sent new msg
                    m = un.msg;
                    printf("CMD: %d\nFROM: %s\nTopic: %s\nTitle: %s\nBody: '%s'\nDuration: %d\n", cmd.cmd, cmd.From, m.Topic, m.Title, m.Body, m.Duration);
                    // Verify bad words
                    int i = adm_cmd_verify(m.Body, 0);
                    if (i > sv_cfg.maxnot) {
                        // Delete message and warn user
                    } else {
                        // Save message
                    }
                    printf("Bad words: %d\n", i);
                    break;
                case CMD_GETTOPICS:
                    // User asked for topics
                    break;
                case CMD_GETTITLES:
                    // User asked for messages
                    break;
                case CMD_GETMSG:
                    // User asked for specific message
                    break;
                case CMD_SUB:
                    // User subscribed to a topic
                    break;
                case CMD_UNSUB:
                    // User unsubscribed to a topic
                    break;
                case CMD_ALIVE:
                    // User is ALIVE
                    break;
                case CMD_IGN:
                    // Ignore cmd
                    break;
                default:
                    // Send Err cmd
                    break;
                }
            }

            close(sv_cfg.sv_fifo_fd);
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
    if (return_val != -3) {
        shutdown();
    }
    exit(return_val);
}

void shutdown() {
    // TO DO: warn clients

    cmd_reader_bool = 1;
    if (sv_cfg.sv_fifo_fd != -1) {
        COMMAND cmd;
        cmd.cmd = CMD_IGN;
        write(sv_cfg.sv_fifo_fd, &cmd, sizeof(cmd));
    }
    unlink(sv_fifo);
    kill(sv_cfg.sv_verificador_pid, SIGUSR2);
}

void printerr(const char *str, int val) {
    switch (val) {
    case PERR_NORM:
        fprintf(stderr, "[Server] ");
        break;
    case PERR_ERROR:
        fprintf(stderr, "[ERROR] ");
        break;
    case PERR_WARNING:
        fprintf(stderr, "[WARNING] ");
        break;
    case PERR_INFO:
        fprintf(stderr, "[INFO] ");
        break;
    default:
        break;
    }
    fprintf(stderr, "%s", str);
    fprintf(stderr, "\n");
}

//void verificacao() {
//    int fd1[2];
//    int fd2[2];
//
//    if (pipe(fd1) < 0) {
//        printf("Error in pipe");
//        sv_exit(-1);
//    }
//    if (pipe(fd2) < 0) {
//        printf("Error in pipe");
//        sv_exit(-1);
//    }
//
//    if (fork() == 0) {
//
//        //Read / scanf
//        close(0);
//        dup(fd1[0]);
//        close(fd1[1]);
//
//        //write / printf
//        close(1);
//        dup(fd2[1]);
//        close(fd2[0]);
//
//        execlp("./verificador", "./verificador", "la", NULL);
//    } else {
//        close(fd1[0]);
//        close(fd2[1]);
//        char str[20];
//        write(fd1[1], "Olá Pedro!!!\nola adeus 22\n", strlen("Olá Pedro!!!\nola adeus 22\n"));
//        write(fd1[1], "##MSGEND##\n", strlen("##MSGEND##\n"));
//        read(fd2[0], str, 20);
//        printf("%s\n", str);
//    }
//}

void set_signal() {
    signal(SIGHUP, received_signal);
    signal(SIGINT, received_signal);
    signal(SIGPIPE, received_signal);
    signal(SIGALRM, received_signal);
    signal(SIGUSR1, received_signal);
    signal(SIGUSR2, received_signal);
    signal(SIGSTOP, received_signal);
    signal(SIGTSTP, received_signal);
    signal(SIGTTIN, received_signal);
    signal(SIGTTOU, received_signal);
}

void received_signal(int i) {
    sv_exit(i);
}

void adm_cmd_help() {
    printf("Here's a list of all commands currently available:\n");
    printf("\tfilter [on/off] - Turns message filtering on or off.\n");
    printf("\tusers - Lists all users currently connected to the server.\n");
    printf("\ttopics - Lists all topics currently stored on the server.\n");
    printf("\tmsg - Lists all messages currently stored on the server.\n");
    printf("\ttopic [topic_name] - Lists all messages with the topic_name.\n");
    printf("\tdel [message_id] - Deletes the message associated with the message_id.\n");
    printf("\tkick [user_id] - Kicks the user associated with the user_id.\n");
    printf("\tprune - Deletes topics with no messages associated and cancel subscriptions to those topics.\n");
    printf("\tshutdown - Shuts the server down.\n");
    printf("\tverify [message] - Uses the 'verificador' program to verify the message for banned words.\n");
    printf("\tcfg - Shows the server's configuration.\n");
    printf("\thelp - Shows this.\n");
}

void adm_cmd_filter(int on) {
    if (on == 1) {
        if (sv_cfg.use_filter == 1) {
            printf("Filter is already enabled.\n");
        } else {
            sv_cfg.use_filter = 1;
            printf("Enabled word filter.\n");
        }
    } else if (on == 0) {
        if (sv_cfg.use_filter == 0) {
            printf("Filter is already disabled.\n");
        } else {
            sv_cfg.use_filter = 0;
            printf("Disabled word filter.\n");
        }
    } else {
        printf("Filter is ");
        if (sv_cfg.use_filter == 1) {
            printf("enabled.\n");
        } else {
            printf("disabled.\n");
        }
    }
}

void adm_cmd_users() {
    printf("Current users: %d\n", sv_cfg.n_users);
    for (int i = 0; i < sv_cfg.users_size; i++) {
        if (sv_cfg.users[i].id != -1) {
            printf("User:\n\tID: %d\tUsername: %s\tFIFO: %s\n", sv_cfg.users[i].id, sv_cfg.users[i].user.Username, sv_cfg.users[i].user.FIFO);
        }
    }
}

int adm_cmd_verify(const char *ptr, int sv) {
    if (sv_cfg.use_filter == 1) {
        char bwc[20];
        char *bwc2;
        write(sv_cfg.sv_verificador_pipes[1], ptr, strlen(ptr));
        write(sv_cfg.sv_verificador_pipes[1], "\n", strlen("\n"));
        write(sv_cfg.sv_verificador_pipes[1], "##MSGEND##\n", strlen("##MSGEND##\n"));
        read(sv_cfg.sv_verificador_pipes[0], bwc, 20);
        // strtok because 'verificador' sends garbage after the number... for some reason...
        bwc2 = strtok(bwc, "\n");
        int bwci = atoi(bwc2);
        if (sv == 1) {
            printf("[Server] Bad word count: %d\n", bwci);
        } else {
            // Not an adm command
            // return number of bad words so that later on, the server can delete message
            // and warn client if (bwci > sv_cfg.maxnot)
            return bwci;
        }
    } else if (sv_cfg.use_filter == 0 && sv == 1) {
        printf("Word filter is disabled. Please enable word filter with 'filter on'.\n");
    }
    return 0;
}

void adm_cmd_cfg() {
    printf("MAXMSG: %d\n", sv_cfg.maxmsg);
    printf("MAXNOT: %d\n", sv_cfg.maxnot);
    printf("WORDSNOT: %s\n", sv_cfg.wordsnot);
    printf("Current users: %d\n", sv_cfg.n_users);
    printf("Server PID: %d\n", getpid());
    printf("'Verificador' PID: %d\n", sv_cfg.sv_verificador_pid);
}
