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
			if (mkfifo(cl_cfg.cl_fifo, 0666) == 0 && mkfifo(cl_cfg.cl_fifo_tt, 0666) == 0) {
				cl_cfg.cl_fifo_fd = open(cl_cfg.cl_fifo, O_RDWR);
				cl_cfg.cl_fifo_tt_fd = open(cl_cfg.cl_fifo_tt, O_RDWR);
				if (cl_cfg.cl_fifo_fd == -1 || cl_cfg.cl_fifo_tt_fd == -1) {
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
							printf("Press enter key to continue...");
							getchar();
							char b[1];
							fgets(b, 1, stdin);
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
					int menu_op = CL_MENU_MIN;
					int ch;
					draw_cl_menu(&menu_op);
					while(cl_menu(&menu_op, &ch));

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

int cl_menu(int *menu_op, int *ch) {
	(*ch) = wgetch(cl_cfg.win[WIN_MAIN].w);

	switch ((*ch)) {
		case KEY_UP:
			(*menu_op)--;
			(*menu_op) = (*menu_op) == CL_MENU_MIN - 1 ? CL_MENU_MIN : (*menu_op);
			draw_cl_menu_option((*menu_op) + 1, menu_options[(*menu_op)], FALSE);
			draw_cl_menu_option((*menu_op), menu_options[(*menu_op) - 1], TRUE);
			break;
		case KEY_DOWN:
			(*menu_op)++;
			(*menu_op) = (*menu_op) == CL_MENU_MAX + 1 ? CL_MENU_MAX : (*menu_op);
			draw_cl_menu_option((*menu_op) - 1, menu_options[(*menu_op) - 2], FALSE);
			draw_cl_menu_option((*menu_op), menu_options[(*menu_op) - 1], TRUE);
			break;
		case KEY_RETURN:
		case KEY_F10:
            win_draw2(WIN_SVMSG);
			switch ((*menu_op)) {
				case CL_MENU_NEWMSG:
					cl_menu_newmsg();
					draw_cl_menu(menu_op);
					break;
				case CL_MENU_TOPICS:
					cl_menu_topics();
					draw_cl_menu(menu_op);
					break;
				case CL_MENU_TITLES:
					cl_menu_titles();
					draw_cl_menu(menu_op);
					break;
				case CL_MENU_MSG:
					cl_menu_msg();
					draw_cl_menu(menu_op);
					break;
				case CL_MENU_SUB:
					cl_menu_sub();
					draw_cl_menu(menu_op);
					break;
				case CL_MENU_UNSUB:
					cl_menu_unsub();
					draw_cl_menu(menu_op);
					break;
				case CL_MENU_EXIT:
					return 0;
				default:
					break;
			}
			break;
		default:
			if ((*ch) >= (CL_MENU_MIN + 48) && (*ch) <= (CL_MENU_MAX + 48)) {
				draw_cl_menu_option((*menu_op), menu_options[(*menu_op) - 1], FALSE);
				(*menu_op) = ((*ch) - 48);
				draw_cl_menu_option((*menu_op), menu_options[(*menu_op) - 1], TRUE);
			}
			break;
	}

	return 1;
}

void draw_cl_menu(int *menu_op) {
	win_draw2(WIN_MAIN);
	for (int i = 0; i < CL_MENU_MAX; i++) {
		draw_cl_menu_option(i + 1, menu_options[i], ((*menu_op) == i + 1));
	}
	draw_cl_menu_help(" UP - Menu up    DOWN - Menu down    F10/RETURN/[1-7] - Menu select ");
}

void draw_cl_menu_help(char *str) {
	win_print2(WIN_MAIN, cl_cfg.win[WIN_MAIN].height - 1, 1, str, FALSE, C_BW);
}

void draw_cl_menu_option(int index, char *str, int attr) {
	if (attr) {
		win_print2(WIN_MAIN, (index - 1) * 2 + 2, 2, str, FALSE, C_WBL);
	} else {
		win_print2(WIN_MAIN, (index - 1) * 2 + 2, 2, str, FALSE, C_NONE);
	}
}

void cl_menu_newmsg() {
    WINDOW *w = cl_cfg.win[WIN_MAIN].w;
    int x, y;
    int *bod_offset_x;
	int ask_index, dur_index, top_index, tit_index, bod_index, bod_offset, duration;
	char scr_buffer[2];
	char topic[MAX_TPCTTL], title[MAX_TPCTTL], body[MAX_BODY], dur[MAX_NUM];
    int bod_offset_size = cl_cfg.win[WIN_MAIN].height - 2 - 2 - 3;
    bod_offset_x = calloc(bod_offset_size, sizeof(int));
    if (bod_offset_x == NULL)
        return;
	ask_index = dur_index = top_index = tit_index = bod_index = bod_offset = duration = 0;
    dur[0] = '\0';
    dur[MAX_NUM - 1] = '\0';
	topic[0] = '\0';
    topic[MAX_TPCTTL - 1] = '\0';
	title[0] = '\0';
	title[MAX_TPCTTL - 1] = '\0';
	body[0] = '\0';
    body[MAX_BODY - 1] = '\0';
	win_draw2(WIN_MAIN);
	draw_cl_menu_help(" F9/END - Go back    F10 - Send    F11 - Previous    F12 - Next ");
	// Print "Duration: "
	win_print2(WIN_MAIN, 1, 1, "Duration: ", FALSE, C_YB);
	// Print "Topic: "
	win_print2(WIN_MAIN, 2, 1, "Topic: ", FALSE, C_YB);
	// Print "Title: "
	win_print2(WIN_MAIN, 3, 1, "Title: ", FALSE, C_YB);
	// Print "Body: "
	win_print2(WIN_MAIN, 4, 1, "Body: ", FALSE, C_YB);
	x = 11;
    y = 1;
    wmove(w, y, x);
	int ch;
	do {
		ch = wgetch(cl_cfg.win[WIN_MAIN].w);
		switch (ch) {
			case KEY_F9:
			case KEY_END:
			    free(bod_offset_x);
				return;
			case KEY_F10:
				// Check every field and then send
				dur[dur_index] = '\0';
                duration = atoi(dur);
                topic[top_index] = '\0';
                title[tit_index] = '\0';
                body[bod_index] = '\0';
				if (duration == 0 || strnlen(topic, MAX_TPCTTL) == 0 || strnlen(title, MAX_TPCTTL) == 0 || strnlen(body, MAX_BODY) == 0) {
					// One field is empty...
					// Ignore
				} else {
                    if (duration > INT_MAX)
                        duration = INT_MAX;
					cl_cmd_newmsg(duration, topic, title, body);
                    free(bod_offset_x);
					return;
				}
				break;
			case KEY_F11:
				// Go to previous
				if (ask_index > 0) {
					ask_index--;
                    x = (ask_index == 0) ? dur_index + 11 : (ask_index == 1) ? top_index + 11 : (ask_index == 2) ? tit_index + 11 : bod_offset_x[bod_offset] + 1;
                    y = (ask_index == 3) ? ask_index + bod_offset + 2 : ask_index + 1;
                    wmove(w, y, x);
				}
				break;
			case KEY_F12:
				// Go to next
				if (ask_index < 3) {
					ask_index++;
                    x = (ask_index == 0) ? dur_index + 11 : (ask_index == 1) ? top_index + 11 : (ask_index == 2) ? tit_index + 11 : bod_offset_x[bod_offset] + 1;
                    y = (ask_index == 3) ? ask_index + bod_offset + 2 : ask_index + 1;
                    wmove(w, y, x);
				}
				break;
			default:
				switch (ask_index) {
					case 0: {
						// Duration
                        if (dur_index < (MAX_NUM - 1) && ((dur_index + 1) < cl_cfg.win[WIN_MAIN].width - 1) && ch >= '0' && ch <= '9') {
                            // Print to the screen
                            snprintf(scr_buffer, 2, "%c", ch);
                            win_print2(WIN_MAIN, ask_index + 1, dur_index + 11, scr_buffer, FALSE, C_NONE);
                            // If it's a number add it
                            dur[dur_index] = (char)ch;
                            dur_index++;
                        } else if (dur_index > 0 && ch == KEY_BACKSPACE) {
                            // If it's a backspace delete the last number
                            dur_index--;
                            // Print to the screen
                            win_print2(WIN_MAIN, ask_index + 1, dur_index + 11, " ", FALSE, C_NONE);
                            wmove(w, ask_index + 1, dur_index + 11);
                            dur[dur_index] = '\0';
                        }
						break;
					}
					case 1: {
						// Topic
                        if (top_index < (MAX_TPCTTL - 1) && ((top_index + 1) < cl_cfg.win[WIN_MAIN].width - 1) && ch >= 32 && ch <= 126) {
                            snprintf(scr_buffer, 2, "%c", ch);
                            win_print2(WIN_MAIN, ask_index + 1, top_index + 11, scr_buffer, FALSE, C_NONE);
                            topic[top_index] = (char)ch;
                            top_index++;
                        } else if (top_index > 0 && ch == KEY_BACKSPACE) {
                            top_index--;
                            win_print2(WIN_MAIN, ask_index + 1, top_index + 11, " ", FALSE, C_NONE);
                            wmove(w, ask_index + 1, top_index + 11);
                            topic[top_index] = '\0';
                        }
						break;
					}
					case 2: {
						// Title
                        if (tit_index < (MAX_TPCTTL - 1) && ((tit_index + 1) < cl_cfg.win[WIN_MAIN].width - 1) && ch >= 32 && ch <= 126) {
                            snprintf(scr_buffer, 2, "%c", ch);
                            win_print2(WIN_MAIN, ask_index + 1, tit_index + 11, scr_buffer, FALSE, C_NONE);
                            title[tit_index] = (char)ch;
                            tit_index++;
                        } else if (tit_index > 0 && ch == KEY_BACKSPACE) {
                            tit_index--;
                            win_print2(WIN_MAIN, ask_index + 1, tit_index + 11, " ", FALSE, C_NONE);
                            wmove(w, ask_index + 1, tit_index + 11);
                            title[tit_index] = '\0';
                        }
						break;
					}
					case 3: {
						// Body
                        if (bod_index < (MAX_BODY - 1) && ((bod_offset_x[bod_offset] + 1) < cl_cfg.win[WIN_MAIN].width - 2)  && ch >= 32 && ch <= 126) {
                            snprintf(scr_buffer, 2, "%c", ch);
                            win_print2(WIN_MAIN, ask_index + bod_offset + 2, bod_offset_x[bod_offset] + 1, scr_buffer, FALSE, C_NONE);
                            body[bod_index] = (char)ch;
                            bod_index++;
                            bod_offset_x[bod_offset]++;
                        } else if (bod_index > 0 && ch == KEY_BACKSPACE) {
                            bod_index--;
                            bod_offset_x[bod_offset]--;
                            if (body[bod_index] == '\n') {
                                bod_offset_x[bod_offset] = 0;
                                bod_offset--;
                            }
                            win_print2(WIN_MAIN, ask_index + bod_offset + 2, bod_offset_x[bod_offset] + 1, " ", FALSE, C_NONE);
                            wmove(w, ask_index + bod_offset + 2, bod_offset_x[bod_offset] + 1);
                            body[bod_index] = '\0';
                        } else if (bod_index < (MAX_BODY - 1) && ((ask_index + bod_offset + 2) < cl_cfg.win[WIN_MAIN].height - 2) && ch == KEY_RETURN) {
                            // New line
                            bod_offset++;
                            wmove(w, ask_index + bod_offset + 2, bod_offset_x[bod_offset] + 1);
                            body[bod_index] = '\n';
                            bod_index++;
                        }
						break;
					}
				}
				break;
		}
	} while (1);
}

void cl_menu_topics() {
	win_draw2(WIN_MAIN);
	// Show topics
	cl_cmd_gettopics();
	// Draw help
	draw_cl_menu_help(" F9/RETURN/END - Go back ");
	// Wait for user to press key
	int ch;
	do {
		ch = wgetch(cl_cfg.win[WIN_MAIN].w);
		switch (ch) {
			case KEY_F9:
			case KEY_RETURN:
			case KEY_END:
				return;
			default:
				break;
		}
	} while (1);
}

void cl_menu_titles() {
	win_draw2(WIN_MAIN);
	// Show titles
	cl_cmd_gettitles();
	// Reset unread messages counter
	cl_reset_HUD_unrmsg();
	// Draw help
	draw_cl_menu_help(" F9/RETURN/END - Go back ");
	// Wait for user to press key
	int ch;
	do {
		ch = wgetch(cl_cfg.win[WIN_MAIN].w);
		switch (ch) {
			case KEY_RETURN:
			case KEY_F9:
			case KEY_END:
				return;
			default:
				break;
		}
	} while (1);
}

void cl_menu_msg() {
	win_draw2(WIN_MAIN);
	// Draw help
	draw_cl_menu_help(" F9/END - Go back    F10/ENTER - Continue ");
	// Ask for Message ID
	int y, x, h_txt, bo = 1;
	y = x = 1;
	h_txt = 22;
	char c_h_txt[] = "Insert a Message ID: ";
	win_print2(WIN_MAIN, y, x, c_h_txt, FALSE, C_YB);
	int ch, index = 0;
	char *u_txt = malloc(sizeof(char) * 12);
	char buffer[2];
	if (u_txt != NULL) {
		do {
			ch = wgetch(cl_cfg.win[WIN_MAIN].w);
			switch (ch) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (bo && index < 11) {
						snprintf(buffer, 2, "%d", (ch - 48));
						win_print2(WIN_MAIN, y, h_txt + index, buffer, FALSE, C_NONE);
						u_txt[index] = (char)ch;
						index++;
					}
					break;
				case KEY_BACKSPACE:
					if (bo && index > 0) {
						index--;
						win_print2(WIN_MAIN, y, h_txt + index, " ", FALSE, C_NONE);
						wmove(cl_cfg.win[WIN_MAIN].w, y, h_txt + index);
						u_txt[index] = '\0';
					}
					break;
				case KEY_F10:
				case KEY_RETURN:
					if (bo && index > 0) {
						win_draw2(WIN_MAIN);
						draw_cl_menu_help(" F9/END - Go back ");
						// Show message
						cl_cmd_getmsg(atoi(u_txt));
						// Wait for user to press key
						bo = 0;
					}
					break;
				case KEY_F9:
				case KEY_END:
					free(u_txt);
					return;
				default:
					break;
			}
		} while (1);
	}
}

void cl_menu_sub() {
	win_draw2(WIN_MAIN);
	// Draw help
	draw_cl_menu_help(" F9/END - Go back    F10/ENTER - Continue ");
	// Ask for Topic ID
	int y, x, h_txt;
	y = x = 1;
	h_txt = 20;
	char c_h_txt[] = "Insert a Topic ID: ";
	win_print2(WIN_MAIN, y, x, c_h_txt, FALSE, C_YB);
	int ch, index = 0;
	char *u_txt = malloc(sizeof(char) * 12);
	char buffer[2];
	if (u_txt != NULL) {
		do {
			ch = wgetch(cl_cfg.win[WIN_MAIN].w);
			switch (ch) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (index < 11) {
						snprintf(buffer, 2, "%d", (ch - 48));
						win_print2(WIN_MAIN, y, h_txt + index, buffer, FALSE, C_NONE);
						u_txt[index] = (char)ch;
						index++;
					}
					break;
				case KEY_BACKSPACE:
					if (index > 0) {
						index--;
						win_print2(WIN_MAIN, y, h_txt + index, " ", FALSE, C_NONE);
						wmove(cl_cfg.win[WIN_MAIN].w, y, h_txt + index);
						u_txt[index] = '\0';
					}
					break;
				case KEY_F10:
				case KEY_RETURN:
					if (index > 0) {
						cl_cmd_sub(atoi(u_txt));
						free(u_txt);
						return;
					}
					break;
				case KEY_F9:
				case KEY_END:
					free(u_txt);
					return;
				default:
					break;
			}
		} while (1);
	}
}

