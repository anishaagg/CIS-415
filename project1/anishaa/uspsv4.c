/*
Author: Anisha Aggarwal
DuckID: anishaa
Title: CIS 415 Project 1
File: uspsv4.c
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
#include <sys/time.h>
#include <signal.h>
#include "p1fxns.h"

// Struct to hold the stats of child processes
struct stats {
	pid_t	pid;
	char	command[1024];
	int		vmsize;
	unsigned int	uticks;
	unsigned int	sticks;
	int		rchar;
	int		wchar;
};

struct stats *childstats;
int		statcount;
pid_t	pid[100];
int 	child_count = 0;
int 	msec = -1;

void PutStrAndNum (char *s, int i)
{
	p1putstr(STDOUT_FILENO, s);
	p1putint(STDOUT_FILENO, i);
	p1putstr(STDOUT_FILENO, "\n");
}

// Given PID and the file name in PROC, construct the filename and open the file
void constructFilename(char *f, pid_t p, char *w, int *fd) {
	char	str[80];

	f[0] = '\0';
	p1strcat(f, "/proc/");

	// Append PID
	p1itoa(p, str);
	p1strcat(f, str);
	p1strcat(f, "/");

	// append the specific filename, as specified in "w"
	p1strcat(f, w);

	//p1putstr(STDOUT_FILENO, "filename ");
	//p1putstr(STDOUT_FILENO, f);

	*fd = open(f, O_RDONLY);
	if (*fd == -1) {
		p1putstr(STDOUT_FILENO, "File was unable to open \n");
	}
}

void GetStatus(pid_t pid) {
	char	filename[80];
	int		fd;
	char	str[1024];
	char	word[100];
	int	i = 0;

	// Print command line and memory size from /proc/[pid]/status
	constructFilename(filename, pid, "status", &fd);

	if (p1getline(fd, str, 1024) == 0) {
		p1putstr(STDOUT_FILENO, "PrintStatus: Empty file\n");
		return;
	}

	i = p1getword(str, i, word); // discard Name:
	if (strcmp(word, "Name:") != 0) {
		p1putstr(STDOUT_FILENO, "Bad Name: ");
		p1putstr(STDOUT_FILENO, word);
		p1putstr(STDOUT_FILENO, "\n");
	}

	i = p1getword(str, i, word); // this is the command
	p1strcpy(childstats[statcount].command, word);

	//p1putstr(STDOUT_FILENO, "Command = ");
	//p1putstr(STDOUT_FILENO, word);
	//p1putstr(STDOUT_FILENO, "\n");
	
	int len = p1strlen(childstats[statcount].command);
	while (len >= 0) {
		if (childstats[statcount].command[len] == '\n') {
			childstats[statcount].command[len] = '\0';
		}
		len--;
	}
	
	// discard next 11 lines to read VmSize
	for (i=0; i<11; i++) {
		if (p1getline(fd, str, 1024) == 0) {
			PutStrAndNum("PrintStatus: Line is bad: ", i);
			return;
		}
	}
	
	p1getline(fd, str, 1024);
	i = 0;
	i = p1getword(str, i, word); // discard VmSize:
	if (strcmp(word, "VmSize:") != 0) {
		p1putstr(STDOUT_FILENO, "Bad VmSize: ");
		p1putstr(STDOUT_FILENO, word);
		p1putstr(STDOUT_FILENO, "\n");
	}

	i = p1getword(str, i, word); // this is the size of virutal memory
	childstats[statcount].vmsize = p1atoi(word);
	//PutStrAndNum("VmSize = ", childstats[statcount].vmsize);
	//p1putstr(STDOUT_FILENO, "\n");

	close(fd);
}

void GetStat(pid_t pid) {
	char	filename[80];
	int		fd;
	char	str[1024];
	char	word[100];

	// Print execution time from /proc/[pid]/stat
	constructFilename(filename, pid, "stat", &fd);

	if (p1getline(fd, str, 1024) == 0) {
		p1putstr(STDOUT_FILENO, "PrintExecution: Empty file\n");
		return;
	}

	int w;
	int i = 0;
	for (w=0; w<13; w++) { // discard first 13 values
		i = p1getword(str, i, word); // discard 
		if (i == -1) {
			PutStrAndNum("Bad return from p1getword at w = ", w);
		}
	}

	// Read UTime
	i = p1getword(str, i, word); 
	if (i == -1) {
		p1putstr(STDOUT_FILENO, "Bad return from p1getword for UTime\n");
		return;
	} else {
		childstats[statcount].uticks = p1atoi(word);
		//PutStrAndNum("UTime = ", childstats[statcount].uticks);
	}

	// Read STime
	i = p1getword(str, i, word); 
	if (i == -1) {
		p1putstr(STDOUT_FILENO, "Bad return from p1getword for STime\n");
		return;
	} else {
		childstats[statcount].sticks = p1atoi(word);
		//PutStrAndNum("STime = ", childstats[statcount].sticks);
	}

	close(fd);
}

void GetIo(pid_t pid) {
	char	filename[80];
	int		fd;
	char	str[1024];
	char	word[100];
	int	i = 0;

	// Print io stats from /proc/[pid]/io
	constructFilename(filename, pid, "io", &fd);

	//read rchar
	if (p1getline(fd, str, 1024) == 0) {
		p1putstr(STDOUT_FILENO, "PrintExecution: Empty file\n");
		return;
	}

	i = 0;
	i = p1getword(str, i, word); // discard rchar:
	if (strcmp(word, "rchar:") != 0) {
		p1putstr(STDOUT_FILENO, "Bad rchar: ");
		p1putstr(STDOUT_FILENO, word);
		p1putstr(STDOUT_FILENO, "\n");
	}

	i = p1getword(str, i, word);
	childstats[statcount].rchar = p1atoi(word);
	//PutStrAndNum("rchar = ", childstats[statcount].rchar);

	//read wchar
	if (p1getline(fd, str, 1024) == 0) {
		p1putstr(STDOUT_FILENO, "PrintExecution: Empty file\n");
		return;
	}

	i = 0;
	i = p1getword(str, i, word); // discard wchar:
	if (strcmp(word, "wchar:") != 0) {
		p1putstr(STDOUT_FILENO, "Bad wchar: ");
		p1putstr(STDOUT_FILENO, word);
		p1putstr(STDOUT_FILENO, "\n");
	}

	i = p1getword(str, i, word);
	childstats[statcount].wchar = p1atoi(word);
	//PutStrAndNum("rchar = ", childstats[statcount].wchar);

	close(fd);
}

void PrintStats() {
	childstats = malloc(child_count * sizeof(struct stats));
	if (childstats == NULL) {
		p1putstr(STDOUT_FILENO, "Error malloc of struct stats\n");
		return;
	}

	statcount = 0;
	int i;
	for (i=0; i<child_count; i++) {

		childstats[statcount].pid = pid[i];

		if (pid[i] != 0) {
			// get command line and virtual memory size from /proc/[pid]/status
			GetStatus(pid[i]);

			// get execution time from /proc/[pid]/stat
			GetStat(pid[i]);

			// get io from /proc/[pid]/io
			GetIo(pid[i]);

			statcount++;
		}

	}

	p1putstr(STDOUT_FILENO, "========\n");
	p1putstr(STDOUT_FILENO, "PID\tCommand\t\tVMSize\tUticks\tSticks\trchar\twchar\n");
	p1putstr(STDOUT_FILENO, "========\n");

	int	index = 0;
	for (i=0; i<statcount; i++) {

		p1putint(STDOUT_FILENO, childstats[index].pid);
		p1putstr(STDOUT_FILENO, "\t");
		p1putstr(STDOUT_FILENO, childstats[index].command);
		p1putstr(STDOUT_FILENO, "\t");
		p1putint(STDOUT_FILENO, childstats[index].vmsize);
		p1putstr(STDOUT_FILENO, "\t");
		p1putint(STDOUT_FILENO, childstats[index].uticks);
		p1putstr(STDOUT_FILENO, "\t");
		p1putint(STDOUT_FILENO, childstats[index].sticks);
		p1putstr(STDOUT_FILENO, "\t");
		p1putint(STDOUT_FILENO, childstats[index].rchar);
		p1putstr(STDOUT_FILENO, "\t");
		p1putint(STDOUT_FILENO, childstats[index].wchar);
		p1putstr(STDOUT_FILENO, "\t");

		p1putstr(STDOUT_FILENO, "\n");
		index++;
	}

	free(childstats);
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
		p1perror(STDOUT_FILENO, "fork");
		return;
	} else if (id == 0) { // child
		
		int	sig;
		sigset_t waitSig;
	
		// Wait for SIGUSR1 from the parent process				
		sigemptyset(&waitSig);
		sigaddset(&waitSig, SIGCONT);
		if (sigwait(&waitSig, &sig) != 0) {
			p1perror(STDOUT_FILENO, "Sigwait : ");
			exit(1);
		}

		if (sig != SIGCONT) {
			PutStrAndNum("Unexpected signal received ", sig);
			exit(1);
		} else {
			//PutStrAndNum("Child received SIGCONT ", getpid());
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

void AlarmHandler(int sig) {
	static	int childToRun = -1;
	int	childToStop;
	static	int timeToPrint = 0;

	if (sig != SIGALRM) {
		PutStrAndNum("AlarmHandler: unexpected signal ", sig);
		return;
	}

	if (childToRun == -1) {
		// we have not started any child process yet
		childToStop = -1;
		childToRun = 0;
	} else {
		childToStop = pid[childToRun]? childToRun: -1;
		int loopCount = 0;
		do {
			childToRun = childToRun+1;
			childToRun = ((childToRun > (child_count -1)) ? 0: childToRun);
			
			if (loopCount == 2 * child_count) {
				break;
			}

			loopCount++;

		} while (pid[childToRun] == 0);
	}

	//PutStrAndNum("STOP Child ", childToStop);
	//PutStrAndNum("CONT Child ", childToRun);
	
	// Stop the currently running child process
	if ((childToStop != -1) && (pid[childToStop] != 0)){
		if (kill(pid[childToStop], SIGSTOP) == -1) {
			p1perror(STDOUT_FILENO, "Kill SIGSTOP");
			exit(1);
		} else {
			//PutStrAndNum("S", childToStop);
			// Wait for the child process to be stopped
			WaitChildToStop(pid[childToStop]);
		}
	}

	if ((timeToPrint++ % 5) == 0) {
		PrintStats();
	}

	//set up timer for new child process to run
	struct itimerval newtimer;

	signal(SIGALRM, AlarmHandler);
	newtimer.it_value.tv_sec=0;
	newtimer.it_value.tv_usec=msec * 1000;
	newtimer.it_interval.tv_sec=0;
	newtimer.it_interval.tv_usec=0;
	if (setitimer(ITIMER_REAL, &newtimer, NULL) == -1) {
		p1putstr(STDOUT_FILENO, "Parent: Problem setting setitimer()\n");
		exit(1);
	}

	// Start the selected child process
	if (kill(pid[childToRun], SIGCONT) == -1) {
		p1perror(STDOUT_FILENO, "Kill SIGCONT");
		exit(1);
	} else {
		//PutStrAndNum("C", childToRun);
	}

}

int main(int argc, char **argv) {

/*
 * process environment variable and command line arguments
 */
	char 	*filename = NULL;
	int 	fd = STDIN_FILENO;
	char 	str[1024];
	char 	*childargv[100];

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
	sigaddset(&blocksig, SIGCONT);
	if (sigprocmask(SIG_BLOCK, &blocksig, &origmask) == -1) {
		p1putstr(STDOUT_FILENO, "Blocking SIGCONT failed\n");
		exit(1);
	} else {
		//p1putstr(STDOUT_FILENO, "Parent: SIGCONT Blocked\n");
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

	struct itimerval newtimer;

	signal(SIGALRM, AlarmHandler);
	newtimer.it_value.tv_sec=0;
	newtimer.it_value.tv_usec=msec * 1000;
	newtimer.it_interval.tv_sec=0;
	newtimer.it_interval.tv_usec=0;
	if (setitimer(ITIMER_REAL, &newtimer, NULL) == -1) {
		p1putstr(STDOUT_FILENO, "Parent: Problem setting setitimer()\n");
		exit(1);
	}

	int numTerminatedChild = 0;	
	for (;;) { // Wait for all child processes to exit
		int	status;
		
		pid_t childPID = wait(&status);
		if (childPID != -1) {
			int p;
			for (p=0; p<child_count; p++) {
				if (pid[p] == childPID) {
					pid[p] = 0;
					break;
				}
			}
			numTerminatedChild++;
			if (numTerminatedChild == child_count) {
				//p1putstr(STDOUT_FILENO, "All child processes terminated\n");
				return(0);
			}
		}
	}

	//PutStrAndNum("Terminated child count = ", numTerminatedChild);

	return 0;
}
