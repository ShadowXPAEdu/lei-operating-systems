#include "msgdist.h"
#include "msgdist_c.h"

#include <time.h>

int main(int argc, char *argv[], char **envp) {
    srand(time(NULL));
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
					init_client();
//                    char bu[1000];
//                    fgets(bu, 1000, stdin);
                    char op;
                    int r, i;
                    char buf[22];
                    COMMAND s_cmd2;
                    strncpy(s_cmd2.From, cl_cfg.cl_fifo, MAX_FIFO);
                    do {
                        op = getchar();
                        if (op == '1') {
                            r = rand() % 22;
                            for (i = 0; i < r; i++) {
                                buf[i] = 'a' + r;
                            }
                            buf[i] = '\0';
                            win_print(2, 1, 1, buf);
                        } else if (op == '2') {
                            s_cmd2.cmd = CMD_HEARTBEAT;
                            write(cl_cfg.sv_fifo_fd, &s_cmd2, sizeof(COMMAND));
                        } else if (op == '3') {
                            s_cmd2.cmd = CMD_NEWMSG;
                            strncpy(s_cmd2.Body.un_msg.Author, cl_cfg.cl_username, MAX_USER);
                            s_cmd2.Body.un_msg.Duration = (rand() % 101) + 30;
                            strncpy(s_cmd2.Body.un_msg.Title, "Ola", MAX_TPCTTL);
                            strncpy(s_cmd2.Body.un_msg.Topic, "Topic 1", MAX_TPCTTL);
                            strncpy(s_cmd2.Body.un_msg.Body, "Ouidwaohd waion waniodnawion ola oiwabdoiaw ndwaoin ola.", MAX_BODY);
                            write(cl_cfg.sv_fifo_fd, &s_cmd2, sizeof(COMMAND));
                        } else if (op == '4') {
                            s_cmd2.cmd = CMD_NEWMSG;
                            strncpy(s_cmd2.Body.un_msg.Author, cl_cfg.cl_username, MAX_USER);
                            s_cmd2.Body.un_msg.Duration = (rand() % 101) + 30;
                            strncpy(s_cmd2.Body.un_msg.Title, "Ola", MAX_TPCTTL);
                            strncpy(s_cmd2.Body.un_msg.Topic, "Topic ban", MAX_TPCTTL);
                            strncpy(s_cmd2.Body.un_msg.Body, "ola ban 22 adeus haha lala ola.", MAX_BODY);
                            write(cl_cfg.sv_fifo_fd, &s_cmd2, sizeof(COMMAND));
                        } else if (op == '5') {
                            s_cmd2.cmd = CMD_NEWMSG;
                            strncpy(s_cmd2.Body.un_msg.Author, cl_cfg.cl_username, MAX_USER);
                            s_cmd2.Body.un_msg.Duration = (rand() % 101) + 30;
                            strncpy(s_cmd2.Body.un_msg.Title, "Ola 2", MAX_TPCTTL);
                            strncpy(s_cmd2.Body.un_msg.Topic, "Topic 2", MAX_TPCTTL);
                            strncpy(s_cmd2.Body.un_msg.Body, "Ouidwaohd waion waniodnawion ola oiwabdoiaw ndwaoin ola.", MAX_BODY);
                            write(cl_cfg.sv_fifo_fd, &s_cmd2, sizeof(COMMAND));
                        } else if (op == '6') {
                            s_cmd2.cmd = CMD_SUB;
                            s_cmd2.Body.un_tt = 1;
                            write(cl_cfg.sv_fifo_fd, &s_cmd2, sizeof(COMMAND));
                        }
                    } while (op != 'q');

					shutdown();
					pthread_join(tds, NULL);
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
	cl_cfg.cl_unr_msg = 0;
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

void *handle_connection(void *p_cmd) {
	COMMAND r_cmd = *((COMMAND *)p_cmd);
	free(p_cmd);

    char str[30];
    sprintf(str, "Received %s", CMD_ID_to_STR(r_cmd.cmd));
    win_color(1, C_RB, 1);
    win_print(1, 1, 1, str);
    win_color(1, C_RB, 0);
	//printf("Received %s\n", CMD_ID_to_STR(r_cmd.cmd));
	switch (r_cmd.cmd) {
	case CMD_HEARTBEAT:
		f_CMD_HEARTBEAT(r_cmd);
		break;
	case CMD_SDC:
		f_CMD_SDC(r_cmd);
		break;
    case CMD_FDC:
        f_CMD_FDC(r_cmd);
        break;
    case CMD_SUB:
        // Received sub message
        f_CMD_SUB(r_cmd);
        break;
    case CMD_ERR:
        // Show error message
        f_CMD_ERR(r_cmd);
        break;
	case CMD_IGN:
		// Ignore
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

void f_CMD_FDC(COMMAND r_cmd) {
    sv_kick = 1;
    cl_exit(-4);
}

void f_CMD_SUB(COMMAND r_cmd) {
    cl_cfg.cl_unr_msg++;
    char str[5];
    sprintf(str, "%d", cl_cfg.cl_unr_msg);
    win_color(0, C_BW, 1);
    win_print(0, 3, 14, str);
    win_color(0, C_BW, 0);
}

void f_CMD_ERR(COMMAND r_cmd) {
    win_print(1, 3, 1, r_cmd.Body.un_topic);
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

void init_client() {
	init_win = 1;
	initscr();
	start_color();
	cbreak();
	noecho();
	clear();
	init_pair(C_YB, COLOR_YELLOW, COLOR_BLACK);
	init_pair(C_RB, COLOR_RED, COLOR_BLACK);
	init_pair(C_BW, COLOR_BLACK, COLOR_WHITE);

	cl_cfg.win[0].height = WIN_H_H;
	cl_cfg.win[0].width = COLS - WIN_H_W;
	cl_cfg.win[0].w = newwin(WIN_H_H, COLS - WIN_H_W, 0, 0);
	cl_cfg.win[1].height = WIN_H_H;
	cl_cfg.win[1].width = WIN_H_W;
	cl_cfg.win[1].w = newwin(WIN_H_H, WIN_H_W, 0, COLS - WIN_H_W);
    cl_cfg.win[2].height = LINES - WIN_H_H;
    cl_cfg.win[2].width = COLS;
	cl_cfg.win[2].w = newwin(LINES - WIN_H_H, COLS, WIN_H_H, 0);

    for (int i = 0; i < NUM_WIN; i++) {
        win_draw(i);
    }

    char str[10], str2[6], str3[13], str4[5];
    sprintf(str, "Username:");
    sprintf(str2, "FIFO:");
    sprintf(str3, "Unread msgs:");
    sprintf(str4, "%d", cl_cfg.cl_unr_msg);
    win_color(0, C_YB, 1);
    win_print(0, 1, 1, str);
    win_print(0, 2, 1, str2);
    win_print(0, 3, 1, str3);
    win_color(0, C_YB, 0);
    win_print(0, 1, 14, cl_cfg.cl_username);
    win_print(0, 2, 14, cl_cfg.cl_fifo);
    win_print(0, 3, 14, str4);
}

// w = window index --> 0 = top left box, 1 = top right box, 2 = main box
// x = x coordinate inside the window
// y = y coordinate inside the window
// str = string to write on the window
void win_print(int w, int x, int y, char *str) {
    mvwprintw(cl_cfg.win[w].w, x, y, str);
    win_refresh(w);
}

// w = window index
// c_pair = color pair
// on = attribute on or off
void win_color(int w, int c_pair, int on) {
    if (on) {
        wattron(cl_cfg.win[w].w, COLOR_PAIR(c_pair));
    } else {
        wattroff(cl_cfg.win[w].w, COLOR_PAIR(c_pair));
    }
}

void win_erase(int w) {
    werase(cl_cfg.win[w].w);
}

void win_refresh(int w) {
    wrefresh(cl_cfg.win[w].w);
}

void win_draw(int w) {
    win_erase(w);
    box(cl_cfg.win[w].w, 0, 0);
    win_refresh(w);
}

void shutdown() {
	shutdown_init = 1;
	COMMAND cmd;
	if (sv_shutdown == 0) {
		cmd.cmd = CMD_DC;
		strncpy(cmd.From, cl_cfg.cl_fifo, MAX_FIFO);
		write(cl_cfg.sv_fifo_fd, &cmd, sizeof(COMMAND));
		close(cl_cfg.sv_fifo_fd);
	}
	cmd_reader_bool = 1;
	cmd.cmd = CMD_IGN;
	write(cl_cfg.cl_fifo_fd, &cmd, sizeof(COMMAND));
	close(cl_cfg.cl_fifo_fd);
	unlink(cl_cfg.cl_fifo);

	if (init_win == 1) {
        for(int i = 0; i < NUM_WIN; i++) {
            wclear(cl_cfg.win[i].w);
            wrefresh(cl_cfg.win[i].w);
            delwin(cl_cfg.win[i].w);
        }
        clear();
		endwin();
	}

    if (sv_kick) {
        printf("You have been kicked by the server.\n");
    }
}

void cl_exit(int return_val) {
	if (return_val != -3 && shutdown_init == 0) {
		shutdown();
	}
	exit(return_val);
}
