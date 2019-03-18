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
typedef struct commandStruct {
	int num;
	char com[128];
	char desc[100];
} command;

long int oldpf = 0;
long int oldpfr = 0;

command *OGList() {
	command *OGList = malloc(sizeof(command) * 5);
	OGList[0].num = 0;
	strcpy(OGList[0].com, "whoami");
	strcpy(OGList[0].desc, "Prints out the result of the whoami command");

	OGList[1].num = 1;
	strcpy(OGList[1].com, "last");
	strcpy(OGList[1].desc, "Prints out the result of the last command");

	OGList[2].num = 2;
	strcpy(OGList[2].com, "ls"); //need to put special condition
	strcpy(OGList[2].desc, "Prints out the result of a lisiting on a user-specified path");

	return OGList;
}

command *makeCommandList(command *oldList, int num, char *com) {
	command *commandList = malloc(sizeof(command) * 100);
	for(int i = 0; i < num; i++) {
		commandList[i].num = oldList[i].num;
		strcpy(commandList[i].com, oldList[i].com);
		strcpy(commandList[i].desc, oldList[i].desc);
	}
	free(oldList);
	commandList[num].num = num;
	strcpy(commandList[num].com, com);
	strcpy(commandList[num].desc, "User Added Command");
	return commandList;
}

void intro(command *commandList, int totOption) { // Option select
	printf("G'day, Commander! What command would you like to run\n");
	int comLength;
	for(int i = 0; i < totOption; i++) {
		comLength = (10-(int)(strlen(commandList[i].com)));
		if (comLength < 0) {
			comLength = 1;
		}
		printf("  %d. %s %*c: %s\n", commandList[i].num, commandList[i].com, comLength, ' ', commandList[i].desc);
	}
	printf("  a. Add Command - Adds a new command to the menu\n");
	printf("  c. Change Directory - Changes process working directory\n");
	printf("  e. Exit - Leave Mid-Day Commander\n");
	printf("  p. PWD - Prints working directory\n\n");
	printf("Option?: ");
}

void statPrint(struct timespec sttime, struct timespec etime) { // prints time and other stats
	printf("\n--- Statistics ---\n");
	double elapsed = ((double)(((etime.tv_sec - sttime.tv_sec) * 1000000000) + etime.tv_nsec - sttime.tv_nsec) / 1000000); //calculates the elapsed time in ns, then converts to ms
	printf("Elapsed Time: %.2f ms\n", elapsed);
	getrusage(RUSAGE_CHILDREN, &usage); // Page Faults
	long pf = usage.ru_majflt - oldpf; // Page Faults
	long pfr = usage.ru_minflt - oldpfr; // Page Faults Reclaimed
	printf("Page Faults: %ld\n", pf);
	printf("Page Faults (Reclaimed): %ld\n\n", pfr);
	oldpf += pf;
	oldpfr += pfr;
}

void execute(command *commandList, int option) { // executes the 3 different commands
	pid_t pid = fork();
	clock_gettime(CLOCK_MONOTONIC_RAW, &sttime); // start time
	if (pid == -1) { // error
		perror("fork");
	}
	if (pid == 0) { //child
		char *args[128]; // arg list
		char *temp; // temp array
		char **i = args; // iterator for args
		temp = strtok(commandList[option].com, " ");
		while (temp != NULL) { // split commands into args by ' '
			*i++ = temp;
			temp = strtok(NULL, " ");
		}
		*i = NULL;
		if (execvp(args[0], args) < 0) {
			printf("\nError: Invalid Command\n\n");
			exit(-1);
		}
	}
	if (pid > 0) { //parent
		wait(0);
		clock_gettime(CLOCK_MONOTONIC_RAW, &etime); // end time
		statPrint(sttime, etime);
	}
}

