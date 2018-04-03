/*
Author: Anisha Aggarwal
DuckID: anishaa
Title: CIS 415 Project 1
File: uspsv2.c
Statement: All of the work included in this file is my own.
	I consulted http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/exec.html
	for help with argument parsing.
*/

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

pid_t	pid[100];

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
		//PutStrAndNum("Waitpid: PID stopped : ", pid);
	} else {
		PutStrAndNum("Waitpid: bad status : ", status);
		exit(1);
	}
}

// Create child process
void executeChild(char **argv, int child_num) {
	pid_t 	id;

	id = fork();
	if (id < 0) { //error
		//p1perror(STDOUT_FILENO, "fork");
		return;
	} else if (id == 0) { // child
		
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
			//PutStrAndNum("Child received SIGUSR1 ", getpid());
		}

		if (execvp(*argv, argv) == -1) {
			p1perror(STDOUT_FILENO, "EXECVP Failed \n");
			exit(1);
		}
	} else { // parent
		pid[child_num] = id;
	}
}

// Break the read line into argv vector
void createArgv(char *s, char **argv) {
	while(*s != '\0'){
		// skip whitespace
		while((*s == ' ') || (*s == '\t') || (*s == '\n')){
			*s++ = '\0';
		}

		// set argv to point to this location
		*argv++ = s;

		while((*s != '\0') && (*s != ' ') && (*s != '\t') && (*s != '\n')){
			s++;
		}
	}
	*argv = NULL;
}

int main(int argc, char **argv) {

/*
 * process environment variable and command line arguments
 */
	int 	msec = -1;
	char 	*filename = NULL;
	int 	fd = STDIN_FILENO;
	char 	str[1024];
	char 	*childargv[100];
	int 	child_count = 0;

	//read env variable
	char* env = getenv("USPS_QUANTUM_MSEC");
	if (env != NULL) {
		msec = p1atoi(env);
	}

	// Scan the args
	int i;
	for (i = 1; i < argc; i++) {
		//read commmand line to see if msec needs to be overriden 
		int index = p1strchr(argv[i], '=');
		if (index != -1) {
			// make sure this matches "--quantum="
			char	*quantumstr = "--quantum=";
			int	arglen = p1strlen(argv[i]);
			int	quantumlen = p1strlen(quantumstr);

			if (arglen < quantumlen) {
				p1putstr(STDOUT_FILENO, "usage: ./uspsv? [–-quantum=<msec>] [workload_file]\n");
				exit(1);
			}
			int q;
			for (q = 0; q < quantumlen; q++) {
				if (argv[i][q] != quantumstr[q]) {
					p1putstr(STDOUT_FILENO, "usage: ./uspsv? [–-quantum=<msec>] [workload_file]\n");
					exit(1);
				}
			}

			msec = p1atoi(argv[i] + index + 1);
		} else {
			//see if there is a file name specified
			filename = argv[i];
		}
	}

	//check if quantum is in the enviornment or command line
	if (msec == -1) {
		p1putstr(STDOUT_FILENO, "Quantum not specified \n");
		exit(1);
	}

	//open and read file
	if (filename != NULL) {
		fd = open(filename, O_RDONLY);
		if (fd == -1) {
			p1putstr(STDOUT_FILENO, "File was unable to open\n");
			exit(1);
		}
	}


	// Block SIGUSR1
	sigset_t blocksig, origmask;

	sigemptyset(&blocksig);
	sigaddset(&blocksig, SIGUSR1);
	if (sigprocmask(SIG_BLOCK, &blocksig, &origmask) == -1) {
		p1putstr(STDOUT_FILENO, "Blocking SIGUSR1 failed\n");
		exit(1);
	} else {
		//p1putstr(STDOUT_FILENO, "Parent: SIGUSR1 Blocked\n");
	}


	int num_char;
	do {
		num_char = p1getline(fd, str, 1024);
		if (num_char != 0) { // not EOF
			
			if (str[num_char -1] == '\n') {
				str[num_char-1] = '\0';
			} else {
				str[num_char] = '\0';
			}

			createArgv(str, childargv);
			executeChild(childargv, child_count);
			child_count++;
		}

	} while (num_char != 0);
	
	//PutStrAndNum("Child count = ", child_count);

	// Send SIGUSR1 to all child processes
	for (i = 0; i<child_count; i++) {
		if (kill(pid[i], SIGUSR1) == -1) {
			p1putstr(STDOUT_FILENO, "Kill SIGUSR1");
			exit(1);
		} else {
			//p1putstr(STDOUT_FILENO, "SIGUSR sent \n");
		}
	}

	// send SIGSTOP to all child processes
	for (i = 0; i<child_count; i++) {
		if (kill(pid[i], SIGSTOP) == -1) {
			p1putstr(STDOUT_FILENO, "Kill SIGSTOP");
			exit(1);
		} else {
			//p1putstr(STDOUT_FILENO, "SIGSTOP sent \n");
		}
	}

	// Wait for all child processes to be STOPPED
	for (i = 0; i<child_count; i++) {
		WaitChildToStop(pid[i]);
	}

	// send SIGCONT to all child processes
	for (i = 0; i<child_count; i++) {
		if (kill(pid[i], SIGCONT) == -1) {
			p1putstr(STDOUT_FILENO, "Kill SIGCONT");
			exit(1);
		} else {
			//p1putstr(STDOUT_FILENO, "SIGCONT sent \n");
		}
	}

	int numTerminatedChild = 0;	
	for (;;) { // Wait for all child processes to exit
		pid_t childPID = wait(NULL);
		if (childPID != -1) {
			numTerminatedChild++;
			//PutStrAndNum("Terminated child count = ", numTerminatedChild);
		} else {
			//p1putstr(STDOUT_FILENO, "All child processes terminated\n");
			exit(0);
		}
	}

	return 0;
}
