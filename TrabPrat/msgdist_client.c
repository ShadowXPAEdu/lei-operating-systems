#include "msgdist.h"
#include "msgdist_c.h"

int main(int argc, char* argv[], char** envp) {
    if (argc != 2) {
        printf("You need to specify a username!\nTry %s [username]\n", argv[0]);
        cl_exit(-3);
    } else {
        // 5 characters in case server needs to add numbers in front
        // Example: username = User / if server has a 'User' it will change to
        // User(0)... User(1)... User(14)... etc
        strncpy(cl_cfg.cl_username, argv[1], (MAX_USER - 5));
        if (IsServerRunning(sv_fifo) == 0) {
            printf("Server is not online...\nPlease try another time.\n");
            cl_exit(-3);
        } else {
            pthread_t tds;
            init_config();
            // Server is online.. try to connect
            if (mkfifo(cl_cfg.cl_fifo, 0666) == 0) {
                cl_cfg.cl_fifo_fd = open(cl_cfg.cl_fifo, O_RDWR);
                if (cl_cfg.cl_fifo_fd == -1) {
                    printf("Error opening named pipe.\n");
                    cl_exit(-1);
                } else {
                    COMMAND cl_cmd;
                    cl_cmd.cmd = CMD_CON;
                    strncpy(cl_cmd.Body.un_user.Username, cl_cfg.cl_username, MAX_USER);
                    strncpy(cl_cmd.Body.un_user.FIFO, cl_cfg.cl_fifo, MAX_FIFO);
                    // Send connection attempt from user
                    write(cl_cfg.sv_fifo_fd, &cl_cmd, sizeof(COMMAND));
                    // Receiving response from server
                    COMMAND s_cmd;
                    read(cl_cfg.cl_fifo_fd, &s_cmd, sizeof(COMMAND));
                    if (s_cmd.cmd == CMD_OK) {
                        if (strcmp(cl_cfg.cl_username, s_cmd.Body.un_user.Username) != 0) {
                            printf("Username has been changed due to username being already in use.\n");
                            strncpy(cl_cfg.cl_username, s_cmd.Body.un_user.Username, MAX_USER);
                            printf("New username: %s.\n", cl_cfg.cl_username);
                        }
                    } else if (s_cmd.cmd == CMD_ERR) {
                        printf("Error: %s", s_cmd.Body.un_topic);
                        cl_exit(-1);
                    } else {
                        printf("\nCommand received: %s\nFrom: %s\n", CMD_ID_to_STR(s_cmd.cmd), s_cmd.From);
                        cl_exit(-1);
                    }

                    if (pthread_create(&tds, NULL, cmd_reader, NULL) != 0) {
                        cl_exit(-2);
                    }

                    // Main thread work ...
                    char lala[1000];
                    fgets(lala, 1000, stdin);


                    pthread_cancel(tds);
                    cl_exit(0);
                }
            } else {
                printf("An error occurred trying to open named pipe!\nClosing...\n");
                cl_exit(-1);
            }
        }
    }

    cl_exit(0);
}

void init_config() {
    sprintf(cl_cfg.cl_fifo, "/tmp/%d", getpid());
    cl_cfg.sv_fifo_fd = open(sv_fifo, O_WRONLY);
    if (cl_cfg.sv_fifo_fd == -1) {
        cl_exit(-3);
    }
    set_signal();
}

void *cmd_reader() {
    pthread_t hc;
    while (cmd_reader_bool == 0) {
        COMMAND cmd;
        cmd.cmd = CMD_IGN;
        read(cl_cfg.cl_fifo_fd, &cmd, sizeof(COMMAND));

        COMMAND *cmd2 = malloc(sizeof(COMMAND));
        *cmd2 = cmd;
        if (pthread_create(&hc, NULL, handle_connection, cmd2) != 0) {
            free(cmd2);
        }
    }

    return NULL;
}

void *handle_connection(void* p_cmd) {
    COMMAND r_cmd = *((COMMAND*)p_cmd);
    free(p_cmd);

    switch (r_cmd.cmd) {
    case CMD_HEARTBEAT:
        printf("Received %s\n", CMD_ID_to_STR(r_cmd.cmd));
        f_CMD_HEARTBEAT(r_cmd);
        break;
    case CMD_SDC:
        printf("Received %s\n", CMD_ID_to_STR(r_cmd.cmd));
        f_CMD_SDC(r_cmd);
    default:
        // Ignore
        break;
    }

}

void f_CMD_HEARTBEAT(COMMAND r_cmd) {
    r_cmd.cmd = CMD_ALIVE;
    strncpy(r_cmd.From, cl_cfg.cl_fifo, MAX_FIFO);
    write(cl_cfg.sv_fifo_fd, &r_cmd, sizeof(COMMAND));
}

void f_CMD_SDC(COMMAND r_cmd) {
    // Server disconnected
    sv_shutdown = 1;
    cl_exit(-2);
}

void set_signal() {
    signal(SIGHUP, received_signal);
    signal(SIGINT, received_signal);
    signal(SIGPIPE, received_signal);
    signal(SIGALRM, received_signal);
    signal(SIGUSR1, received_sigusr1);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGSTOP, received_signal);
    signal(SIGTSTP, received_signal);
    signal(SIGTTIN, received_signal);
    signal(SIGTTOU, received_signal);
}

void received_signal(int i) {
    cl_exit(-2);
}

void received_sigusr1(int i) {
    // do nothing
}

void shutdown() {
    shutdown_init = 1;
    if (sv_shutdown == 0) {
        COMMAND cmd;
        cmd.cmd = CMD_DC;
        strncpy(cmd.From, cl_cfg.cl_fifo, MAX_FIFO);
        write(cl_cfg.sv_fifo_fd, &cmd, sizeof(COMMAND));
        close(cl_cfg.sv_fifo_fd);
    }
    close(cl_cfg.cl_fifo_fd);
    unlink(cl_cfg.cl_fifo);
}

void cl_exit(int return_val) {
    if (return_val != -3 && shutdown_init == 0) {
        shutdown();
    }
    exit(return_val);
}
