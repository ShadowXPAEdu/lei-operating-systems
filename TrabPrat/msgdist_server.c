#include "msgdist.h"
#include "msgdist_s.h"

int main(int argc, char *argv[], char **envp) {
	// Check if has reset fifo argument
	if (argc == 2)
		if (strcmp(argv[1], "-rst") == 0)
			unlink(sv_fifo);
	// Check if server is already running
	if (IsServerRunning(sv_fifo) != 0) {
		printerr("Looks like the server is already running...\nClosing instance...\n", PERR_ERROR, FALSE);
		sv_exit(-3);
	} else {
		pthread_t tds[THREADS];
		// Initiate configuration
		init_config();
		// Sets action when signal is received
		set_signal();

		// Open 'verificador'
		init_verificador();

		printerr("Opening server.", PERR_NORM, FALSE);

		// Create threads
		// Read cmd from named pipe
		if (pthread_create(&tds[0], NULL, cmd_reader, NULL) != 0) {
			printerr("Could not create thread to read commands from users.", PERR_ERROR, FALSE);
			sv_exit(-3);
		}
		// Checks message durations
		if (pthread_create(&tds[1], NULL, msg_duration, NULL) != 0) {
			printerr("Could not create thread to check message durations.", PERR_ERROR, FALSE);
			sv_exit(1);
		}
		// Sends heartbeat to users
		if (pthread_create(&tds[2], NULL, heartbeat, NULL) != 0) {
			printerr("Could not create thread to send heartbeat messages to users.", PERR_ERROR, FALSE);
			sv_exit(1);
		}

		// Read admin cmds
		pthread_mutex_lock(&mtx_wait_for_init_td);
		printerr("Accepting admin commands.", PERR_NORM, FALSE);
		printf("Type 'help' for a list of commands.\n");
		pthread_mutex_unlock(&mtx_wait_for_init_td);
		// Admin "menu"
		while (adm_cmd_reader() == 0);

		// Wait for all threads to finish
		for (int i = 0; i < THREADS; i++) {
			pthread_join(tds[i], NULL);
		}
//        for (int i = 0; i < THREADS; i++) {
//            pthread_cancel(tds[i]);
//        }
		sv_exit(0);
	}
}

// ********************** INITIALIZER FUNCTIONS

void init_config() {
	// Initialize filter and environment variables
	sv_cfg.use_filter = 1;
	sv_cfg.maxmsg = MAXMSG;
	sv_cfg.maxnot = MAXNOT;
    snprintf(sv_cfg.wordsnot, PATH_MAX, "%s", WORDSNOT);
//	sv_cfg.wordsnot = WORDSNOT;
	char *env;
	if ((env = getenv("MAXMSG")) != NULL) {
		sv_cfg.maxmsg = atoi(env);
        if (sv_cfg.maxmsg <= 0) {
            sv_cfg.maxmsg = MAXMSG;
        }
	}
	if ((env = getenv("MAXNOT")) != NULL) {
		sv_cfg.maxnot = atoi(env);
		if (sv_cfg.maxnot < 0) {
            sv_cfg.maxnot = MAXNOT;
		}
	}
	if ((env = getenv("WORDSNOT")) != NULL) {
        snprintf(sv_cfg.wordsnot, PATH_MAX, "%s", env);
	}
    // Using is server running function to verify if the file exists
	if (IsServerRunning(sv_cfg.wordsnot) == 0) {
        fprintf(stderr, "[Server] WORDSNOT: '%s' does not exist! Checking if default file exists...\n", sv_cfg.wordsnot);
        snprintf(sv_cfg.wordsnot, PATH_MAX, "%s", WORDSNOT);
        if (IsServerRunning(sv_cfg.wordsnot) == 0) {
            fprintf(stderr, "[Server] Default WORDSNOT: '%s' does not exist! Closing...\n", sv_cfg.wordsnot);
            sv_exit(-3);
        }
	}
	fprintf(stderr, "[Server] MAXMSG: %d\n[Server] MAXNOT: %d\n[Server] WORDSNOT: %s\n", sv_cfg.maxmsg, sv_cfg.maxnot, sv_cfg.wordsnot);
	// Initialize users, messages and topics
	sv_cfg.next_uid = 1;
	sv_cfg.next_mid = 1;
	sv_cfg.next_tid = 1;
//
	sv_cfg.n_msgs = 0;

	// Allocate memory for messages
	sv_cfg.msgs = malloc(sv_cfg.maxmsg * sizeof(SV_MSG));
	if (sv_cfg.msgs == NULL) {
		printerr("Couldn't allocate memory for Messages.", PERR_ERROR, FALSE);
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
		printerr("Couldn't allocate memory for Topics.", PERR_ERROR, FALSE);
		free(sv_cfg.msgs);
		sv_exit(-3);
	}
	for (int i = 0; i < sv_cfg.topic_size; i++) {
		sv_cfg.topics[i].id = -1;
	}
	// Allocate memory for users
	sv_cfg.users_size = INIT_MALLOC_SIZE;
	sv_cfg.users = malloc(sv_cfg.users_size * sizeof(SV_USER));
	if (sv_cfg.users == NULL) {
		printerr("Couldn't allocate memory for Users.", PERR_ERROR, FALSE);
		free(sv_cfg.msgs);
		free(sv_cfg.topics);
		sv_exit(-3);
	}
	for (int i = 0; i < sv_cfg.users_size; i++) {
		sv_cfg.users[i].id = -1;
		sv_cfg.users[i].sub_size = INIT_MALLOC_SIZE;
		sv_cfg.users[i].topic_ids = NULL;
	}
}

void init_verificador() {
	printerr("Opening verificador.", PERR_NORM, FALSE);
	int fd1[2];
	int fd2[2];

	printerr("Opening anonymous pipes.", PERR_NORM, FALSE);
	if (pipe(fd1) < 0) {
		printerr("Error opening anonymous pipe.", PERR_ERROR, FALSE);
		sv_exit(-1);
	} else {
		if (pipe(fd2) < 0) {
			printerr("Error opening anonymous pipe.", PERR_ERROR, FALSE);
			sv_exit(-1);
		} else {
			// Anonymous pipes opened
			printerr("Anonymous pipes opened.", PERR_NORM, FALSE);
			printerr("Creating child process.", PERR_NORM, FALSE);
			sv_cfg.sv_verificador_pid = fork();
			if (sv_cfg.sv_verificador_pid == -1) {
				printerr("Error creating child process.", PERR_ERROR, FALSE);
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
				printerr("Child process created.", PERR_NORM, FALSE);
				close(fd1[0]);
				close(fd2[1]);
				sv_cfg.sv_verificador_pipes[0] = fd2[0];
				sv_cfg.sv_verificador_pipes[1] = fd1[1];
			}
		}
	}
}

// ********************** INITIALIZER FUNCTIONS END

// ********************** MAIN THREAD FUNCTION

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
				adm_cmd_topics();
			} else if (strcmp(adm_cmd, "msg") == 0) {
				// list messages
				adm_cmd_msg();
			} else if (strcmp(adm_cmd, "view") == 0) {
				// view help
				printf("Incomplete command.\nTo use '%s', try '%s [message_id]'\n", adm_cmd, adm_cmd);
			} else if (strcmp(adm_cmd, "subs") == 0) {
				// subs help
				printf("Incomplete command.\nTo use '%s', try '%s [user_id]'\n", adm_cmd, adm_cmd);
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
				adm_cmd_prune();
			} else if (strcmp(adm_cmd, "shutdown") == 0 || strcmp(adm_cmd, "exit") == 0) {
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
				adm_cmd_topic(ptr);
			} else if (strcmp(adm_cmd, "del") == 0) {
				// delete message with msg_id [ptr]
				adm_cmd_del(ptr);
			} else if (strcmp(adm_cmd, "kick") == 0) {
				// kick user with user_id [ptr]
				adm_cmd_kick(ptr);
			} else if (strcmp(adm_cmd, "verify") == 0) {
				// verifies the message with 'verificador'
				adm_cmd_verify(ptr, 1);
			} else if (strcmp(adm_cmd, "view") == 0) {
				// view message with message_id [ptr]
				adm_cmd_view(ptr);
			} else if (strcmp(adm_cmd, "subs") == 0) {
				// view subscriptions from user with user_id [ptr]
				adm_cmd_subs(ptr);
			} else {
				printf("Unknown command...\n");
			}
		}
	}
	return 0;
}

