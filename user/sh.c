#include <inc/lib.h>

// SHELL-PATH variable
// Accessible by all shell commands
char* PATH = (char*)PATH_VA;	// PROJECT

#define BUFSIZ 1024		/* Find the buffer overrun bug! */
int debug = 0;

// gettoken(s, 0) prepares gettoken for subsequent calls and returns 0.
// gettoken(0, token) parses a shell token from the previously set string,
// null-terminates that token, stores the token pointer in '*token',
// and returns a token ID (0, '<', '>', '|', or 'w').
// Subsequent calls to 'gettoken(0, token)' will return subsequent
// tokens from the string.
int gettoken(char *s, char **token);


// Parse a shell command from string 's' and execute it.
// Do not return until the shell command is finished.
// runcmd() is called in a forked child,
// so it's OK to manipulate file descriptor state.
#define MAXARGS 16
void
runcmd(char* s)
{
	char *argv[MAXARGS], *t, argv0buf[BUFSIZ];
	int argc, c, i, r, p[2], fd, pipe_child;
	char path[MAXPATHLEN];	// PROJECT

	pipe_child = 0;
	gettoken(s, 0);

again:
	argc = 0;
	while (1) {
		switch ((c = gettoken(0, &t))) {

		case 'w':	// Add an argument
			if (argc == MAXARGS) {
				cprintf("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;

		case '<':	// Input redirection
			// Grab the filename from the argument list
			if (gettoken(0, &t) != 'w') {
				cprintf("syntax error: < not followed by word\n");
				exit();
			}
			// Open 't' for reading as file descriptor 0
			// (which environments use as standard input).
			// We can't open a file onto a particular descriptor,
			// so open the file as 'fd',
			// then check whether 'fd' is 0.
			// If not, dup 'fd' onto file descriptor 0,
			// then close the original 'fd'.

			// LAB 5: Your code here.
			if((fd = open(t, O_RDONLY)) < 0){

				cprintf("open %s for read: %e", t, fd);
				exit();
			}
			if(fd != 0){
				dup(fd, 0);
				close(fd);
			}
			break;

		case '>':	// Output redirection
			// Grab the filename from the argument list
			if((c = gettoken(0, &t)) == '>'){	// PROJECT
				// redirection eith append
				if(gettoken(0, &t) != 'w'){
					cprintf("PROJECT: syntax error: >> not followed by word\n");
					exit();
				}
				if(t[0] == '/')
					// user pass absolute path
					strcpy(path, t);
				else{
					// user pass relative path
					strcpy(path, PATH);
					if(strlen(path) > 1)
						strcat(path, "/");
					strcat(path, t);
				}
				if((fd = open(path, O_WRONLY | O_APPEND)) < 0){
					cprintf("PROJECT: open %s for write: %e", path, fd);
					exit();
				}
			}
			else{ 
				if (c != 'w') {
					cprintf("syntax error: > not followed by word\n");
					exit();
				}
				if(t[0] == '/')		// PROJECT
					// user pass absolute path
					strcpy(path, t);
				else{
					// user pass relative path
					strcpy(path, PATH);
					if(strlen(path) > 1)
						strcat(path, "/");
					strcat(path, t);
				}
				if ((fd = open(path, O_WRONLY | O_TRUNC)) < 0) {
					cprintf("open %s for write: %e", path, fd);
					exit();
				}
			}
			if (fd != 1) {
				dup(fd, 1);
				close(fd);
			}
			break;

		case '|':	// Pipe
			if ((r = pipe(p)) < 0) {
				cprintf("pipe: %e", r);
				exit();
			}
			if (debug)
				cprintf("PIPE: %d %d\n", p[0], p[1]);
			if ((r = fork()) < 0) {
				cprintf("fork: %e", r);
				exit();
			}
			if (r == 0) {
				if (p[0] != 0) {
					dup(p[0], 0);
					close(p[0]);
				}
				close(p[1]);
				goto again;
			} else {
				pipe_child = r;
				if (p[1] != 1) {
					dup(p[1], 1);
					close(p[1]);
				}
				close(p[0]);
				goto runit;
			}
			break;

		case 0:		// String is complete
			// Run the current command!
			goto runit;

		default:
			panic("bad return %d from gettoken", c);
			break;

		}
	}

runit:
	// Return immediately if command line was empty.
	if(argc == 0) {
		if (debug)
			cprintf("EMPTY COMMAND\n");
		return;
	}
	// Clean up command line.
	// Read all commands from the filesystem: add an initial '/' to
	// the command name.
	// This essentially acts like 'PATH=/'.
	if (argv[0][0] != '/') {
		argv0buf[0] = '/';
		strcpy(argv0buf + 1, argv[0]);
		argv[0] = argv0buf;
	}
	argv[argc] = 0;

	// Print the command.
	if (debug) {
		cprintf("[%08x] SPAWN:", thisenv->env_id);
		for (i = 0; argv[i]; i++)
			cprintf(" %s", argv[i]);
		cprintf("\n");
	}

	// Spawn the command!
	if ((r = spawn(argv[0], (const char**) argv)) < 0)
		cprintf("spawn %s: %e\n", argv[0], r);
	// In the parent, close all file descriptors and wait for the
	// spawned command to exit.
	close_all();
	if (r >= 0) {
		if (debug)
			cprintf("[%08x] WAIT %s %08x\n", thisenv->env_id, argv[0], r);
		wait(r);
		if (debug)
			cprintf("[%08x] wait finished\n", thisenv->env_id);
	}

	// If we were the left-hand part of a pipe,
	// wait for the right-hand part to finish.
	if (pipe_child) {
		if (debug)
			cprintf("[%08x] WAIT pipe_child %08x\n", thisenv->env_id, pipe_child);
		wait(pipe_child);
		if (debug)
			cprintf("[%08x] wait finished\n", thisenv->env_id);
	}

	// Done!
	exit();
}


// Get the next token from string s.
// Set *p1 to the beginning of the token and *p2 just past the token.
// Returns
//	0 for end-of-string;
//	< for <;
//	> for >;
//	| for |;
//	w for a word.
//
// Eventually (once we parse the space where the \0 will go),
// words get nul-terminated.
#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

int
_gettoken(char *s, char **p1, char **p2)
{
	int t;

	if (s == 0) {
		if (debug > 1)
			cprintf("GETTOKEN NULL\n");
		return 0;
	}

	if (debug > 1)
		cprintf("GETTOKEN: %s\n", s);

	*p1 = 0;
	*p2 = 0;

	while (strchr(WHITESPACE, *s))
		*s++ = 0;
	if (*s == 0) {
		if (debug > 1)
			cprintf("EOL\n");
		return 0;
	}
	if (strchr(SYMBOLS, *s)) {
		t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		if (debug > 1)
			cprintf("TOK %c\n", t);
		return t;
	}
	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s))
		s++;
	*p2 = s;
	if (debug > 1) {
		t = **p2;
		**p2 = 0;
		cprintf("WORD: %s\n", *p1);
		**p2 = t;
	}
	return 'w';
}

