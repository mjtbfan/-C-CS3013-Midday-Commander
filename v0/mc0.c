#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

struct rusage usage;
struct timespec sttime, etime, sltime;

void intro() { // Option select
	printf("G'day, Commander! What command would you like to run\n");
	printf("  0. whoami  : Prints out the result of the whoami command\n");
	printf("  1. last    : Prints out the result of the last command\n");
	printf("  2. ls      : Prints out the result of a listing on a user-specified path\n");
	printf("Option?:");
}

void statPrint(struct timespec sttime, struct timespec etime) { // prints time and other stats
	printf("\n--- Statistics ---\n");
	double elapsed = ((double)(((etime.tv_sec - sttime.tv_sec) * 1000000000) + etime.tv_nsec - sttime.tv_nsec) / 1000000); //calculates the elapsed time in ns, then converts to ms
	printf("Elapsed Time: %.2f ms\n", elapsed);
	getrusage(RUSAGE_CHILDREN, &usage); // Page Faults
	long pf = usage.ru_majflt; // Page Faults
	long pfr = usage.ru_minflt; // Page Faults Reclaimed
	printf("Page Faults: %ld\n", pf);
	printf("Page Faults (Reclaimed): %ld\n\n", pfr);
}

void execute(int option, char* args[]) { // executes the 3 different commands
	pid_t pid = fork();
	clock_gettime(CLOCK_MONOTONIC_RAW, &sttime); // start time
	if (option == 0) { // whoami
		if (pid == -1) { // error
			perror("fork");
		}
		if (pid == 0) { //child
			args[0] = (char*) "whoami";
			args[1] = NULL;
			execvp(args[0], args);
		}
		if (pid > 0) { //parent
			wait(0);
			clock_gettime(CLOCK_MONOTONIC_RAW, &etime); // end time
			statPrint(sttime, etime);
		}
	}
	else if (option == 1) { // last
		if (pid == -1) { //error
			perror("fork");
		}
		if (pid == 0) { //child
			args[0] = (char*) "last";
			args[1] = NULL;
			execvp(args[0], args);
		}
		if (pid > 0) { //parent
			wait(0);
			clock_gettime(CLOCK_MONOTONIC_RAW, &etime);
			statPrint(sttime, etime); // end time
		}
	}
	else if (option == 2) { // ls
		if (pid == -1) { //error
			perror("fork");
		}
		if (pid == 0) { //child
			char* lsargs = malloc(sizeof(char) * 32);
			char* lspath = malloc(sizeof(char) * 32);
			printf("Arguments? ('NULL' for none):");
			scanf("%s", lsargs);
			printf("Paths? ('NULL' for none):");
			scanf("%s", lspath);
			printf("\n");
			if(strcmp(lsargs, "NULL") == 0 && strcmp(lspath, "NULL") == 0) { // checks for args and path inputs
				char* argls[2] = {"ls", NULL} ;
				execvp(argls[0], argls);
			} else if(strcmp(lsargs, "NULL") == 0) {
				char* argls[3] = {"ls", lspath, NULL} ;
				execvp(argls[0], argls);
			} else if (strcmp(lspath, "NULL") == 0) {
				char* argls[3] = {"ls",lsargs, NULL} ;
				execvp(argls[0], argls);
			} else {
				char* argls[4] = {"ls", lsargs, lspath, NULL} ;
				execvp(argls[0], argls);
			}
		}
		if (pid > 0) { //parent
			wait(0);
			clock_gettime(CLOCK_MONOTONIC_RAW, &etime); // end time - currently counts the time for inputting arg and path
			statPrint(sttime, etime);
		}
	}
}

int main() {
	char optionC[0];
	char* args[2];
	int option;
	printf("\n----- Mid-Day Commander, v0 -----\n");
	while(1) {
		intro();
		scanf("%s", optionC);
		if (strcmp(optionC, "0") != 0 && strcmp(optionC, "1") != 0 && strcmp(optionC, "2") != 0) { // checks for valid option
			printf("\nError: Invalid Option\n");
			continue;
		} else {
			option = atoi(optionC);
			if (option == 0) {
				printf("\n--- Who Am I? ---\n\n");
			}
			else if (option == 1) {
				printf("\n--- Last Logins ---\n\n");
			}
			else if (option == 2) {
				printf("\n--- Directory Listing ---\n");
			}
		}
		execute(option, args);
	}
	return 0;
}