// ********************** MAIN THREAD FUNCTION END

// ********************** THREAD FUNCTIONS

void *cmd_reader() {
	pthread_mutex_lock(&mtx_wait_for_init_td);
	set_signal();
	if (mkfifo(sv_fifo, 0666) == 0) {
		printerr("Named pipe created.", PERR_INFO, FALSE);
		sv_cfg.sv_fifo_fd = open(sv_fifo, O_RDWR);
		if (sv_cfg.sv_fifo_fd == -1) {
			printerr("Error opening named pipe.", PERR_ERROR, FALSE);
			pthread_mutex_unlock(&mtx_wait_for_init_td);
			sv_exit(-1);
		} else {
			printerr("Named pipe opened.", PERR_INFO, FALSE);
			printerr("Ready to receive commands from users.", PERR_INFO, FALSE);
			pthread_mutex_unlock(&mtx_wait_for_init_td);
			COMMAND cmd;
			pthread_t hc;
			while (cmd_reader_bool == 0) {
				cmd.cmd = CMD_IGN;
				read(sv_cfg.sv_fifo_fd, &cmd, sizeof(COMMAND));

				// Handle the connection in a separate thread
				// so that the server can receive more commands at once
				COMMAND *cmd2 = malloc(sizeof(COMMAND));
				*cmd2 = cmd;
				if (pthread_create(&hc, NULL, handle_connection, cmd2) != 0) {
					free(cmd2);
				}
			}
			close(sv_cfg.sv_fifo_fd);
			printerr("Named pipe closed.", PERR_INFO, FALSE);
			unlink(sv_fifo);
			printerr("Named pipe deleted.", PERR_INFO, FALSE);
		}
	} else {
		printerr("An error occurred trying to open named pipe!\nClosing...", PERR_ERROR, FALSE);
		pthread_mutex_unlock(&mtx_wait_for_init_td);
		sv_exit(-1);
	}
	return NULL;
}

void *handle_connection(void *p_command) {
	COMMAND r_cmd = *((COMMAND *)p_command);
	free(p_command);

	// If cmd_reader_bool is still 0
	if (cmd_reader_bool == 0) {
		// Take care of cmd
		switch (r_cmd.cmd) {
			case CMD_CON:
				// User connecting
				pthread_mutex_lock(&mtx_user);
				f_CMD_CON(r_cmd);
				pthread_mutex_unlock(&mtx_user);
				break;
			case CMD_DC:
				// User disconnecting
				pthread_mutex_lock(&mtx_user);
				f_CMD_DC(r_cmd);
				pthread_mutex_unlock(&mtx_user);
				break;
			case CMD_NEWMSG:
				// User sent new msg
				pthread_mutex_lock(&mtx_msg);
				f_CMD_NEWMSG(r_cmd);
				pthread_mutex_unlock(&mtx_msg);
				break;
			case CMD_GETTOPICS:
				// User asked for topics
				pthread_mutex_lock(&mtx_topic);
				pthread_mutex_lock(&mtx_user);
				f_CMD_GETTOPICS(r_cmd);
				pthread_mutex_unlock(&mtx_user);
				pthread_mutex_unlock(&mtx_topic);
				break;
			case CMD_GETTITLES:
				// User asked for messages
				pthread_mutex_lock(&mtx_msg);
				pthread_mutex_lock(&mtx_user);
				f_CMD_GETTITLES(r_cmd);
				pthread_mutex_unlock(&mtx_user);
				pthread_mutex_unlock(&mtx_msg);
				break;
			case CMD_GETMSG:
				// User asked for specific message
				pthread_mutex_lock(&mtx_msg);
				f_CMD_GETMSG(r_cmd);
				pthread_mutex_unlock(&mtx_msg);
				break;
			case CMD_SUB:
				// User subscribed to a topic
				pthread_mutex_lock(&mtx_topic);
				pthread_mutex_lock(&mtx_user);
				f_CMD_SUB(r_cmd);
				pthread_mutex_unlock(&mtx_user);
				pthread_mutex_unlock(&mtx_topic);
				break;
			case CMD_UNSUB:
				// User unsubscribed to a topic
				pthread_mutex_lock(&mtx_topic);
				pthread_mutex_lock(&mtx_user);
				f_CMD_UNSUB(r_cmd);
				pthread_mutex_unlock(&mtx_user);
				pthread_mutex_unlock(&mtx_topic);
				break;
			case CMD_ALIVE:
				// User is ALIVE
				pthread_mutex_lock(&mtx_user);
				f_CMD_ALIVE(r_cmd);
				pthread_mutex_unlock(&mtx_user);
				break;
			case CMD_IGN:
				// Ignore cmd
				break;
			default:
				// Send Err cmd
				f_CMD_default(r_cmd);
				break;
		}
	}
	return NULL;
}

void f_CMD_CON(COMMAND r_cmd) {
	int uid = -1;
	int x = 0;
	int fd;
	char username[MAX_USER];
	COMMAND s_cmd;
	strncpy(s_cmd.Body.un_user.Username, r_cmd.Body.un_user.Username, MAX_USER);
	strncpy(s_cmd.Body.un_user.FIFO, r_cmd.Body.un_user.FIFO, MAX_FIFO);
	strncpy(s_cmd.From, sv_fifo, MAX_FIFO);
	strncpy(username, r_cmd.Body.un_user.Username, MAX_USER);
	while (get_uindex_by_username(r_cmd.Body.un_user.Username) != -1 && x != 100) {
		// Change username
		char *_u = malloc(MAX_USER * sizeof(char));
		snprintf(_u, MAX_USER, "%s(%d)", username, x);
		strncpy(r_cmd.Body.un_user.Username, _u, MAX_USER);
		free(_u);
		strncpy(s_cmd.Body.un_user.Username, r_cmd.Body.un_user.Username, MAX_USER);
		x++;
	}
	uid = add_user(r_cmd.Body.un_user);
	if (uid >= 0 && x != 100) {
		int ind = get_uindex_by_id(uid);
		char *_u = malloc((MAX_USER + strlen("User '' has connected.")) * sizeof(char));
		sprintf(_u, "User '%s' has connected.", r_cmd.Body.un_user.Username);
		printerr(_u, PERR_INFO, TRUE);
		free(_u);
		s_cmd.cmd = CMD_OK;
		write(sv_cfg.users[ind].user_fd, &s_cmd, sizeof(COMMAND));
		// Only alive after we reply OK to the connection
		sv_cfg.users[ind].alive = 1;
	} else {
		s_cmd.cmd = CMD_ERR;
		strncpy(s_cmd.Body.un_topic, "Server is out of space, or username is not available.", strlen("Server is out of space, or username is not available."));
		int op = open(r_cmd.From, O_WRONLY);
		write(op, &s_cmd, sizeof(COMMAND));
		close(op);
	}
}

void f_CMD_DC(COMMAND r_cmd) {
	int i = get_uindex_by_FIFO(r_cmd.From);
	char buf[26 + MAX_USER];
	snprintf(buf, 26 + MAX_USER, "User '%s' has disconnected.", sv_cfg.users[i].user.Username);
	printerr(buf, PERR_INFO, TRUE);
	rem_user(r_cmd.From);
}