void cl_menu_unsub() {
	win_draw2(WIN_MAIN);
	// Draw help
	draw_cl_menu_help(" F9/END - Go back    F10/ENTER - Continue ");
	// Ask for Topic ID
	int y, x, h_txt;
	y = x = 1;
	h_txt = 20;
	char c_h_txt[] = "Insert a Topic ID: ";
	win_print2(WIN_MAIN, y, x, c_h_txt, FALSE, C_YB);
	int ch, index = 0;
	char *u_txt = malloc(sizeof(char) * 12);
	char buffer[2];
	if (u_txt != NULL) {
		do {
			ch = wgetch(cl_cfg.win[WIN_MAIN].w);
			switch (ch) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (index < 11) {
						snprintf(buffer, 2, "%d", (ch - 48));
						win_print2(WIN_MAIN, y, h_txt + index, buffer, FALSE, C_NONE);
						u_txt[index] = (char)ch;
						index++;
					}
					break;
				case KEY_BACKSPACE:
					if (index > 0) {
						index--;
						win_print2(WIN_MAIN, y, h_txt + index, " ", FALSE, C_NONE);
						wmove(cl_cfg.win[WIN_MAIN].w, y, h_txt + index);
						u_txt[index] = '\0';
					}
					break;
				case KEY_F10:
				case KEY_RETURN:
					if (index > 0) {
						cl_cmd_unsub(atoi(u_txt));
						free(u_txt);
						return;
					}
					break;
				case KEY_F9:
				case KEY_END:
					free(u_txt);
					return;
				default:
					break;
			}
		} while (1);
	}
}

