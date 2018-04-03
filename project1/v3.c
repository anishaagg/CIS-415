//Anisha Aggarwal CIS415	Project 1	uspsv1.c

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Assume a max of 100 programs to run, and for each program ... 10 args, each with 30 chars at most

#define MAX_PROG 			100
#define	MAX_ARGS			10
#define MAX_CHARS_IN_ARG	30


void PutStrAndNum (char *s, int i)
{
		p1putstr(stdout, s);
		p1putint(stdout, child_count);
		p1putstr(stdout, "\n");
}

void AlarmHandler(int sig) {
	static	int childToRun = -1;
	int	childToStop;

	if (sig != SIGALRM) {
		PutStrAndNum("AlarmHandler: unexpected signal ", sig)
		return;
	}

	if (childToRun == -1) {
		// we have not started any child process yet
		childToStop = -1;
		childToRun = 0;
	} else {
		childToStop = pid[childToRun]? childToRun: 0;
		do {
			childToRun = childToRun+1;
			childTorun = ((childToRun > (child_count -1)) ? 0: childToRun);
		while (pid[childToRun] == 0)
	}

	PutStrAndNum("STOP Child ", childToStop);
	p1putstr(STDIO_FILENO, "\n");
	PutStrAndNum("CONT Child ", childToRun);
	p1putstr(STDIO_FILENO, "\n");

	// Stop the currently running child process
	if (childToStop != -1) {
		if (kill(pid[childToStop], SIGSTOP) == -1) {
			p1perror("Kill SIGSTOP");
			exit(1);
		} else {
			PutStrAndNum("S", childToStop);
			// Wait for the child process to be stopped
			WaitChildToStop(pid[childToStop]);
		}
	}

	// Start the selected child process
	if (kill(pid[childToRun], SIGCONT) == -1) {
		p1perror("Kill SIGCONT");
		exit(1)
	} else {
		PutStrAndNum("C", ChildToRun);
	}

}


int main(int argc, char **argv) {

/*
 * process environment variable and command line arguments
 */
	int 	msec = -1;
	char 	*filename = NULL;
	int 	fd = stdin;
	char 	args[MAX_PROG][MAX_ARGS][MAX_CHARS_IN_ARG];

	int 	num_prog = 0;
	char 	str[100];
	int 	child_count = 0;

	//read env variable
	char* env = getenv("USPS_QUANTUM_MSEC");
	if (env != NULL) {
		msec = p1atoi(env);
	}

	int index;
    for (int i = 1; i < argc; i++) {
    	//read commmand line to see if msec needs to be overriden 
    	index = p1strchr(argv[i], '=');
    	if (index != -1) {
    		msec = p1atoi(&argv[i] + index + 1);
    	} else {
    		//see if there is a file name specified
    		filename = argv[i];
    	}
    }

    //open and read file
    if (filename != NULL) {
    	fd = open(filename, O_RDONLY);
    	if (fd == -1) {
    		//printf("%s\n", "File was unable to open");
    	}

    	do {
			int num_char = p1getline(fd, str, 100);
			if (num_char != 0) { // not EOF
    			
				// Separate the program name and arguments into prog array
				int		i=0;
				char	word[100];
				int		argcount=0;

				PutStrAndNum("Child ", child_count);

				while ((i = p1getword(str, i, word) != -1)) {
					p1strcpy(args[child_count][argcount], word);
					p1putstr(args[child_count][argcount]);
					p1putstr(" ");
					argcount++;
				}

				args[child_count][argcount] = NULL;

				p1putstr("\n");
			}

    	} while (num_char != 0);
    	
		PutStrAndNum("Child count = ", child_count);

		//
		// Block SIGUSR1 so that they are not ignored when the child starts
		// 
		sigset_t blocksig, origmask;

		sigemptyset(&blocksig);
		sigaddset(&blocksig, SIGUSR1);
		if (sigprocmask(SIG_BLOCK, &blocksig, &origmask == -1)) {
			p1putstr("Blocking SIGUSR1 failed\n");
			exit(1);
		}

    	for (int j = 0; j < child_count; ++j) {


			
    		pid[j] = fork();
    		if (pid[j] == 0) {
    			//child process
    		
				int			sig;
				sigset_t	waitSig;
				siginfo_t	sigInfo;
			
				// Wait for SIGUSR1 from the parent process				
				sigemptyset(&waitSig);
				sigaddset(&waitSig, SIGUSR1);
				sig = sigwaitinfo(&waitSig, sigInfo);
				if (sig != SIGUSR1) {
					PutStrAndNum("Unexpected signal received ", sig);
					exit(1);
				} else {
					PutStrAndNum("Child received SIGUSR1 ", getpid());
				}

				// Setup Signal handlers for SIGSTOP and SIGCONT
				// signal(SIGSTOP, ChildSigHandler);
				// signal(SIGCONT, ChildSigHandler);

				execvp(args[j][0], args[j]);
    		}
    	}

		// Send SIGUSR1 to all child processes
		if (kill(0, SIGUSR1) == -1) {
			p1perror("Kill SIGUSR1");
			exit(1);
		}

/// VERSION 2

		// Sleep for 1 sec, and send SIGSTOP to all child processes
		if (kill(0, SIGSTOP) == -1) {
			p1perror("Kill SIGSTOP");
			exit(1);
		}

		// Sleep for 1 sec, and send SIGCONT to all child processes
		if (kill(0, SIGCONT) == -1) {
			p1perror("Kill SIGCONT");
			exit(1);
		}
// END OF VERSION 2
		// SIGSTOP to all child processes
		if (kill(0, SIGSTOP) == -1) {
			p1perror("Kill SIGSTOP");
			exit(1);
		}

		itimerval	newtimer;

		signal(SIGALRM, AlarmHandler);
		newtimer.it_value.tv_sec=0;
		newtimer.it_value.tv_usec=msec * 1000;
		newtimer.it_interval.tv_sec=0;
		newtimer.it_interval.tv_usec=msec * 1000;
		if (setitimer(ITIMER_REAL, &newtimer, NULL) == -1) {
			p1putstr("Parent: Problem setting setitimer()\n");
			exit(1);
		}


// END OF VERSION 3
		for (;;) { // Wait for all child processes to exit
			int	status;
			
			childPID = wait(&status);
			if (childPID != -1) {
				for (int p=0; p<child_count; p++) {
					if (pid[p] == childPID) {
						pid[p] = 0;
						break;
					}
				}
				numTerminatedChild++
				if (numTerminatedChild == child_count) {
					p1putstr(stdput, "All child processes terminated\n");
					return(0);
				}
			}
		}

		PutStrAndNum("Terminated child count = ", numTerminatedChild);
    }

	return 0;
}