void f_CMD_ALIVE(COMMAND r_cmd) {
	int ind = get_uindex_by_FIFO(r_cmd.From);
	if (ind != -1) {
		sv_cfg.users[ind].alive = 1;
	}
}

void f_CMD_NEWMSG(COMMAND r_cmd) {
	// Check server capacity
	// Check if message has bad words or not
	// Check is message already exists
	COMMAND s_cmd;
	strncpy(s_cmd.From, sv_fifo, MAX_FIFO);
	if (sv_cfg.n_msgs < sv_cfg.maxmsg) {
		// Verify bad words
		int i = adm_cmd_verify(r_cmd.Body.un_msg.Body, 0);
		if (i > sv_cfg.maxnot) {
			// Delete message and warn user
			s_cmd.cmd = CMD_ERR;
			strncpy(s_cmd.Body.un_topic, "Invalid message. Message was deleted from the server.", sizeof("Invalid message. Message was deleted from the server.") + 1);
			pthread_mutex_lock(&mtx_user);
			int ui = get_uindex_by_FIFO(r_cmd.From);
			write(sv_cfg.users[ui].user_fd, &s_cmd, sizeof(COMMAND));
			pthread_mutex_unlock(&mtx_user);
			char buf[MAX_USER + 93];
			snprintf(buf, MAX_USER + 93, "User '%s' tried adding a message to the server but had %d bad words and was rejected.", r_cmd.Body.un_msg.Author, i);
			printerr(buf, PERR_INFO, TRUE);
		} else {
			int ind;
			// Save message
			if ((ind = get_mindex_by_message(r_cmd.Body.un_msg)) != -1) {
				sv_cfg.msgs[ind].msg.Duration += r_cmd.Body.un_msg.Duration;
				char buf[MAX_USER + 79];
				snprintf(buf, MAX_USER + 79, "User '%s' extended a message's time on the server by adding %d seconds.", r_cmd.Body.un_msg.Author, r_cmd.Body.un_msg.Duration);
				printerr(buf, PERR_INFO, TRUE);
			} else {
				// Check if topic exists
				pthread_mutex_lock(&mtx_topic);
				if (get_tindex_by_topic_name(r_cmd.Body.un_msg.Topic) == -1) {
					// Topic doesn't exist add it to the list
					add_topic(r_cmd.Body.un_msg.Topic);
					char buf[MAX_TPCTTL + 62];
					snprintf(buf, MAX_TPCTTL + 62, "Topic '%s' does not exist therefore it was added to the server.", r_cmd.Body.un_msg.Topic);
					printerr(buf, PERR_INFO, TRUE);
				} else {
					// Topic exists check who needs to be warned
					// Lock users to warn each user
					pthread_mutex_lock(&mtx_user);
					int num;
					int *userfd = get_ufd_subbed_to(r_cmd.Body.un_msg.Topic, &num);
					COMMAND s_cmd;
					s_cmd.cmd = CMD_SUB;
					strncpy(s_cmd.From, sv_fifo, MAX_FIFO);
					strncpy(s_cmd.Body.un_topic, r_cmd.Body.un_msg.Topic, MAX_TPCTTL);
					for (int ui = 0; ui < num; ui++) {
						write(userfd[ui], &s_cmd, sizeof(COMMAND));
					}
					pthread_mutex_unlock(&mtx_user);
				}
				pthread_mutex_unlock(&mtx_topic);
				add_msg(r_cmd.Body.un_msg);
				char buf[MAX_USER + 66];
				snprintf(buf, MAX_USER + 66, "User '%s' added a message to the server with %d bad words.", r_cmd.Body.un_msg.Author, i);
				printerr(buf, PERR_INFO, TRUE);
			}
		}
	} else {
		s_cmd.cmd = CMD_ERR;
		strncpy(s_cmd.Body.un_topic, "Server capacity reached its limit. Message not sent.", 53);
		pthread_mutex_lock(&mtx_user);
		int i = get_uindex_by_FIFO(r_cmd.From);
		write(sv_cfg.users[i].user_fd, &s_cmd, sizeof(COMMAND));
		pthread_mutex_unlock(&mtx_user);
		char buf[MAX_USER + 94];
		snprintf(buf, MAX_USER + 94, "User '%s' tried adding a message to the server however server is full and rejected the message.", r_cmd.Body.un_msg.Author);
		printerr(buf, PERR_INFO, TRUE);
	}
}

void f_CMD_GETTOPICS(COMMAND r_cmd) {
	COMMAND s_cmd;
	s_cmd.cmd = CMD_GETTOPICS;
	snprintf(s_cmd.From, MAX_FIFO, "%s", sv_fifo);
	int n_topics = count_topics();
	s_cmd.Body.un_tt = n_topics;
	int u_ind = get_uindex_by_FIFO(r_cmd.From);
	if (u_ind != -1) {
		// Send number of topics
		write(sv_cfg.users[u_ind].user_fd, &s_cmd, sizeof(COMMAND));
		// Open new fifo communication so there is no other interference
		char tt_fifo[MAX_FIFO];
		snprintf(tt_fifo, MAX_FIFO + 3, "%s_tt", sv_cfg.users[u_ind].user.FIFO);
		int tt_fd = open(tt_fifo, O_WRONLY);
		COMMAND s_cmd2;
		s_cmd2.cmd = CMD_GETTOPICS;
		snprintf(s_cmd2.From, MAX_FIFO, "%s", sv_fifo);
		// Send each topic at a time
		for (int i = 0; i < sv_cfg.topic_size; i++) {
			// Duration = ID of topic
			if (sv_cfg.topics[i].id != -1) {
				s_cmd2.Body.un_msg.Duration = sv_cfg.topics[i].id;
				snprintf(s_cmd2.Body.un_msg.Topic, MAX_TPCTTL, "%s", sv_cfg.topics[i].topic);
				write(tt_fd, &s_cmd2, sizeof(COMMAND));
			}
		}
		close(tt_fd);
	} else {
		s_cmd.cmd = CMD_ERR;
		snprintf(s_cmd.Body.un_topic, MAX_TPCTTL, "User not connect or has timed out.");
		int tmp_fd = open(r_cmd.From, O_WRONLY);
		write(tmp_fd, &s_cmd, sizeof(COMMAND));
		close(tmp_fd);
	}
}

void f_CMD_GETTITLES(COMMAND r_cmd) {
	COMMAND s_cmd;
	s_cmd.cmd = CMD_GETTITLES;
	snprintf(s_cmd.From, MAX_FIFO, "%s", sv_fifo);
	int n_titles = count_msg();
	s_cmd.Body.un_tt = n_titles;
	int u_ind = get_uindex_by_FIFO(r_cmd.From);
	if (u_ind != -1) {
		// Send number of titles
		write(sv_cfg.users[u_ind].user_fd, &s_cmd, sizeof(COMMAND));
		// Open new fifo communication so there is no other interference
		char tt_fifo[MAX_FIFO];
		snprintf(tt_fifo, MAX_FIFO + 3, "%s_tt", sv_cfg.users[u_ind].user.FIFO);
		int tt_fd = open(tt_fifo, O_WRONLY);
		COMMAND s_cmd2;
		s_cmd2.cmd = CMD_GETTOPICS;
		snprintf(s_cmd2.From, MAX_FIFO, "%s", sv_fifo);
		// Send each title at a time
		for (int i = 0; i < sv_cfg.maxmsg; i++) {
			// Duration = ID of message
			if (sv_cfg.msgs[i].id != -1) {
				s_cmd2.Body.un_msg.Duration = sv_cfg.msgs[i].id;
				snprintf(s_cmd2.Body.un_msg.Title, MAX_TPCTTL, "%s", sv_cfg.msgs[i].msg.Title);
				snprintf(s_cmd2.Body.un_msg.Author, MAX_USER, "%s", sv_cfg.msgs[i].msg.Author);
				write(tt_fd, &s_cmd2, sizeof(COMMAND));
			}
		}
		close(tt_fd);
	} else {
		s_cmd.cmd = CMD_ERR;
		snprintf(s_cmd.Body.un_topic, MAX_TPCTTL, "User not connect or has timed out.");
		int tmp_fd = open(r_cmd.From, O_WRONLY);
		write(tmp_fd, &s_cmd, sizeof(COMMAND));
		close(tmp_fd);
	}
}