void cl_reset_HUD_unrmsg() {
	cl_cfg.cl_unr_msg = 0;
	char buf[11];
	snprintf(buf, 11, "%d", cl_cfg.cl_unr_msg);
	win_print2(WIN_HUD, 3, 14, buf, FALSE, C_NONE);
	for (int i = 15; i < cl_cfg.win[WIN_HUD].width - 1; i++) {
		win_print2(WIN_HUD, 3, i, " ", FALSE, C_NONE);
	}
}

void cl_cmd_newmsg(int duration, const char *topic, const char *title, const char *body) {
	COMMAND s_cmd;
	// Basic COMMAND info
	s_cmd.cmd = CMD_NEWMSG;
	snprintf(s_cmd.From, MAX_FIFO, "%s", cl_cfg.cl_fifo);
	// COMMAND info
	s_cmd.Body.un_msg.Duration = duration;
	snprintf(s_cmd.Body.un_msg.Author, MAX_USER, "%s", cl_cfg.cl_username);
	snprintf(s_cmd.Body.un_msg.Topic, MAX_TPCTTL, "%s", topic);
	snprintf(s_cmd.Body.un_msg.Title, MAX_TPCTTL, "%s", title);
	snprintf(s_cmd.Body.un_msg.Body, MAX_BODY, "%s", body);
	// Send new message
	write(cl_cfg.sv_fifo_fd, &s_cmd, sizeof(COMMAND));
}