int
gettoken(char *s, char **p1)
{
	static int c, nc;
	static char* np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}


void
usage(void)
{
	cprintf("usage: sh [-dix] [command-file]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int r, interactive, echocmds;
	struct Argstate args;

	interactive = '?';
	echocmds = 0;
	argstart(&argc, argv, &args);
	while ((r = argnext(&args)) >= 0)
		switch (r) {
		case 'd':
			debug++;
			break;
		case 'i':
			interactive = 1;
			break;
		case 'x':
			echocmds = 1;
			break;
		default:
			usage();
		}

	if (argc > 2)
		usage();
	if (argc == 2) {
		close(0);
		if ((r = open(argv[1], O_RDONLY)) < 0)
			panic("open %s: %e", argv[1], r);
		assert(r == 0);
	}
	if (interactive == '?')
		interactive = iscons(0);

	// Alloc a page that will be shared with every shell child
	// This page will keep the current PATH and will update by cd command
	if((r = sys_page_alloc(thisenv->env_id, PATH_VA, PTE_U | PTE_P | PTE_W | PTE_SHARE)) < 0) // PROJECT
		panic("init_PATH: sys_page_alloc return %e\n", r);

	// init PATH to root
	strcpy(PATH, "/");	// PROJECT

	while (1) {
		char *buf;

		if(interactive)	// PROJECT
			cprintf("jos-shell %s ", PATH);

		buf = readline(interactive ? "$ " : NULL);
		if (buf == NULL) {
			if (debug)
				cprintf("EXITING\n");
			exit();	// end of file
		}
		if (debug)
			cprintf("LINE: %s\n", buf);
		if (buf[0] == '#')
			continue;
		if (echocmds)
			printf("# %s\n", buf);
		if (debug)
			cprintf("BEFORE FORK\n");
		if ((r = fork()) < 0)
			panic("fork: %e", r);
		if (debug)
			cprintf("FORK: %d\n", r);
		if (r == 0) {
			runcmd(buf);
			exit();
		} else
			wait(r);
	}
}

