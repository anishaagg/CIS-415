/*
Author: Anisha Aggarwal
DuckID: anishaa
Title: CIS 415 Project 0
File: date.c
Statement: All work included in this file is my own
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "date.h"

struct date {
	int day;
	int month;
	int year;
};

/*
* date_create creates a Date structure from `datestr`
* returns pointer to Date structure if successful,
* NULL if not (syntax error)
*/
Date *date_create(char *datestr) {
	char date_buff[3];
	char month_buff[3];
	char year_buff[5];

	if (datestr == NULL) {
		return NULL;
	}

	memcpy(date_buff, &datestr[0], 2);
	date_buff[2] = '\0';
	memcpy(month_buff, &datestr[3], 2);
	month_buff[2] = '\0';
	memcpy(year_buff, &datestr[6], 4);
	year_buff[4] = '\0';

	Date *d = (Date*)malloc(sizeof(Date));
	//check if memory was allocated
	if (d != NULL) {
		d->day = atoi(date_buff);
		d->month = atoi(month_buff);
		d->year = atoi(year_buff);
	}

	return d;
}

/*
* date_duplicate creates a duplicate of `d'
* returns pointer to new Date structure if successful,
* NULL if not (memory allocation failure)
*/
Date *date_duplicate(Date *d) {
	Date *dup = (Date*)malloc(sizeof(Date));
	//check if memory was allocated
	if (dup != NULL) {
		dup->day = d->day;
		dup->month = d->month;
		dup->year = d->year;
	}

	return dup;
}

/*
* date_compare compares two dates, returning <0, 0, >0 if
* date1<date2, date1==date2, date1>date2, respectively
*/
int date_compare(Date *date1, Date *date2) {
	//printf("%d %d %d\n", date1->day, date1->month, date1->year);
	//printf("%d %d %d\n", date2->day, date2->month, date2->year);
	//check the year first
	if (date1->year < date2->year) {
		return -1;
	} else if (date1->year > date2->year) {
		return 1;
	} 
	//years are the same, check the months
	else {
		if (date1->month < date2->month) {
			return -1;
		} else if (date1->month > date2->month) {
			return 1;
		} 
		//months are the same, check the day
		else {
			if (date1->day < date2->day) {
				return -1;
			} else if (date1->day > date2->day) {
				return 1;
			} 
			//days are the same, return 0
			else {
				return 0;
			}
		}
	}
}

//returns any storage associated with `d' to the system
void date_destroy(Date *d) {
	free(d);
}