void cl_cmd_gettopics() {
	COMMAND s_cmd;
	// Basic COMMAND info
	s_cmd.cmd = CMD_GETTOPICS;
	snprintf(s_cmd.From, MAX_FIFO, "%s", cl_cfg.cl_fifo);
	// Send get topics
	write(cl_cfg.sv_fifo_fd, &s_cmd, sizeof(COMMAND));
}

void cl_cmd_gettitles() {
	COMMAND s_cmd;
	// Basic COMMAND info
	s_cmd.cmd = CMD_GETTITLES;
	snprintf(s_cmd.From, MAX_FIFO, "%s", cl_cfg.cl_fifo);
	// Send get topics
	write(cl_cfg.sv_fifo_fd, &s_cmd, sizeof(COMMAND));
}

void cl_cmd_getmsg(int message_id) {
	COMMAND s_cmd;
	// Basic COMMAND info
	s_cmd.cmd = CMD_GETMSG;
	snprintf(s_cmd.From, MAX_FIFO, "%s", cl_cfg.cl_fifo);
	// COMMAND info
	s_cmd.Body.un_tt = message_id;
	// Send get topics
	write(cl_cfg.sv_fifo_fd, &s_cmd, sizeof(COMMAND));
}

void cl_cmd_sub(int topic_id) {
	COMMAND s_cmd;
	// Basic COMMAND info
	s_cmd.cmd = CMD_SUB;
	snprintf(s_cmd.From, MAX_FIFO, "%s", cl_cfg.cl_fifo);
	// COMMAND info
	s_cmd.Body.un_tt = topic_id;
	// Send get topics
	write(cl_cfg.sv_fifo_fd, &s_cmd, sizeof(COMMAND));
}