void f_CMD_GETMSG(COMMAND r_cmd) {
	COMMAND s_cmd;
	strncpy(s_cmd.From, sv_fifo, MAX_FIFO);
	int m_ind = get_mindex_by_id(r_cmd.Body.un_tt);
	pthread_mutex_lock(&mtx_user);
	int u_ind = get_uindex_by_FIFO(r_cmd.From);
	pthread_mutex_unlock(&mtx_user);
	if (m_ind != -1) {
		// If message exists sends message to client
		MESSAGE m;
		snprintf(m.Author, MAX_USER, "%s", sv_cfg.msgs[m_ind].msg.Author);
		snprintf(m.Body, MAX_BODY, "%s", sv_cfg.msgs[m_ind].msg.Body);
		snprintf(m.Title, MAX_TPCTTL, "%s", sv_cfg.msgs[m_ind].msg.Title);
		snprintf(m.Topic, MAX_TPCTTL, "%s", sv_cfg.msgs[m_ind].msg.Topic);
		m.Duration = sv_cfg.msgs[m_ind].id;
		s_cmd.cmd = CMD_GETMSG;
		s_cmd.Body.un_msg = m;
		if (u_ind != -1) {
			pthread_mutex_lock(&mtx_user);
			write(sv_cfg.users[u_ind].user_fd, &s_cmd, sizeof(COMMAND));
			pthread_mutex_unlock(&mtx_user);
		}
	} else {
		// Send CMD_ERR
		// No message with specified ID
		s_cmd.cmd = CMD_ERR;
		snprintf(s_cmd.Body.un_topic, MAX_TPCTTL, "No message exists with specified ID.");
		if (u_ind != -1) {
			pthread_mutex_lock(&mtx_user);
			write(sv_cfg.users[u_ind].user_fd, &s_cmd, sizeof(COMMAND));
			pthread_mutex_unlock(&mtx_user);
		}
	}
}

void f_CMD_SUB(COMMAND r_cmd) {
	subscribe(r_cmd.From, r_cmd.Body.un_tt);
}

void f_CMD_UNSUB(COMMAND r_cmd) {
	unsubscribe(r_cmd.From, r_cmd.Body.un_tt);
}

void f_CMD_default(COMMAND r_cmd) {
	COMMAND s_cmd;
	s_cmd.cmd = CMD_ERR;
	strncpy(s_cmd.From, sv_fifo, MAX_FIFO);
	strncpy(s_cmd.Body.un_topic, "Invalid server command.", sizeof("Invalid server command.") + 1);
	pthread_mutex_lock(&mtx_user);
	int i = get_uindex_by_FIFO(r_cmd.From);
	write(sv_cfg.users[i].user_fd, &s_cmd, sizeof(COMMAND));
	pthread_mutex_unlock(&mtx_user);
}

void *msg_duration() {
	pthread_mutex_lock(&mtx_wait_for_init_td);
	set_signal();
	printerr("Ready to check messages duration.", PERR_INFO, FALSE);
	pthread_mutex_unlock(&mtx_wait_for_init_td);
	while (msg_duration_bool == 0) {
		pthread_mutex_lock(&mtx_msg);
		// maxmsg is fixed
		if (sv_cfg.n_msgs > 0) {
			for (int i = 0; i < sv_cfg.maxmsg; i++) {
				//pthread_mutex_lock(&mtx_msg);
				if (sv_cfg.msgs[i].id != -1) {
					// Get [i]th msg and decrements duration
					sv_cfg.msgs[i].msg.Duration--;
					// Check if [i]th msg duration is 0
					if (sv_cfg.msgs[i].msg.Duration == 0) {
						// If msg duration is 0 set msg id to -1
						char buf[43];
						snprintf(buf, 43, "Message with ID [%d] has expired.", sv_cfg.msgs[i].id);
						printerr(buf, PERR_INFO, TRUE);
						rem_msg(sv_cfg.msgs[i].id);
					}
				}
				//pthread_mutex_unlock(&mtx_msg);
			}
		}
		pthread_mutex_unlock(&mtx_msg);
		sleep(1);
	}
	return NULL;
}

void *heartbeat() {
	pthread_mutex_lock(&mtx_wait_for_init_td);
	set_signal();
	COMMAND cmd;
	cmd.cmd = CMD_HEARTBEAT;
	strncpy(cmd.From, sv_fifo, MAX_FIFO);
	printerr("Ready to send heartbeat to users.", PERR_INFO, FALSE);
	pthread_mutex_unlock(&mtx_wait_for_init_td);
	while (heartbeat_bool == 0) {
		pthread_mutex_lock(&mtx_user);
		for (int i = 0; i < sv_cfg.users_size; i++) {
			if (sv_cfg.users[i].id != -1 && sv_cfg.users[i].alive == 1) {
				// Set user alive to 0
				sv_cfg.users[i].alive = 0;
				// Send HB CMD to user
				write(sv_cfg.users[i].user_fd, &cmd, sizeof(COMMAND));
			}
		}
		pthread_mutex_unlock(&mtx_user);
		sleep(10);
		pthread_mutex_lock(&mtx_user);
		for (int i = 0; i < sv_cfg.users_size; i++) {
			// Sets ID -1 of users with alive 0
			if (sv_cfg.users[i].id != -1 && sv_cfg.users[i].alive == 0) {
				rem_user2(i);
			}
		}
		// Resize users, sorts users
		resize_users();
		pthread_mutex_unlock(&mtx_user);
	}
	return NULL;
}

// ********************** THREAD FUNCTIONS END

// ********************** RANDOM FUNCTIONS

void sv_exit(int return_val) {
	if (return_val != -3 && shutdown_init == 0) {
		shutdown();
	}
	exit(return_val);
}

void shutdown() {
	COMMAND cmd;
	shutdown_init = 1;
	printf("[Server] Server shutting down. Please wait.\n");
	// Warn users of server shutdown
	pthread_mutex_lock(&mtx_user);
	for (int i = 0; i < sv_cfg.users_size; i++) {
		if (sv_cfg.users[i].id != -1) {
			cmd.cmd = CMD_SDC;
			strncpy(cmd.From, sv_fifo, MAX_FIFO);
			sv_cfg.users[i].id = -1;
			write(sv_cfg.users[i].user_fd, &cmd, sizeof(COMMAND));
			close(sv_cfg.users[i].user_fd);
		}
	}
	pthread_mutex_unlock(&mtx_user);
	// Set thread control variables
	heartbeat_bool = 1;
	msg_duration_bool = 1;
	cmd_reader_bool = 1;
	// Send CMD_IGN to own fifo so that we can leave the thread
	cmd.cmd = CMD_IGN;
	write(sv_cfg.sv_fifo_fd, &cmd, sizeof(COMMAND));
//    close(sv_cfg.sv_fifo_fd);
//    printerr("Named pipe closed.", PERR_INFO, FALSE);
//    unlink(sv_fifo);
//    printerr("Named pipe deleted.", PERR_INFO, FALSE);
	// Kill 'verificador'
	close(sv_cfg.sv_verificador_pipes[0]);
    close(sv_cfg.sv_verificador_pipes[1]);
	kill(sv_cfg.sv_verificador_pid, SIGUSR2);
}