void executels(command *commandList, char *lsargs, char *lspath) {
	pid_t pid = fork();
	clock_gettime(CLOCK_MONOTONIC_RAW, &sttime);
	if (pid == -1) { //error
		perror("fork");
	}
	if (pid == 0) { //child
		if (strcmp(lsargs, "NULL") == 0 && strcmp(lspath, "NULL") == 0) { // checks for args and path inputs
			char* argls[2] = { "ls", NULL };
			execvp(argls[0], argls);
		} else if (strcmp(lsargs, "NULL") == 0) {
			char* argls[3] = { "ls", lspath, NULL };
			execvp(argls[0], argls);
		} else if (strcmp(lspath, "NULL") == 0) {
			char* argls[3] = { "ls", lsargs, NULL };
			execvp(argls[0], argls);
		} else {
			char* argls[4] = { "ls", lsargs, lspath, NULL };
			execvp(argls[0], argls);
		}
	}
	if (pid > 0) { //parent
		wait(0);
		clock_gettime(CLOCK_MONOTONIC_RAW, &etime); // end time - currently counts the time for inputting arg and path
		statPrint(sttime, etime);
	}
}

int runUser() {
	int totOption = 3;
	command *commandList = malloc(sizeof(command) * 100);
	commandList = OGList();
	char optionC[32];
	char addedCom[128];
	int option;
	int errorOption;
	printf("\n----- Mid-Day Commander, v0 -----\n");
	while (1) {
		errorOption = 1;
		intro(commandList, totOption);
		fgets(optionC, 32, stdin);
		if (feof(stdin)) {
			strcpy(optionC, "e");
		}
		optionC[strcspn(optionC, "\n")] = '\0';
		for (int i = 1; i < totOption; i++) {
			if (atoi(optionC) == 0) {
				if (strcmp(optionC, "0") == 0) {
					errorOption = 0;
					break;
				}
			}
			if (atoi(optionC) == i) {
				errorOption = 0;
				break;
			}
		}
		if (errorOption != 0 && strcmp(optionC, "a") != 0
				&& strcmp(optionC, "c") != 0 && strcmp(optionC, "e") != 0
				&& strcmp(optionC, "p") != 0) {
			// checks for valid option
			printf("\nError: Invalid Option\n\n");
			continue;
		} else {
			if (strcmp(optionC, "a") == 0) {
				totOption++;
				printf("\n-- Add a Command --\n");
				printf("Command to add?: ");
				fgets(addedCom, 128, stdin);
				addedCom[strcspn(addedCom, "\n")] = '\0';
				printf("Okay, Added with ID %d\n\n", (totOption - 1));
				commandList = makeCommandList(commandList, (totOption - 1),
						addedCom);
				continue;
			} else if (strcmp(optionC, "c") == 0) {
				char chgDir[128];
				printf("\n-- Change Directory --\n\n");
				printf("New Directory?: ");
				fgets(chgDir, sizeof(chgDir), stdin);
				chgDir[strcspn(chgDir, "\n")] = '\0';
				if (chdir(chgDir) != 0) {
					printf("Error: Invalid Directory");
				}
				printf("\n");
			} else if (strcmp(optionC, "e") == 0) {
				printf("Logging you out, Commander\n");
				free(commandList);
				return 0;
			} else if (strcmp(optionC, "p") == 0) {
				char curDir[128];
				printf("\n-- Current Directory --\n\n");
				getcwd(curDir, 128);
				printf("Directory: %s", curDir);
				printf("\n\n");
			} else {
				option = atoi(optionC);
				if (option == 0) {
					printf("\n--- Who Am I? ---\n\n");
					execute(commandList, option);
				} else if (option == 1) {
					printf("\n--- Last Logins ---\n\n");
					execute(commandList, option);
				} else if (option == 2) {
					printf("\n--- Directory Listing ---\n\n");
					char* lsargs = malloc(sizeof(char) * 1024);
					char* lspath = malloc(sizeof(char) * 1024);
					printf("Arguments? ('NULL' for none):");
					fgets(lsargs, 1024, stdin);
					lsargs[strcspn(lsargs, "\n")] = '\0';
					printf("Paths? ('NULL' for none):");
					fgets(lspath, 1024, stdin);
					lspath[strcspn(lspath, "\n")] = '\0';
					printf("\n");
					executels(commandList, lsargs, lspath);
				} else {
					printf("\n--- User Command, %s ---\n\n", commandList[option].com);
					execute(commandList, option);
				}
			}
		}
	}
	free(commandList);
	return 0;
}

int main(int argc, char *argv[]) {
	return runUser();
}
