/*
Author: Anisha Aggarwal
DuckID: anishaa
Title: CIS 415 Extra Credit Project
File: include_crawler.c
Statement: All of the work included in this file is my own.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "tslinkedlist.h"
#include "tshashmap.h"

#define MAX_DIRCOUNT	20
#define MAX_PATHLEN	 200
char	directory[MAX_DIRCOUNT][MAX_PATHLEN];
int	 	directory_count = 0;
TSLinkedList	*work_queue;
TSHashMap	   	*the_table;
int				numWorkerThreads=1;

// Function to print the linked list
void print_ll(char *s, TSLinkedList *w) {
	char	**a;
	long	len;

	a = (char **)tsll_toArray(w, &len);
	if (a!=NULL) {
		printf("%s: Len=%ld Elements: ", s, len);
		int	i;
		for (i=0; i<len; i++) {
			printf("%s ", a[i]);
		}
		printf("\n");
	} else {
		printf("%s: Len=0\n", s);
	}
}

// Function to print the hash map (the_table)
void print_the_table() {
	TSHashMap	   *t = the_table;
	char	**a;
	long	len;

	a = (char **)tshm_keyArray(t, &len);
	if (a!=NULL) {
		printf("the_table: len = %ld\n", len);

		TSLinkedList *ll;
		int			i;
		for (i=0; i<len; i++) {
			printf("\'%s\': ", a[i]);
			tshm_get(t, a[i], (void **)&ll);
			print_ll("DEP", ll);
		}
	} else {
		printf("print_the_table: no entries\n");
	}
}

FILE *open_file(char  *afile) {
	int i;
	FILE	*fp;
	char	fname[MAX_PATHLEN];

	for (i=0; i < directory_count; i++) {
		strcpy(fname, directory[i]);
		strcat(fname, afile);
		fp = fopen(fname, "r");
		if (fp != NULL) {
			//printf("Found file \'%s\' in directory \'%s\'\n", afile, directory[i]);
			return fp;
		}
	} 

	printf("Did NOT find file \'%s\'\n", afile);
	return NULL;

}

void skip_whitespace(char **p, int len) {
	int i=0;

	while (i<len) {
		if ((*p)[i] == ' ') {
			++i;
		} else {
			break;
		}
	}
	(*p) += i;
}

void parse_file(char *afile, char *fname, char *fext) {
	char	*p;

	p = strrchr(afile, '.');
	if (p == NULL) {
		strcpy(fname, afile);
		strcpy(fext, "");
	} else {
		strncpy(fname, afile, p-afile);
		fname[p-afile] = '\0';
		strcpy(fext, afile+(p-afile+1));
	}
}

void print_dependencies(TSHashMap *the_table, TSLinkedList *printed, TSLinkedList *to_process) {
	char			*fn;
	TSLinkedList	*namell;
	long			toprocess_size, printed_size;
	int			 	i, j;
	char			*f;
	char			*printed_f;

	while (tsll_removeFirst(to_process, (void *)&fn)) {
		if (tshm_get(the_table, fn, (void **)&namell) == 0) {
			printf("print_dependencies: tshm_get failed\n");
			return;
		}
		toprocess_size = tsll_size(namell);
		for (i=0; i<toprocess_size; i++) {
			if (tsll_get(namell, i, (void**)&f)) {
				// if f not in printed
				printed_size = tsll_size(printed);
				for (j=0; j<printed_size; j++) {
					if ((tsll_get(printed, j, (void **)&printed_f)) &&
						(strcmp(f, printed_f) == 0)) {
						break;
					}
				}
				if (j == printed_size) { // not found in printed
					printf(" %s", f);
					tsll_addLast(printed, f);
					tsll_addLast(to_process, f);
				}
			} else {
				printf("Error in tsll_get\n");
			}
		}
	}
}

void	free_ll_element(void *p) {
	free(p);
}

void	free_ll(void *p) {
	if (p != NULL) {
		tsll_destroy((TSLinkedList *)p, free_ll_element);
	}
}

void process(char *afile, TSLinkedList  *dep, TSHashMap *the_table, TSLinkedList *work_queue) {
	FILE	*fp;
	char	*line;
	size_t  len;
	char	*fn;
	TSLinkedList	*previous;

	//printf("process_file: \'%s\' -----\n", afile);

	fp = open_file(afile);
	if (fp == NULL) {
		printf("process: unexpected file %s not found\n", afile);
		return;
	}

	line = NULL;
	len = 0;
	while (getline(&line, &len, fp) != -1) {
		char	*p = line;

		// Skip initial whitespace
		skip_whitespace(&p, strlen(p));
		
		if (strncmp(p, "#include", 8) == 0) {
			p += 8;;
			
			// look for '"'
			skip_whitespace(&p, strlen(p));
			
			if (p[0] == '"') { // non-system include file
				char	*q;
				p += 1;				

				q = strchr(p, '"');
				if (q != NULL) {
					fn = (char *)malloc(MAX_PATHLEN);
					if (fn == NULL) {
						printf("Error allocating fn\n");
						return;
					}
					strncpy(fn, p, q-p);
					fn[q-p] = '\0';
					//printf("Found INCLUDE: '\%s\'\n", fn);

					//print_ll("Orig Dep", dep);
					tsll_add(dep, fn);
					//print_ll("New Dep", dep);

					// If fn does not exist in the_table, add it to the_table and work_queue
					if (tshm_containsKey(the_table, fn) == 0) {
						//printf("Adding \'%s\' to the_table\n", fn);
						previous = NULL;
						if (tshm_put(the_table, fn, tsll_create(), (void **)&previous) == 0) { //FREE
							printf("Error in tshm_put %s\n", fn);
							return;
						}

						//print_the_table();

						if (previous != NULL) {
						//	tsll_destroy(previous, free_ll);
						}

						// add this file to work_queue
						tsll_addLast(work_queue, fn);
						//print_ll("WQ", work_queue);
					} else {
						//printf("'\%s\' already in the_table\n", fn);
					}
				}
			}

		}
	}

	if (line != NULL) {
		free(line);
	}

	return;
}

// Worker thread to process Work Q, process and put the data in the_table
//
void *	WorkerThread(void *args) {
	TSLinkedList  	*dep;
	char 			*aafile;
	TSLinkedList	*previous;

	if (args != NULL) {
		printf("Args non-NULL\n");
		return NULL;
	}

	while (tsll_removeFirst(work_queue, (void **)&aafile)) {
		//printf("Worker: Removed \'%s\' from work_queue\n", aafile);
		// Get the current dependency list from the_table
		if (tshm_get(the_table, aafile, (void **)&dep) == 1) {
			process(aafile, dep, the_table, work_queue); // Update the dependency list
			// Put the new set of dependencies back in the_table
			previous = NULL;
			if (tshm_put(the_table, aafile, dep, (void **)&previous) == 0) {
				printf("Error in tshm_put %s\n", aafile);
				return NULL;
			}
			//print_the_table();
		
			if (previous != NULL) {
				//tsll_destroy(previous, free_ll);
			}
		
		} else {
			printf("tshm_get failed for %s\n", aafile);
			return NULL;
		}
	}

	return NULL;
}

int main(int argc, char **argv) {
	int fstart=0;	 // index into argv for first filename
	int i;
	char	*obj;

	// Setup the list of directories to be searched
	strcpy(directory[0], "./");
	//printf("Adding directory \'%s\'\n", directory[directory_count]);
	++directory_count;

	for (i=1; i<argc; i++) {
		if ((argv[i][0] == '-') && (argv[i][1] == 'I')) { // starts with -I
			strcpy(directory[directory_count], &argv[i][2]);
			int len = strlen(directory[directory_count]);
			if (directory[directory_count][len] != '/') {
				strcat(directory[directory_count], "/");
			}
			//printf("Adding directory \'%s\'\n", directory[directory_count]);
			++directory_count;
		} else {
			fstart = i;
			break;
		}
	}

	// Read env variable CPATH
	char* cpath = getenv("CPATH");
	if (cpath != NULL) {
		char	*p=cpath;
		char	*newp;

		//printf("CPATH defined in environment\n");
		while ((newp = strchr(p, ':')) != NULL) {
			strncpy(directory[directory_count], p, newp-p);
			directory[directory_count][newp-p] = '\0'; // NULL terminate the string
			//printf("Adding directory \'%s\'\n", directory[directory_count]);
			++directory_count;
			p = newp+1;
		}

		strcpy(directory[directory_count], p);
		//printf("Adding directory \'%s\'\n", directory[directory_count]);
		++directory_count;

	} else {
		//printf("CPATH not defined in environment\n");
	}

	// Read env variable CRAWLER_THREADS to determine the number of worker threads
	char *crawler_threads = getenv("CRAWLER_THREADS");
	if (crawler_threads != NULL) {
		//printf("CRAWLER_THREADS defined in environment\n");
		numWorkerThreads = atoi(crawler_threads);
		//printf("Number of worker threads = %d\n", numWorkerThreads);
	}
	
	// at this point, we have the list of directories to search for #include files
	work_queue = tsll_create();
	if (work_queue == NULL) {
		printf("Error creating work_queue\n");
		return(-1);
	}

	the_table = tshm_create(0, 0.0);
	if (the_table == NULL) {
		printf("Error creating the_table\n");
		return(-1);
	}

	char	fname[MAX_PATHLEN];
	char	fext[MAX_PATHLEN];
	TSLinkedList	*previous;
	TSLinkedList  	*dep;

	// Process each file from the command line
	for (i = fstart; i<argc; i++) {
		parse_file(argv[i], fname, fext);
		if ((strcmp(fext, "c") != 0) && (strcmp(fext, "l") != 0) && (strcmp(fext, "y") != 0)) {
			printf("Illegal argument: \'%s\' must end in .c, .l or .y\n", argv[i]);
			return(-1);
		}
		obj = (char *)malloc(MAX_PATHLEN);
		if (obj == NULL) {
			printf("Error allocating obj\n");
			return (-1);
		}
		strcpy(obj, fname);
		strcat(obj, ".o");
		//printf("fname %s fext %s obj %s\n", fname, fext, obj);

		// Add .o --> .c in the_table
		dep = tsll_create();
		if (dep != NULL) {
			tsll_addLast(dep, argv[i]);
		} else {
			printf("Error creating dep\n");
			return(-1);
		}

		previous = NULL;
		if (tshm_put(the_table, obj, dep, (void **)&previous) == 0) {
			printf("Error in tshm_put %s\n", obj);
			return(-1);
		}
		if (previous != NULL) {
		//	tsll_destroy(previous, free_ll);
		}
		//print_the_table();

		// Add .c file to the work_queue
		tsll_addLast(work_queue, (void *)argv[i]);
		//print_ll("WQ", work_queue);

		// Add .c --> .h in the_table (start with empty)
		dep = tsll_create();
		if (dep == NULL) {
			printf("Error creating dep\n");
			return(-1);
		}

		previous = NULL;
		if (tshm_put(the_table, argv[i], dep, (void**)&previous) == 0) {
			printf("Error in tshm_put %s\n", obj);
			return(-1);
		}
		if (previous != NULL) {
			//tsll_destroy(previous, free_ll);
		}
		//print_the_table();
	}

	// at this point, we have all the .o's, .c's, .l's, and .y's in the_table 
	// and all the .cs in the work_queue
	//
	// Create numWorkerThreads number of threads to process the Work Q

	pthread_t	*tid;
	tid = (pthread_t *)malloc(numWorkerThreads * sizeof(pthread_t));
	if (tid == NULL) {
		printf("Error allocating tid array\n");
		return(-1);
	}

	for (i=0; i<numWorkerThreads; i++) {
		pthread_create(&tid[i], NULL, WorkerThread, NULL);
	}

	//printf("Main: Waiting for worker threads to finish\n");
	// Wait for all threads to complete
	for (i=0; i<numWorkerThreads; i++) {
		pthread_join(tid[i], NULL);
	}

	// Free the tid array
	free(tid);

	//printf("\n******** All Worker Threads done *******\n");
	//print_ll("WQ", work_queue);
	//printf("-------------------------------\n");
	//print_the_table();
	//printf("*******************************\n\n");

	TSLinkedList  	*to_process;
	TSLinkedList  	*printed;

	for (i=fstart; i<argc; i++) {
		parse_file(argv[i], fname, fext);
		
		obj = (char *)malloc(MAX_PATHLEN);
		if (obj == NULL) {
			printf("Error allocating obj\n");
			return (-1);
		}

		strcpy(obj, fname);
		strcat(obj, ".o");
		//printf("%s:", obj);

		printed = tsll_create();
		if (printed != NULL) {
			tsll_addLast(printed, obj);
		} else {
			printf("Error creating printed\n");
			return(-1);
		}

		obj = (char *)malloc(MAX_PATHLEN); //FREE
		strcpy(obj, fname);
		strcat(obj, ".o");
		//printf("%s:", obj);

		to_process = tsll_create();
		if (to_process != NULL) {
			tsll_addLast(to_process, obj);
		} else {
			printf("Error creating to_process\n");
			return(-1);
		}

		print_dependencies(the_table, printed, to_process);
		printf("\n");

		tsll_destroy(printed, NULL);

	}

	tsll_destroy(work_queue, NULL);
	tshm_destroy(the_table, NULL);
	return 0;
}