// str = string to show, val = PERR_????, adm = write "\nAdmin > " after
void printerr(const char *str, int val, int adm) {
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
	fprintf(stderr, "%s\n", str);
	if (adm) {
		write(STDERR_FILENO, "Admin > ", strlen("Admin > "));
	}
}

void set_signal() {
	signal(SIGHUP, received_signal);
	signal(SIGINT, received_signal);
	// Ignore broken pipes...
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, received_signal);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	signal(SIGSTOP, received_signal);
	signal(SIGTSTP, received_signal);
	signal(SIGTTIN, received_signal);
	signal(SIGTTOU, received_signal);
}

void received_signal(int i) {
	sv_exit(i);
}

// ********************** RANDOM FUNCTIONS END

// ********************** ADMINISTRATOR COMMAND FUNCTIONS

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
	printf("\tverify [message] - Uses the 'verificador' program to verify the message for banned words.\n");
	printf("\tcfg - Shows the server's configuration.\n");
	printf("\tview [message_id] - View message with the message_id.\n");
	printf("\tsubs [user_id] - View subscriptions from the user with the user_id.\n");
	printf("\tshutdown - Shuts the server down.\n");
	printf("\texit - Same as shutdown.\n");
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
	pthread_mutex_lock(&mtx_user);
	printf("Current number of users: %d\n", count_users());
	for (int i = 0; i < sv_cfg.users_size; i++) {
		// Users with ID = -1 don't count as users
		if (sv_cfg.users[i].id != -1) {
			printf("\tID: %d\tUsername: %s\tFIFO: %s\n", sv_cfg.users[i].id, sv_cfg.users[i].user.Username, sv_cfg.users[i].user.FIFO);
		}
	}
	pthread_mutex_unlock(&mtx_user);
}

void adm_cmd_topics() {
	pthread_mutex_lock(&mtx_topic);
	printf("Current number of topics: %d\n", count_topics());
	for (int i = 0; i < sv_cfg.topic_size; i++) {
		// Topics with ID = -1 don't count as topics
		if (sv_cfg.topics[i].id != -1) {
			printf("\tID: %d\tTopic: %s\n", sv_cfg.topics[i].id, sv_cfg.topics[i].topic);
		}
	}
	pthread_mutex_unlock(&mtx_topic);
}

void adm_cmd_msg() {
	pthread_mutex_lock(&mtx_msg);
	printf("Current number of messages: %d\n", sv_cfg.n_msgs);
	for (int i = 0; i < sv_cfg.maxmsg; i++) {
		// Messages with ID = -1 don't count as messages
		if (sv_cfg.msgs[i].id != -1) {
			printf("\tID: %d\tTitle: %s\tAuthor: %s\tTime left: %d seconds\n", sv_cfg.msgs[i].id, sv_cfg.msgs[i].msg.Title, sv_cfg.msgs[i].msg.Author, sv_cfg.msgs[i].msg.Duration);
		}
	}
	pthread_mutex_unlock(&mtx_msg);
}

void adm_cmd_prune() {
	int i, j, k, t, n = 0, s = 0;
	pthread_mutex_lock(&mtx_topic);
	// foreach topic
	for (i = 0; i < sv_cfg.topic_size; i++) {
		if (sv_cfg.topics[i].id != -1) {
			t = 0;
			// foreach message
			pthread_mutex_lock(&mtx_msg);
			for (j = 0; j < sv_cfg.maxmsg; j++) {
				if (sv_cfg.msgs[j].id != -1 && strcmp(sv_cfg.topics[i].topic, sv_cfg.msgs[j].msg.Topic) == 0) {
					t = 1;
					break;
				}
			}
			if (t == 0) {
				// No messages with the same topic
				// Delete topic and unsubscribe
				pthread_mutex_lock(&mtx_user);
				int num, u_ind;
				// Get user file descriptors of who is subscribed to certain topic
				int *ufd = get_ufd_subbed_to(sv_cfg.topics[i].topic, &num);
				// foreach user subscribed
				for (k = 0; k < num; k++) {
					u_ind = get_uindex_by_fd(ufd[k]);
					unsubscribe(sv_cfg.users[k].user.FIFO, sv_cfg.topics[i].id);
					s++;
				}
				pthread_mutex_unlock(&mtx_user);
				rem_topic(sv_cfg.topics[i].id);
				n++;
			}
			pthread_mutex_unlock(&mtx_msg);
		}
	}
	pthread_mutex_unlock(&mtx_topic);
	printf("[Server] The prune command deleted %d topics.\n", n);
	printf("[Server] The prune command canceled %d subscriptions.\n", s);
}

void adm_cmd_topic(const char *ptr) {
	char top[MAX_TPCTTL];
	snprintf(top, MAX_TPCTTL, "%s", ptr);
	// Check topic
	pthread_mutex_lock(&mtx_topic);
	int t_i = get_tindex_by_topic_name(top);
	if (t_i == -1) {
		printerr("Invalid topic name.", PERR_WARNING, FALSE);
	} else {
		// Writes messages with that topic on screen
		pthread_mutex_lock(&mtx_msg);
		printf("Messages with topic '%s':\n", top);
		for (int i = 0; i < sv_cfg.maxmsg; i++) {
			if (sv_cfg.msgs[i].id != -1 && strcmp(sv_cfg.msgs[i].msg.Topic, top) == 0) {
				printf("\tID: %d\tTitle: %s\tAuthor: %s\tTime left: %d seconds\n", sv_cfg.msgs[i].id, sv_cfg.msgs[i].msg.Title, sv_cfg.msgs[i].msg.Author, sv_cfg.msgs[i].msg.Duration);
			}
		}
		pthread_mutex_unlock(&mtx_msg);
	}
	pthread_mutex_unlock(&mtx_topic);
}

void adm_cmd_del(const char *ptr) {
	int i = atoi(ptr);
	pthread_mutex_lock(&mtx_msg);
	if (i <= 0 || i >= sv_cfg.next_mid) {
		printerr("Invalid ID.", PERR_WARNING, FALSE);
	} else {
		// Check if message was deleted
		if (rem_msg(i)) {
			printerr("Message deleted.", PERR_INFO, FALSE);
		} else {
			printerr("Invalid ID.", PERR_WARNING, FALSE);
		}
	}
	pthread_mutex_unlock(&mtx_msg);
}

void adm_cmd_kick(const char *ptr) {
	int i = atoi(ptr);
	pthread_mutex_lock(&mtx_user);
	if (i <= 0 || i >= sv_cfg.next_uid) {
		printerr("Invalid ID.", PERR_WARNING, FALSE);
	} else {
		// Check user
		// if valid force disconnect user
		int ind = get_uindex_by_id(i);
		if (ind != -1) {
			COMMAND cmd;
			cmd.cmd = CMD_FDC;
			strncpy(cmd.From, sv_fifo, MAX_FIFO);
			write(sv_cfg.users[ind].user_fd, &cmd, sizeof(COMMAND));
			rem_user2(ind);
			printerr("User kicked.", PERR_INFO, FALSE);
		} else {
			printerr("Invalid ID.", PERR_WARNING, FALSE);
		}
	}
	pthread_mutex_unlock(&mtx_user);
}

