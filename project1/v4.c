
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
int	statcount;


// Given PID and the file name in PROC, construct the filename and open the file
void constructFilename(char *f, pid_t p, char *w, int &fd) {
	char	*str[80];

	f[0] = '\0';
	p1strcat(f, "/proc/");

	// Append PID
	p1itoa(p, str);
	p1strcat(f, str);
	p1strcat(f, "/");

	// append the specific filename, as specified in "w"
	p1strcat(f, w);

	p1putstr(STDIO_FILENO, "filename ");
	pi1putstr(STDIO_FILENO, filename);

	*fd = open(filename, O_RDONLY);
	if (*fd == -1) {
		p1putstr(STDOUT_FILENO, "File was unable to open \n");
	}
}

void GetStatus(pid_t pid) {
	char	*filename[80];

	int		fd;
	char	str[1024];
	char	word[100];
	int		numchars;

	// Print command line and memory size from /proc/[pid]/status
	constructFilename(filename, pid, "status", &fd);

	if (p1getline(fd, str 1024) == 0) {
		p1putstr(STDIO_FILENO, "PrintStatus: Empty file\n");
		return;
	}

	i = p1getword(str, i, word); // discard Name:
	if (strcmp(word, "Name:") != 0) {
		p1putstr(STDIO_FILENO, "Bad Name: ");
		p1putstr(STDIO_FILENO, word);
		p1putstr(STDIO_FILENO, "\n");
	}
	i = p1getword(str, i, word); // this is the command
	pstrcpy(childstats[statcount].command, word);

	p1putstr(STDIO_FILENO, "Command = ");
	p1putstr(STDIO_FILENO, word);
	p1putstr(STDIO_FILENO, "\n");

	// discard next 15 lines to read VmSize
	for (int i=0; i<15; i++) {
		if (p1getline(fd, str 1024) == 0) {
			PutNumAndStr("PrintStatus: Line is bad: ", i);
			return;
		}
	}
	i = p1getword(str, i, word); // discard VmSize:
	if (strcmp(word, "VmSize:") != 0) {
		p1putstr(STDIO_FILENO, "Bad VmSize: ");
		p1putstr(STDIO_FILENO, word);
		p1putstr(STDIO_FILENO, "\n");
	}
	i = p1getword(str, i, word); // this is the size of virutal memory
	childstats[statcount].vmsize = p1atoi(word);
	PutStrAndNum("VmSize = ", vmsize);
	p1putstr(STDIO_FILENO, "\n");

	close(fd);
}

void GetStat(pid_t pid) {
	char	*filename[80];
	int		fd;

	// Print execution time from /proc/[pid]/stat
	constructFilename(filename, pid, "stat", &fd);

	if (p1getline(fd, str 1024) == 0) {
		p1putstr(STDIO_FILENO, "PrintExecution: Empty file\n");
		return;
	}

	for (int w=0; w<13; w++) { // discard first 13 values
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
		childstats[statcount].utime = p1atoi(word);
		PutStrAndNum("UTime = ", childstats[statcount].utime);
	}

	// Read STime
	i = p1getword(str, i, word); 
	if (i == -1) {
		p1putstr(STDOUT_FILENO, "Bad return from p1getword for STime\n");
		return;
	} else {
		childstats[statcount].stime = p1atoi(word);
		PutStrAndNum("STime = ", childstats[statcount].stime);
	}

	close(fd);
}

void GetIo(pid_t pid) {
	char	*filename[80];
	int		fd;

	// Print execution time from /proc/[pid]/io
	constructFilename(filename, pid, "io", &fd);

	//read rchar
	if (p1getline(fd, str 1024) == 0) {
		p1putstr(STDIO_FILENO, "PrintExecution: Empty file\n");
		return;
	}

	i = p1getword(str, i, word); // discard rchar:
	if (strcmp(word, "rchar:") != 0) {
		p1putstr(STDIO_FILENO, "Bad rchar: ");
		p1putstr(STDIO_FILENO, word);
		p1putstr(STDIO_FILENO, "\n");
	}
	i = p1getword(str, i, word);
	childstats[statcount].rchar = p1atoi(word);
	PutStrAndNum("rchar = ", childstats[statcount].rchar);

	//read wchar
	if (p1getline(fd, str 1024) == 0) {
		p1putstr(STDIO_FILENO, "PrintExecution: Empty file\n");
		return;
	}

	i = p1getword(str, i, word); // discard wchar:
	if (strcmp(word, "wchar:") != 0) {
		p1putstr(STDIO_FILENO, "Bad wchar: ");
		p1putstr(STDIO_FILENO, word);
		p1putstr(STDIO_FILENO, "\n");
	}
	i = p1getword(str, i, word);
	childstats[statcount].wchar = p1atoi(word);
	PutStrAndNum("rchar = ", childstats[statcount].wchar);


	close(fd);
}

void PrintStats() {
	childstats = malloc(child_count * sizeof(struct stats));
	if (childstats === NULL) {
		p1putstr(STDIO_FILENO, "Error malloc of struct stats\n");
		return;
	}

	statcount = 0;

	for (int i=0; i<child_count; i++) {

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

	p1putstr(STDIO_FILENO, "========\n");
	p1putstr(STDIO_FILENO, "PID			Command			VMSize		UTime(ticks)		STime(ticks)	rchar	wchar\n");
	p1putstr(STDIO_FILENO, "========\n");

	int	index = 0;
	for (int i=0; i<statcount; i++) {

		p1putint(STDOUT_FILENO, childstats[index].pid);
		p1putstr(STDOUT_FILENO, "		");
		p1putstr(STDOUT_FILENO, childstats[index].command);
		p1putstr(STDOUT_FILENO, "		");
		p1putint(STDOUT_FILENO, childstats[index].vmsize);
		p1putstr(STDOUT_FILENO, "		");
		p1putint(STDOUT_FILENO, childstats[index].utime);
		p1putstr(STDOUT_FILENO, "		");
		p1putint(STDOUT_FILENO, childstats[index].stime);
		p1putstr(STDOUT_FILENO, "		");
		p1putint(STDOUT_FILENO, childstats[index].rchar);
		p1putstr(STDOUT_FILENO, "		");
		p1putint(STDOUT_FILENO, childstats[index].wchar);
		p1putstr(STDOUT_FILENO, "		");


		index++;
	}

	free(childstats);
}