void cl_cmd_unsub(int topic_id) {
	COMMAND s_cmd;
	// Basic COMMAND info
	s_cmd.cmd = CMD_UNSUB;
	snprintf(s_cmd.From, MAX_FIFO, "%s", cl_cfg.cl_fifo);
	// COMMAND info
	s_cmd.Body.un_tt = topic_id;
	// Send get topics
	write(cl_cfg.sv_fifo_fd, &s_cmd, sizeof(COMMAND));
}

void init_config() {
	snprintf(cl_cfg.cl_fifo, MAX_FIFO, "/tmp/%d", getpid());
	snprintf(cl_cfg.cl_fifo_tt, MAX_FIFO + 3, "%s_tt", cl_cfg.cl_fifo);
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

	if (r_cmd.cmd != CMD_HEARTBEAT) {
		char str[31];
		snprintf(str, 31, "Received %s", CMD_ID_to_STR(r_cmd.cmd));
		win_print2(WIN_SVMSG, 1, 1, str, TRUE, C_RB);
	}
	switch (r_cmd.cmd) {
		case CMD_HEARTBEAT:
			f_CMD_HEARTBEAT(r_cmd);
			break;
		case CMD_SDC:
			// Server disconnect
			f_CMD_SDC(r_cmd);
			break;
		case CMD_FDC:
			// Force disconnect
			f_CMD_FDC(r_cmd);
			break;
		case CMD_GETMSG:
			// Received message
			f_CMD_GETMSG(r_cmd);
			break;
		case CMD_GETTITLES:
			// Received Titles
			pthread_mutex_lock(&mtx_tt);
			f_CMD_GETTITLES(r_cmd);
			pthread_mutex_unlock(&mtx_tt);
			break;
		case CMD_GETTOPICS:
			// Received Topics
			pthread_mutex_lock(&mtx_tt);
			f_CMD_GETTOPICS(r_cmd);
			pthread_mutex_unlock(&mtx_tt);
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

void f_CMD_GETMSG(COMMAND r_cmd) {
//	char buffer[12];
//	snprintf(buffer, 12, "%d", r_cmd.Body.un_msg.Duration);
//	// Print "ID: "
//	win_print2(WIN_MAIN, 1, 1, "ID: ", FALSE, C_YB);
//	// Print "%d" = ID
//	win_print2(WIN_MAIN, 1, 9, buffer, FALSE, C_NONE);
	// Print "Author: "
	win_print2(WIN_MAIN, 1, 1, "Author: ", FALSE, C_YB);
	// Print "%s" = Author
	win_print2(WIN_MAIN, 1, 9, r_cmd.Body.un_msg.Author, FALSE, C_NONE);
	// Print "Topic: "
	win_print2(WIN_MAIN, 2, 1, "Topic: ", FALSE, C_YB);
	// Print "%s" = Topic
	win_print2(WIN_MAIN, 2, 9, r_cmd.Body.un_msg.Topic, FALSE, C_NONE);
	// Print "Title: "
	win_print2(WIN_MAIN, 3, 1, "Title: ", FALSE, C_YB);
	// Print "%s" = Title
	win_print2(WIN_MAIN, 3, 9, r_cmd.Body.un_msg.Title, FALSE, C_NONE);
	// Print "Body: "
	win_print2(WIN_MAIN, 4, 1, "Body: ", FALSE, C_YB);
	// Print "%s" = Body
    char *str;
    str = strtok(r_cmd.Body.un_msg.Body, "\n");
    int i = 5;
    while (str != NULL && i < cl_cfg.win[WIN_MAIN].height - 1) {
        win_print2(WIN_MAIN, i, 1, str, FALSE, C_NONE);
        i++;
        str = strtok(NULL, "\n");
    }
}

void f_CMD_GETTITLES(COMMAND r_cmd) {
	COMMAND r_cmd_tt;
	char buffer[MAX_TPCTTL];
	for (int i = 0; i < r_cmd.Body.un_tt && i < LINES - 1; i++) {
		read(cl_cfg.cl_fifo_tt_fd, &r_cmd_tt, sizeof(COMMAND));
		// Print "ID: "
		win_print2(WIN_MAIN, i + 1, 1, "ID: ", FALSE, C_YB);
		// Print "%d" = ID
		snprintf(buffer, MAX_TPCTTL, "%d", r_cmd_tt.Body.un_msg.Duration);
		win_print2(WIN_MAIN, i + 1, 5, buffer, FALSE, C_NONE);
		// Print "Author: "
		win_print2(WIN_MAIN, i + 1, 17, "Author: ", FALSE, C_YB);
		// Print "%s" = Author
		snprintf(buffer, MAX_TPCTTL, "%s", r_cmd_tt.Body.un_msg.Author);
		win_print2(WIN_MAIN, i + 1, 25, buffer, FALSE, C_NONE);
		// Print "Title: "
		win_print2(WIN_MAIN, i + 1, 27 + MAX_USER, "Title: ", FALSE, C_YB);
		// Print "%s" = Title
		snprintf(buffer, MAX_TPCTTL, "%s", r_cmd_tt.Body.un_msg.Title);
		win_print2(WIN_MAIN, i + 1, 34 + MAX_USER, buffer, FALSE, C_NONE);
	}
}

void f_CMD_GETTOPICS(COMMAND r_cmd) {
	COMMAND r_cmd_tt;
	char buffer[MAX_TPCTTL];
	for (int i = 0; i < r_cmd.Body.un_tt && i < LINES - 1; i++) {
		read(cl_cfg.cl_fifo_tt_fd, &r_cmd_tt, sizeof(COMMAND));
		// Print "ID: "
		win_print2(WIN_MAIN, i + 1, 1, "ID: ", FALSE, C_YB);
		// Print "%d" = ID
		snprintf(buffer, MAX_TPCTTL, "%d", r_cmd_tt.Body.un_msg.Duration);
		win_print2(WIN_MAIN, i + 1, 5, buffer, FALSE, C_NONE);
		// Print "Topic: "
		win_print2(WIN_MAIN, i + 1, 17, "Topic: ", FALSE, C_YB);
		// Print "%s" = Topic
		snprintf(buffer, MAX_TPCTTL, "%s", r_cmd_tt.Body.un_msg.Topic);
		win_print2(WIN_MAIN, i + 1, 24, buffer, FALSE, C_NONE);
	}
}

void f_CMD_SUB(COMMAND r_cmd) {
	cl_cfg.cl_unr_msg++;
	char str[5];
	sprintf(str, "%d", cl_cfg.cl_unr_msg);
	win_print2(WIN_HUD, 3, 14, str, FALSE, C_BW);
}

void f_CMD_ERR(COMMAND r_cmd) {
	win_print2(WIN_SVMSG, 3, 1, r_cmd.Body.un_topic, FALSE, C_NONE);
}

void set_signal() {
	signal(SIGHUP, received_signal);
	signal(SIGINT, received_signal);
	signal(SIGPIPE, received_signal);
	signal(SIGALRM, received_signal);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	signal(SIGSTOP, received_signal);
	signal(SIGTSTP, received_signal);
	signal(SIGTTIN, received_signal);
	signal(SIGTTOU, received_signal);
}

void received_signal(int i) {
	cl_exit(-2);
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
	init_pair(C_WBL, COLOR_WHITE, COLOR_BLUE);

	cl_cfg.win[WIN_HUD].height = WIN_H_H;
	cl_cfg.win[WIN_HUD].width = COLS * (1 - WIN_H_W);
	cl_cfg.win[WIN_HUD].w = newwin(WIN_H_H, COLS * (1 - WIN_H_W), 0, 0);
	cl_cfg.win[WIN_SVMSG].height = WIN_H_H;
	cl_cfg.win[WIN_SVMSG].width = COLS * WIN_H_W + 1;
	cl_cfg.win[WIN_SVMSG].w = newwin(WIN_H_H, COLS * WIN_H_W + 1, 0, COLS * (1 - WIN_H_W));
	cl_cfg.win[WIN_MAIN].height = LINES - WIN_H_H;
	cl_cfg.win[WIN_MAIN].width = COLS;
	cl_cfg.win[WIN_MAIN].w = newwin(LINES - WIN_H_H, COLS, WIN_H_H, 0);
	keypad(cl_cfg.win[WIN_MAIN].w, TRUE);

	for (int i = 0; i < NUM_WIN; i++) {
		win_draw2(i);
	}

	char str[10], str2[6], str3[13], str4[5];
	sprintf(str, "Username:");
	sprintf(str2, "FIFO:");
	sprintf(str3, "Unread msgs:");
	sprintf(str4, "%d", cl_cfg.cl_unr_msg);
	win_print2(WIN_HUD, 1, 1, str, FALSE, C_YB);
	win_print2(WIN_HUD, 2, 1, str2, FALSE, C_YB);
	win_print2(WIN_HUD, 3, 1, str3, FALSE, C_YB);
	win_print2(WIN_HUD, 1, 14, cl_cfg.cl_username, FALSE, C_NONE);
	win_print2(WIN_HUD, 2, 14, cl_cfg.cl_fifo, FALSE, C_NONE);
	win_print2(WIN_HUD, 3, 14, str4, FALSE, C_NONE);
}

// w = window index --> 0 = top left box, 1 = top right box, 2 = main box
// x = x coordinate inside the window
// y = y coordinate inside the window
// str = string to write on the window
void win_print(int w, int y, int x, char *str) {
	mvwprintw(cl_cfg.win[w].w, y, x, str);
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

void win_print2(int w, int y, int x, char *str, int redraw, int c_pair) {
	pthread_mutex_lock(&mtx_win);
	if (redraw) {
		win_draw(w);
	}
	if (c_pair) {
		win_color(w, c_pair, TRUE);
	}
	win_print(w, y, x, str);
	if (c_pair) {
		win_color(w, c_pair, FALSE);
	}
	pthread_mutex_unlock(&mtx_win);
}

void win_erase(int w) {
	werase(cl_cfg.win[w].w);
}

void win_refresh(int w) {
	wrefresh(cl_cfg.win[w].w);
}

void win_draw(int w) {
	char *env;
	win_erase(w);
	box(cl_cfg.win[w].w, 0, 0);
	if ((env = getenv("TERM")) != NULL) {
		if (strcmp(env, "xterm") == 0) {
            wborder(cl_cfg.win[w].w, '|', '|', '-', '-', '+', '+', '+', '+');
		}
	}
	win_refresh(w);
}

void win_draw2(int w) {
	win_print2(w, -1, -1, "", TRUE, C_NONE);
}

void shutdown() {
	shutdown_init = 1;
	COMMAND cmd;
	if (sv_shutdown == 0 && sv_kick == 0) {
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
	close(cl_cfg.cl_fifo_tt_fd);
	unlink(cl_cfg.cl_fifo_tt);

	if (init_win == 1) {
		for(int i = 0; i < NUM_WIN; i++) {
			wclear(cl_cfg.win[i].w);
			wrefresh(cl_cfg.win[i].w);
			delwin(cl_cfg.win[i].w);
		}
//		clear();
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