int adm_cmd_verify(const char *ptr, int sv) {
	if (sv_cfg.use_filter == 1) {
		char bwc[20];
		char *bwc2;
		write(sv_cfg.sv_verificador_pipes[1], ptr, strlen(ptr));
		write(sv_cfg.sv_verificador_pipes[1], "\n", strlen("\n"));
		write(sv_cfg.sv_verificador_pipes[1], "##MSGEND##\n", strlen("##MSGEND##\n"));
		read(sv_cfg.sv_verificador_pipes[0], bwc, 20);
		// Strtok because 'verificador' sends garbage after the number... for some reason...???
		bwc2 = strtok(bwc, "\n");
		int bwci = atoi(bwc2);
		if (sv == 1) {
			printf("[Server] Bad word count: %d\n", bwci);
			if (bwci > sv_cfg.maxnot) {
				printf("[Server] Invalid message.\n");
			} else {
				printf("[Server] Valid message.\n");
			}
		} else {
			// Not an adm command
			// return number of bad words so that later on, the server can "delete message"
			// and warn client if (bwci > sv_cfg.maxnot)
			return bwci;
		}
	} else if (sv_cfg.use_filter == 0 && sv == 1) {
		// Testing filter and 'verificador' before client-server connection
		// No need for this anymore. But it will stay here anyway
		printf("Word filter is disabled. Please enable word filter with 'filter on'.\n");
	}
	return 0;
}

void adm_cmd_cfg() {
	printf("\tMAXMSG: %d\n", sv_cfg.maxmsg);
	printf("\tMAXNOT: %d\n", sv_cfg.maxnot);
	printf("\tWORDSNOT: %s\n", sv_cfg.wordsnot);
	pthread_mutex_lock(&mtx_user);
	printf("\tCurrent users: %d\n", count_users());
	pthread_mutex_unlock(&mtx_user);
	pthread_mutex_lock(&mtx_topic);
	printf("\tCurrent topics: %d\n", count_topics());
	pthread_mutex_unlock(&mtx_topic);
	pthread_mutex_lock(&mtx_msg);
	printf("\tCurrent messages: %d\n", sv_cfg.n_msgs);
	pthread_mutex_unlock(&mtx_msg);
	printf("\tServer PID: %d\n", getpid());
	printf("\t'Verificador' PID: %d\n", sv_cfg.sv_verificador_pid);
}

void adm_cmd_view(const char *ptr) {
	int mid = atoi(ptr);
	if (mid <= 0 || mid >= sv_cfg.next_mid) {
		printerr("Invalid ID.", PERR_WARNING, FALSE);
	} else {
		pthread_mutex_lock(&mtx_msg);
		int mind = get_mindex_by_id(mid);
		if (mind != -1) {
			printf("\tID: %d\n\tTime left: %d\n\tAuthor: %s\n\tTopic: %s\n\tTitle: %s\n\tBody:\n%s\n",
			       sv_cfg.msgs[mind].id, sv_cfg.msgs[mind].msg.Duration, sv_cfg.msgs[mind].msg.Author,
			       sv_cfg.msgs[mind].msg.Topic, sv_cfg.msgs[mind].msg.Title, sv_cfg.msgs[mind].msg.Body);
		} else {
			printerr("Invalid ID.", PERR_WARNING, FALSE);
		}
		pthread_mutex_unlock(&mtx_msg);
	}
}

void adm_cmd_subs(const char *ptr) {
	int uid = atoi(ptr);
	if (uid <= 0 || uid >= sv_cfg.next_mid) {
		printerr("Invalid ID.", PERR_WARNING, FALSE);
	} else {
		pthread_mutex_lock(&mtx_topic);
		pthread_mutex_lock(&mtx_user);
		int uind = get_uindex_by_id(uid);
		if (uind != -1) {
			int tind;
			printf("\tUser is subscribed to %d topics.\n", count_subs(uid));
			for (int i = 0; i < sv_cfg.users[uind].sub_size; i++) {
				tind = get_tindex_by_id(sv_cfg.users[uind].topic_ids[i]);
				if (sv_cfg.topics[tind].id != -1)
					printf("\tID: %d - '%s'\n", sv_cfg.topics[tind].id, sv_cfg.topics[tind].topic);
			}
		} else {
			printerr("Invalid ID.", PERR_WARNING, FALSE);
		}
		pthread_mutex_unlock(&mtx_user);
		pthread_mutex_unlock(&mtx_topic);
	}
}

// ********************** ADMINISTRATOR COMMAND FUNCTIONS END

// ********************** SV_USER FUNCTIONS

// Not thread safe
// Use after locking with mutex

// Counts users with ID != -1
int count_users() {
	int tmp = 0;
	for (int i = 0; i < sv_cfg.users_size; i++) {
		if (sv_cfg.users[i].id != -1) {
			tmp++;
		}
	}
	return tmp;
}

// Reallocates memory if size is bigger than (previously allocated space - 3) or if size is smaller than (previously allocated space - 14)
// If count_users = 9 and previously allocated space = 10
// Then 9 >= 10 - 3 == true
// If count_users = 4 and previously allocated space = 20
// Then 4 <= 20 - 14 == true
void resize_users() {
	int u = count_users();
	sort_users();
	if (u >= (sv_cfg.users_size - 3)) {
		SV_USER *tmp = realloc(sv_cfg.users, (sv_cfg.users_size + 10) * sizeof(SV_USER));
		if (tmp == NULL) {
			printerr("Getting low on memory for users.", PERR_WARNING, TRUE);
		} else {
			sv_cfg.users = tmp;
			for (int i = sv_cfg.users_size; i < (sv_cfg.users_size + 10); i++) {
				sv_cfg.users[i].id = -1;
				sv_cfg.users[i].alive = 0;
				sv_cfg.users[i].sub_size = INIT_MALLOC_SIZE;
				sv_cfg.users[i].topic_ids = NULL;
			}
			sv_cfg.users_size += 10;
		}
	} else if ((sv_cfg.users_size != INIT_MALLOC_SIZE) && (u <= (sv_cfg.users_size - 14))) {
		SV_USER *tmp = realloc(sv_cfg.users, (sv_cfg.users_size - 10) * sizeof(SV_USER));
		if (tmp == NULL) {
			printerr("Getting low on memory for users.", PERR_WARNING, TRUE);
		} else {
			sv_cfg.users = tmp;
			sv_cfg.users_size -= 10;
		}
	}
}

// Switches first user.id = -1 with last user.id != -1
// Unsorted:    [0, -1, -1, 1,  2,  4,  -1, 5,  7,  10, -1]
// Sorted:      [0, 10, 7,  1,  2,  4,  5,  -1, -1, -1, -1]
void sort_users() {
	SV_USER tmp;
	for (int i = 0; i < sv_cfg.users_size; i++) {
		if (sv_cfg.users[i].id == -1) {
			for (int j = sv_cfg.users_size - 1; j > i; j--) {
				if (sv_cfg.users[j].id != -1) {
					tmp = sv_cfg.users[i];
					sv_cfg.users[i] = sv_cfg.users[j];
					sv_cfg.users[j] = tmp;
					break;
				}
			}
		}
	}
}

int get_next_u_index() {
	return get_uindex_by_id(-1);
}

int add_user(USER user) {
	resize_users();
	int d = get_next_u_index();
	int n = get_uindex_by_username(user.Username);
	if (d == -1 || n != -1) {
		// No space
		// or username already exists
		return -1;
	} else  {
		// Add user to the index
		//free(sv_cfg.users[d].topic_ids);
		sv_cfg.users[d].id = sv_cfg.next_uid;
		sv_cfg.users[d].alive = 0;
		strncpy(sv_cfg.users[d].user.Username, user.Username, MAX_USER);
		strncpy(sv_cfg.users[d].user.FIFO, user.FIFO, MAX_FIFO);
		//sv_cfg.users[d].user = user;
		sv_cfg.users[d].user_fd = open(sv_cfg.users[d].user.FIFO, O_WRONLY);
		sv_cfg.users[d].sub_size = INIT_MALLOC_SIZE;
		sv_cfg.users[d].topic_ids = malloc(INIT_MALLOC_SIZE * sizeof(int));
		for (int i = 0; i < sv_cfg.users[d].sub_size; i++) {
			sv_cfg.users[d].topic_ids[i] = -1;
		}
		sv_cfg.next_uid++;
		return sv_cfg.users[d].id;
	}
}

