#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()`"
#define HISTFILESIZE=20

int flag = 0;
int hangup = 0;
char cmd[100];
/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */
int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}
	if (*s == 0 || *s == '#') {
		return 0;
	}
	if (*s == '\"') {
		*s = 0;
		s++;
		*p1 = s;
		while(*s != 0 && *s != '\"') {
			s++;
		}
		*s = 0;
		s++;
		*p2 = s;
		return 'w';
	}
	if (*s == '`') {
		char *p = s;
		p++;
		while (*p != '`') {
			p++;
		}
		*p = ' ';
	}
	if (strchr(SYMBOLS, *s)) {
		int t = *s;
		*p1 = s;
		if (*s == *(s+1)) {
			*s++ = 0;
			flag = 1;
		} else {
			flag = 0;
		}
		*s++ = 0;
		*p2 = s;
		return t;
	}

	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s)) {
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1) {
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 128

int parsecmd(char **argv, int *rightpipe) {
	int argc = 0;
	while (1) {
		char *t;
		int fd, r;
		int c = gettoken(0, &t);
		switch (c) {
		case 0:
			return argc;
		case 'w':
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit(1);
			}
			argv[argc++] = t;
			break;
		case ';':
			r = fork();
			if (r) {
				wait(r);
				return parsecmd(argv, rightpipe);
			} else {
				return argc;
			}
			break;
		case '<':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit(1);
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (1/3) */
			fd=open(t,O_RDONLY);
			if (fd < 0) {
				debugf("failed to open '%s'\n",t);
				exit(1);
			}
			dup(fd,0);
			close(fd);
			//user_panic("< redirection not implemented");

			break;
		case '>':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit(1);
			}
			// Open 't' for writing, create it if not exist and trunc it if exist, dup
			// it onto fd 1, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (2/3) */
			if (flag == 1) {
				fd=open(t, O_WRONLY | O_CREAT);
				if (fd < 0) {
					debugf("failed to open '%s'\n",t);
					exit(1);
				}
				struct Stat st;
				stat(t, &st);
				seek(fd, st.st_size);
			} else {
				fd=open(t, O_WRONLY | O_CREAT | O_TRUNC);
				if (fd < 0) {
					debugf("failed to open '%s'\n",t);
					exit(1);
				}
			}
			dup(fd,1);
			close(fd);
			//user_panic("> redirection not implemented");

			break;
		case '&':
			if (flag == 1) {
				r = fork();
				if (r) {
					if ((r = (syscall_ipc_recv(0)) < 0)) {
						return r;
					}
					int value = env -> env_ipc_value;
					if (value == 1) {
						c = gettoken(0, &t);
						while (c != '|' || flag != 1) {
							if (c == 0) break;
							c = gettoken(0, &t);
						}
					}
					return parsecmd(argv, rightpipe);
				} else {
					return argc;
				}
				break;
			}
			r = fork();
			if (r) {
				hangup = 1;
				syscall_add_job(r, cmd);
				return parsecmd(argv, rightpipe);
			} else {
				hangup = 0;
				// int index = 0;
				// char cmd[1000];
				// strcpy(cmd, "");
				// for (int i = 0; i < argc; i++) {
				// 	if (i > 0) {
				// 		cmd[index++] = ' ';
				// 	}
				// 	for (int j = 0; j < strlen(argv[i]); j++) {
				// 		cmd[index++] = argv[i][j];
				// 	}
				// }
				// cmd[index++] = ' ';
				// cmd[index++] = '&';
				return argc;
			}
			break;
		case '`':
			r = fork();
			if (r) {
				wait(r);
				return argc;
			} else {
				return parsecmd(argv, rightpipe);
			}
			break;
		case '|':;
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
			 *   command line.
			 * The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			if (flag == 1) {
				r = fork();
				if (r) {
					if ((r = (syscall_ipc_recv(0)) < 0)) {
						return r;
					}
					int value = env -> env_ipc_value;
					if (value == 0) {
						c = gettoken(0, &t);
						while (c != '&' || flag != 1) {
							if (c == 0) break;
							c = gettoken(0, &t);
						}
					}
					return parsecmd(argv, rightpipe);
				} else {
					return argc;
				}
				break;
			}
			int p[2];
			/* Exercise 6.5: Your code here. (3/3) */
			pipe(p);
			*rightpipe=fork();
			if(*rightpipe==0)
			{
				dup(p[0],0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv,rightpipe);
			}
			else{
				dup(p[1],1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			//user_panic("| not implemented");
			break;
		}
	}

	return argc;
}

void save_history(char *cmd) {
	int fd;
	if ((fd = open(".mosh_history", O_CREAT | O_WRONLY )) < 0){
		return;
	}
	struct Stat st;
	stat(".mosh_history", &st);
	seek(fd, st.st_size);
	write(fd, cmd, strlen(cmd));
	write(fd, "\n", 1);
	close(fd);
	return;
}

void print_history() {
	int r;
	if ((r = open(".mosh_history", O_RDONLY)) < 0){
			return;
	}
	char buf;
	while (read(r, &buf, 1)) {
		fprintf(1, "%c", buf);
	}
	close(r);
}

void runcmd(char *s) {
	save_history(s);
	strcpy(cmd, s);
	gettoken(s, 0);

	char *argv[MAXARGS];
	int rightpipe = 0;
	int argc = parsecmd(argv, &rightpipe);
	if (argc == 0) {
		return;
	}
	argv[argc] = 0;
	if (strcmp(argv[0], "fg") == 0) {
		int fgId = 0;
		char *str = argv[1];
		while (*str >= '0' && *str <= '9') {
        	fgId = fgId * 10 + (*str - '0');
        	str++;
		}
		// fprintf(1, "%d", fgId);
		int envid = syscall_fg_job(fgId);
		// fprintf(1, "%x" , envid);
		wait(envid);
		exit(0);
		return;
	} else if (strcmp(argv[0], "kill") == 0) {
		int killId = 0;
		char *str = argv[1];
		while (*str >= '0' && *str <= '9') {
        	killId = killId * 10 + (*str - '0');
        	str++;
		}
		int envid = syscall_kill_job(killId);
		exit(0);
		return; 
    } else if (strcmp(argv[0], "jobs") == 0) {
		syscall_print_job();
		exit(0);
		return;	
	} else if (strcmp(argv[0], "history") == 0) {
		print_history();
		exit(0);
		return;
	} 
	int child = spawn(argv[0], argv);
	close_all();
	if (child >= 0) {
		if (hangup == 0) {
			wait(child);
		}
	} else {
		debugf("spawn %s: %d\n", argv[0], child);
	}
	if (rightpipe) {
		wait(rightpipe);
	}
	exit(0);
}

void readline(char *buf, u_int n) {
	int r;
	for (int i = 0; i < n; i++) {
		if ((r = read(0, buf + i, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
			exit(1);
		}
		if (buf[i] == '\b' || buf[i] == 0x7f) {
			if (i > 0) {
				i -= 2;
			} else {
				i = -1;
			}
			if (buf[i] != '\b') {
				printf("\b");
			}
		}
		if (buf[i] == '\r' || buf[i] == '\n') {
			buf[i] = 0;
			return;
		}
	}
	debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
		;
	}
	buf[0] = 0;
}

char buf[1024];

void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	exit(0);
}

int main(int argc, char **argv) {
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                 LGQ MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}
	for (;;) {
		if (interactive) {
			printf("\n$ ");
		}
		readline(buf, sizeof buf);

		if (buf[0] == '#') {
			continue;
		}
		if (echocmds) {
			printf("# %s\n", buf);
		}
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}
		if (r == 0) {
			runcmd(buf);
			exit(0);
		} else {
			wait(r);
		}
	}
	return 0;
}
