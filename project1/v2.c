//Anisha Aggarwal CIS415	Project 1	uspsv2.c

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "p1fxns.h"

// Assume a max of 100 programs to run, and for each program ... 10 args, each with 30 chars at most

#define MAX_PROG 			100
#define	MAX_ARGS			10
#define MAX_CHARS_IN_ARG	30


void PutStrAndNum (char *s, int i)
{
		p1putstr(STDOUT_FILENO, s);
		p1putint(STDOUT_FILENO, i);
		p1putstr(STDOUT_FILENO, "\n");
}

void WaitChildToStop(pid_t pid) {
	pid_t	p;
	int		status;

	p = waitpid(pid, &status, WUNTRACED);
	if (p != pid) {
		PutStrAndNum("Waitpid returned ", p);
		exit(1);
	}

	if (WIFSTOPPED(status)) {
		PutStrAndNum("Waitpid: PID stopped : ", pid);
	} else {
		PutStrAndNum("Waitpid: bad status : ", status);
		exit(1);
	}
}

int main(int argc, char **argv) {

/*
 * process environment variable and command line arguments
 */
	int 	msec = -1;
	char 	*filename = NULL;
	int 	fd = STDIN_FILENO;
	char 	args[MAX_PROG][MAX_ARGS][MAX_CHARS_IN_ARG];
	pid_t	pid[MAX_PROG];
	char 	str[100];
	int 	child_count = 0;

	//read env variable
	char* env = getenv("USPS_QUANTUM_MSEC");
	if (env != NULL) {
		msec = p1atoi(env);
	} else {
		PutStrAndNum("env ", msec);	//test
	}

	int index;
    for (int i = 1; i < argc; i++) {
    	//read commmand line to see if msec needs to be overriden 
    	index = p1strchr(argv[i], '=');
    	if (index != -1) {
    		msec = p1atoi(argv[i] + index + 1);
    		PutStrAndNum("command line ", msec);	//test
    	} else {
    		//see if there is a file name specified
    		filename = argv[i];
    	}
    }

    //check if in the enviornment or command line
    if (msec == -1) {
    	p1putstr(STDOUT_FILENO, "Quantum not specified \n");
    	exit(1);
    } else {
		p1putint(STDOUT_FILENO, msec);
	
	}

    //open and read file
    if (filename != NULL) {
    	fd = open(filename, O_RDONLY);
    	if (fd == -1) {
    		//printf("%s\n", "File was unable to open");
			p1putstr(STDOUT_FILENO, "File was unable to open \n");
    	}
	}
	int num_char;
	do {
		num_char = p1getline(fd, str, 100);
		if (num_char != 0) { // not EOF
			
			// Separate the program name and arguments into prog array
			int		i=0;
			char	word[100];
			int		argcount=0;

			PutStrAndNum("Child ", child_count);

			while ((i = p1getword(str, i, word)) != -1) {
				p1strcpy(args[child_count][argcount], word);

				p1putstr(STDOUT_FILENO, args[child_count][argcount]);
				p1putstr(STDOUT_FILENO, " ");

				argcount++;
			}

			p1putstr(STDOUT_FILENO, "\n");
			
			child_count++;
			args[child_count][argcount][0] = NULL;
			//args[child_count][argcount][0] = '\0';

		}

	} while (num_char != 0);
	
	PutStrAndNum("my Child count = ", child_count);

	// Block SIGUSR1
	sigset_t blocksig, origmask;

	sigemptyset(&blocksig);
	sigaddset(&blocksig, SIGUSR1);
	if (sigprocmask(SIG_BLOCK, &blocksig, &origmask == -1)) {
		p1putstr(STDOUT_FILENO, "Blocking SIGUSR1 failed\n");
		exit(1);
	} else {
		p1putstr(STDOUT_FILENO, "Parent: SIGUSR1 Blocked\n");
	}

	// Create child processes for each command and run the command
	for (int j = 0; j < child_count; ++j) {
		p1putstr(STDOUT_FILENO, "doing fork \n");
		pid[j] = fork();
		if (pid[j] == 0) {
			
			//child process
			p1putstr(STDOUT_FILENO, "in Child ");
			
			// print the args in child process
			for (int k=0; ; k++) {
				if (args[j][k][0] != NULL) {
					p1putstr(STDOUT_FILENO, args[j][k]);
				} else {
					break;
				}
			}

			int	sig;
			sigset_t waitSig;
		
			// Wait for SIGUSR1 from the parent process				
			sigemptyset(&waitSig);
			sigaddset(&waitSig, SIGUSR1);
			if (sigwait(&waitSig, &sig) != 0) {
				p1perror(STDOUT_FILENO, "Sigwait : ");
				exit(1);
			}

			if (sig != SIGUSR1) {
				PutStrAndNum("Unexpected signal received ", sig);
				exit(1);
			} else {
				PutStrAndNum("Child received SIGUSR1 ", getpid());
			}

			p1putstr(STDOUT_FILENO, args[j][0]);
			// EXEC the command in this child
			if (execvp(args[j][0], args[j]) == -1) {
			//if (execvp(args[j][0], (char*const*)args[j]) == -1) {
				p1perror(STDOUT_FILENO, "EXECVP Failed \n");
				exit(1);
			}
			exit(0);
		}
		else {
			p1putint(STDOUT_FILENO, pid[j]);
		}
	}

	sleep(2);

	// Send SIGUSR1 to all child processes
	for (int i = 0; i<child_count; i++) {
		if (kill(pid[i], SIGUSR1) == -1) {
			p1putstr(STDOUT_FILENO, "Kill SIGUSR1");
			exit(1);
		} else {
			p1putstr(STDOUT_FILENO, "SIGUSR sent \n");
		}
	}

	// send SIGSTOP to all child processes
	for (int i = 0; i<child_count; i++) {
		if (kill(pid[i], SIGSTOP) == -1) {
			p1putstr(STDOUT_FILENO, "Kill SIGSTOP");
			exit(1);
		} else {
			p1putstr(STDOUT_FILENO, "SIGSTOP sent \n");
		}
	}

	// Wait for all child processes to be STOPPED
	for (int i = 0; i<child_count; i++) {
		WaitChildToStop(pid[i]);
	}

	// send SIGCONT to all child processes
	for (int i = 0; i<child_count; i++) {
		if (kill(pid[i], SIGCONT) == -1) {
			p1putstr(STDOUT_FILENO, "Kill SIGCONT");
			exit(1);
		} else {
			p1putstr(STDOUT_FILENO, "SIGCONT sent \n");
		}
	}


	int numTerminatedChild = 0;	
	for (;;) { // Wait for all child processes to exit
		pid_t childPID = wait(NULL);
		if (childPID != -1) {
			numTerminatedChild++;
			PutStrAndNum("Terminated child count = ", numTerminatedChild);
		} else {
			p1putstr(STDOUT_FILENO, "All child processes terminated\n");
			exit(0);
		}
	}

	return 0;
}