void rem_user(const char *FIFO) {
	int ind = get_uindex_by_FIFO(FIFO);
	if (ind != -1) {
		rem_user2(ind);
	}
}

void rem_user2(int ind) {
	if (ind < 0 || ind >= sv_cfg.users_size)
		return;

	sv_cfg.users[ind].id = -1;
	sv_cfg.users[ind].alive = 0;
	sv_cfg.users[ind].sub_size = INIT_MALLOC_SIZE;
	free(sv_cfg.users[ind].topic_ids);
	close(sv_cfg.users[ind].user_fd);
	sv_cfg.users[ind].user_fd = 0;
}

int get_uindex_by_id(int id) {
	for (int i = 0; i < sv_cfg.users_size; i++) {
		if (sv_cfg.users[i].id == id) {
			return i;
		}
	}
	return -1;
}

int get_uindex_by_FIFO(const char *FIFO) {
	for (int i = 0; i < sv_cfg.users_size; i++) {
		if (sv_cfg.users[i].id != -1 && strcmp(sv_cfg.users[i].user.FIFO, FIFO) == 0) {
			return i;
		}
	}
	return -1;
}

int get_uindex_by_username(const char *username) {
	for (int i = 0; i < sv_cfg.users_size; i++) {
		if (sv_cfg.users[i].id != -1 && strcmp(sv_cfg.users[i].user.Username, username) == 0) {
			return i;
		}
	}
	return -1;
}

int get_uindex_by_fd(int fd) {
	for (int i = 0; i < sv_cfg.users_size; i++) {
		if (sv_cfg.users[i].user_fd == fd) {
			return i;
		}
	}
	return -1;
}

void subscribe(const char *FIFO, int topic_id) {
	int tind = get_tindex_by_id(topic_id);
	int uind = get_uindex_by_FIFO(FIFO);
	COMMAND s_cmd;
	snprintf(s_cmd.From, MAX_FIFO, "%s", sv_fifo);
	if (uind != -1) {
		if (tind != -1) {
			if (is_user_already_subbed(sv_cfg.users[uind].id, sv_cfg.topics[tind].id) != -1) {
				// Send CMD_ERR
				// Already subscribed
				s_cmd.cmd = CMD_ERR;
				snprintf(s_cmd.Body.un_topic, MAX_TPCTTL, "User is already subscribed to this topic.");
				write(sv_cfg.users[uind].user_fd, &s_cmd, sizeof(COMMAND));
			} else {
				// Add topic to subbed list
				resize_subs(sv_cfg.users[uind].id);
				int nstid = get_next_st_index(sv_cfg.users[uind].id);
				sv_cfg.users[uind].topic_ids[nstid] = topic_id;
				s_cmd.cmd = CMD_ERR;
				snprintf(s_cmd.Body.un_topic, MAX_TPCTTL, "You have subscribed to topic ID: %d.", topic_id);
				write(sv_cfg.users[uind].user_fd, &s_cmd, sizeof(COMMAND));
			}
		} else {
			// Topic doesn't exist
			s_cmd.cmd = CMD_ERR;
			snprintf(s_cmd.Body.un_topic, MAX_TPCTTL, "Topic does not exist.");
			write(sv_cfg.users[uind].user_fd, &s_cmd, sizeof(COMMAND));
		}
	} else {
		// Send CMD_ERR
		// User not connected
		s_cmd.cmd = CMD_ERR;
		snprintf(s_cmd.Body.un_topic, MAX_TPCTTL, "User not connect or has timed out.");
		int tmp_fd = open(FIFO, O_WRONLY);
		write(tmp_fd, &s_cmd, sizeof(COMMAND));
		close(tmp_fd);
	}
}

void unsubscribe(const char *FIFO, int topic_id) {
	int tind = get_tindex_by_id(topic_id);
	int uind = get_uindex_by_FIFO(FIFO);
	int tu_ind;
	COMMAND s_cmd;
	snprintf(s_cmd.From, MAX_FIFO, "%s", sv_fifo);
	if (uind != -1) {
		if (tind != -1) {
			if ((tu_ind = is_user_already_subbed(sv_cfg.users[uind].id, sv_cfg.topics[tind].id)) != -1) {
				// Is subscribed, then unsubscribe
				sv_cfg.users[uind].topic_ids[tu_ind] = -1;
				resize_subs(sv_cfg.users[uind].id);
				s_cmd.cmd = CMD_ERR;
				snprintf(s_cmd.Body.un_topic, MAX_TPCTTL, "You have unsubscribed from topic ID: %d.", topic_id);
				write(sv_cfg.users[uind].user_fd, &s_cmd, sizeof(COMMAND));
			} else {
				// Send CMD_ERR
				// User not subscribed to this topic
				s_cmd.cmd = CMD_ERR;
				snprintf(s_cmd.Body.un_topic, MAX_TPCTTL, "User not subscribed to this topic.");
				write(sv_cfg.users[uind].user_fd, &s_cmd, sizeof(COMMAND));
			}
		} else {
			// Topic doesn't exist
			s_cmd.cmd = CMD_ERR;
			snprintf(s_cmd.Body.un_topic, MAX_TPCTTL, "Topic does not exist.");
			write(sv_cfg.users[uind].user_fd, &s_cmd, sizeof(COMMAND));
		}
	} else {
		// Send CMD_ERR
		// User not connected
		s_cmd.cmd = CMD_ERR;
		snprintf(s_cmd.Body.un_topic, MAX_TPCTTL, "User not connect or has timed out.");
		int tmp_fd = open(FIFO, O_WRONLY);
		write(tmp_fd, &s_cmd, sizeof(COMMAND));
		close(tmp_fd);
	}
}

int get_next_st_index(int uid) {
	// Get next subbed topic index
	int ind = get_uindex_by_id(uid);
	for (int i = 0; i < sv_cfg.users[ind].sub_size; i++) {
		if (sv_cfg.users[ind].topic_ids[i] == -1)
			return i;
	}
	return -1;
}

int count_subs(int uid) {
	// Count how many subscriptions the user has
	int tmp = 0;
	int ind = get_uindex_by_id(uid);
	if (ind != -1) {
		for (int i = 0; i < sv_cfg.users[ind].sub_size; i++) {
			if (sv_cfg.users[ind].topic_ids[i] != -1) {
				tmp++;
			}
		}
	}
	return tmp;
}

void resize_subs(int uid) {
	// Resize user subscriptions array
	int uind = get_uindex_by_id(uid);
	int u = count_subs(uid);
	sort_subs(uid);
	if (u >= (sv_cfg.users[uind].sub_size - 3)) {
		int *tmp = realloc(sv_cfg.users[uind].topic_ids, (sv_cfg.users[uind].sub_size + 10) * sizeof(int));
		if (tmp == NULL) {
			printerr("Getting low on memory for subscribed topics.", PERR_WARNING, TRUE);
		} else {
			sv_cfg.users[uind].topic_ids = tmp;
			for (int i = sv_cfg.users[uind].sub_size; i < (sv_cfg.users[uind].sub_size + 10); i++) {
				sv_cfg.users[uind].topic_ids[i] = -1;
			}
			sv_cfg.users[uind].sub_size += 10;
		}
	} else if ((sv_cfg.users[uind].sub_size != INIT_MALLOC_SIZE) && (u <= (sv_cfg.users[uind].sub_size - 14))) {
		int *tmp = realloc(sv_cfg.users[uind].topic_ids, (sv_cfg.users[uind].sub_size - 10) * sizeof(int));
		if (tmp == NULL) {
			printerr("Getting low on memory for subscribed topics.", PERR_WARNING, TRUE);
		} else {
			sv_cfg.users[uind].topic_ids = tmp;
			sv_cfg.users[uind].sub_size -= 10;
		}
	}
}

void sort_subs(int uid) {
	// Sort subscriptions
	int uind = get_uindex_by_id(uid);
	int tmp;
	for (int i = 0; i < sv_cfg.users[uind].sub_size; i++) {
		if (sv_cfg.users[uind].topic_ids[i] == -1) {
			for (int j = sv_cfg.users[uind].sub_size - 1; j > i; j--) {
				if (sv_cfg.users[uind].topic_ids[j] != -1) {
					tmp = sv_cfg.users[uind].topic_ids[i];
					sv_cfg.users[uind].topic_ids[i] = sv_cfg.users[uind].topic_ids[j];
					sv_cfg.users[uind].topic_ids[j] = tmp;
					break;
				}
			}
		}
	}
}

int is_user_already_subbed(int uid, int tid) {
	// Is user already subscribed to certain topic ID?
	int uind = get_uindex_by_id(uid);
	for (int i = 0; i < sv_cfg.users[uind].sub_size; i++) {
		if (tid == sv_cfg.users[uind].topic_ids[i]) {
			return i;
		}
	}
	return -1;
}

// Gets user file descriptors of whoever is subscribed to the topic
// Return: array with user file descriptors
// *num returns number of users
int *get_ufd_subbed_to(const char *topic, int *num) {
	(*num) = 0;
	int size = 1;
	int *tmp = malloc(sizeof(int) * size);
	if (tmp != NULL) {
		int topicid = sv_cfg.topics[get_tindex_by_topic_name(topic)].id;
		for (int i = 0; i < sv_cfg.users_size; i++) {
			if (sv_cfg.users[i].id != -1) {
				for (int j = 0; j < sv_cfg.users[i].sub_size; j++) {
					if (sv_cfg.users[i].topic_ids[j] == topicid) {
						tmp[size - 1] = sv_cfg.users[i].user_fd;
						size++;
						int *tmp2 = realloc(tmp, sizeof(int) * size);
						if (tmp2 != NULL) {
							tmp = tmp2;
							(*num)++;
						}
					}
				}
			}
		}
	}
	return tmp;
}

// ********************** SV_USER FUNCTIONS END

// ********************** SV_MSG FUNCTIONS

// Not thread safe
// Use after locking with mutex

int get_next_m_index() {
	return get_mindex_by_id(-1);
}

int get_mindex_by_id(int id) {
	for (int i = 0; i < sv_cfg.maxmsg; i++) {
		if (id == sv_cfg.msgs[i].id) {
			return i;
		}
	}
	return -1;
}

int get_mindex_by_message(MESSAGE msg) {
	for (int i = 0; i < sv_cfg.maxmsg; i++) {
		if (sv_cfg.msgs[i].id != -1
		        && (strcmp(msg.Author, sv_cfg.msgs[i].msg.Author) == 0)
		        && (strcmp(msg.Body, sv_cfg.msgs[i].msg.Body) == 0)
		        && (strcmp(msg.Title, sv_cfg.msgs[i].msg.Title) == 0)
		        && (strcmp(msg.Topic, sv_cfg.msgs[i].msg.Topic) == 0)) {
			return i;
		}
	}
	return -1;
}

void add_msg(MESSAGE msg) {
	int m_ind = get_next_m_index();
	if (m_ind != -1) {
		// Add the message
		sv_cfg.msgs[m_ind].id = sv_cfg.next_mid;
		sv_cfg.msgs[m_ind].msg.Duration = msg.Duration;
		strncpy(sv_cfg.msgs[m_ind].msg.Author, msg.Author, MAX_USER);
		strncpy(sv_cfg.msgs[m_ind].msg.Body, msg.Body, MAX_BODY);
		strncpy(sv_cfg.msgs[m_ind].msg.Title, msg.Title, MAX_TPCTTL);
		strncpy(sv_cfg.msgs[m_ind].msg.Topic, msg.Topic, MAX_TPCTTL);
		sv_cfg.next_mid++;
		sv_cfg.n_msgs++;
	}
}

int rem_msg(int id) {
	int ind = get_mindex_by_id(id);
	if (ind != -1) {
		sv_cfg.msgs[ind].id = -1;
		sv_cfg.msgs[ind].msg.Duration = 0;
		sv_cfg.n_msgs--;
		return 1;
	}
	return 0;
}

int count_msg() {
	int tmp = 0;
	for (int i = 0; i < sv_cfg.maxmsg; i++) {
		if (sv_cfg.msgs[i].id != -1) {
			tmp++;
		}
	}
	return tmp;
}

// ********************** SV_MSG FUNCTIONS END

// ********************** SV_TOPIC FUNCTIONS

// Not thread safe
// Use after locking with mutex

int get_next_t_index() {
	return get_tindex_by_id(-1);
}

void add_topic(const char *topic) {
	resize_topics();
	int t_ind = get_next_t_index();
	if (t_ind != -1) {
		sv_cfg.topics[t_ind].id = sv_cfg.next_tid;
		strncpy(sv_cfg.topics[t_ind].topic, topic, MAX_TPCTTL);
		sv_cfg.next_tid++;
	}
}

void rem_topic(int id) {
	int t_ind = get_tindex_by_id(id);
	if (t_ind != -1) {
		sv_cfg.topics[t_ind].id = -1;
	}
}

int get_tindex_by_id(int id) {
	for (int i = 0; i < sv_cfg.topic_size; i++) {
		if (sv_cfg.topics[i].id == id) {
			return i;
		}
	}
	return -1;
}

int get_tindex_by_topic_name(const char *t_name) {
	for (int i = 0; i < sv_cfg.topic_size; i++) {
		if (sv_cfg.topics[i].id != -1 && strcmp(t_name, sv_cfg.topics[i].topic) == 0) {
			return i;
		}
	}
	return -1;
}

// Same as resize users but for topics
void resize_topics() {
	int u = count_topics();
	sort_topics();
	if (u >= (sv_cfg.topic_size - 3)) {
		SV_TOPIC *tmp = realloc(sv_cfg.topics, (sv_cfg.topic_size + 10) * sizeof(SV_TOPIC));
		if (tmp == NULL) {
			printerr("Getting low on memory for topics.", PERR_WARNING, TRUE);
		} else {
			sv_cfg.topics = tmp;
			for (int i = sv_cfg.topic_size; i < (sv_cfg.topic_size + 10); i++) {
				sv_cfg.topics[i].id = -1;
			}
			sv_cfg.topic_size += 10;
		}
	} else if ((sv_cfg.topic_size != INIT_MALLOC_SIZE) && (u <= (sv_cfg.topic_size - 14))) {
		SV_TOPIC *tmp = realloc(sv_cfg.topics, (sv_cfg.topic_size - 10) * sizeof(SV_TOPIC));
		if (tmp == NULL) {
			printerr("Getting low on memory for topics.", PERR_WARNING, TRUE);
		} else {
			sv_cfg.topics = tmp;
			sv_cfg.topic_size -= 10;
		}
	}
}

// Same as sort users but for topics
void sort_topics() {
	SV_TOPIC tmp;
	for (int i = 0; i < sv_cfg.topic_size; i++) {
		if (sv_cfg.topics[i].id == -1) {
			for (int j = sv_cfg.topic_size - 1; j > i; j--) {
				if (sv_cfg.topics[j].id != -1) {
					tmp = sv_cfg.topics[i];
					sv_cfg.topics[i] = sv_cfg.topics[j];
					sv_cfg.topics[j] = tmp;
					break;
				}
			}
		}
	}
}

// Same as count users but for topics
int count_topics() {
	int tmp = 0;
	for (int i = 0; i < sv_cfg.topic_size; i++) {
		if (sv_cfg.topics[i].id != -1) {
			tmp++;
		}
	}
	return tmp;
}

// ********************** SV_TOPIC FUNCTIONS END
