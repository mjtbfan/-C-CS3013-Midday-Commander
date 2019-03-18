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

typedef struct commandStruct { //struct for commandList
	int num;
	char com[128];
	char desc[100];
} command;

typedef struct backgroundStruct { //struct for background tasks
	int completed; // 0 or 1
	int num; // number of the task
	pid_t pid; //pid of task (due to the code is very volatile)
	char com[128]; // string of command
} backcom;

struct rusage usage; // for usage for both background and foreground
struct timespec sttime, etime, sltime; // for time

long int oldpf = 0; //keeps old page faults and then subtracts the new ones -> accurate page faults
long int oldpfr = 0;

command *OGList() { //builds the initial list of commands
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

command *makeCommandList(command *oldList, int num, char *com) { // builds additional list of commands
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

backcom *makeBackList(backcom *oldList, char *command, int totBack) { // builds the list of background tasks (not perfect)
	backcom *backList = malloc(sizeof(backcom) * 100);
	for (int i = 0; i < totBack; i++) {
		backList[i].completed = oldList[i].completed;
		backList[i].num = oldList[i].num;
		backList[i].pid = oldList[i].pid;
		strcpy(backList[i].com, oldList[i].com);
	}
	free(oldList);
	backList[totBack].completed = 0;
	backList[totBack].pid = 0;
	backList[totBack].num = totBack;
	strcpy(backList[totBack].com, command);
	return backList;
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
	printf("  p. PWD - Prints working directory\n");
	printf("  r. Running Processes - Prints list of running processes\n\n");
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

void execute(command *commandList, int option) { // executes all commands except option 2
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

void runBack(backcom *backList, int totBack) { // runs the background commands
	pid_t bgpid = 0;
	char noAmp[128];
	strcpy(noAmp, backList[totBack].com);
	for (int j = 0; ;j++) { // removes the & from the commands so they can be parsed into execvp
		if (noAmp[j] == '&') {
			noAmp[j] = '\0';
			break;
		}
	}
	pid_t pid = fork();
	clock_gettime(CLOCK_MONOTONIC_RAW, &sttime);
	if (pid < 0) {
		perror("fork");
	}
	if (pid == 0) {
		char *args[128]; // arg list
		char *temp; // temp array
		char **i = args; // iterator for args
		temp = strtok(noAmp, " ");
		printf("%s", temp);
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
	if (pid > 0) {
		backList[totBack].pid = getpid();
		sleep(0.5);
		printf("\n[%d] [%d] %s is running in the background\n\n", backList[totBack].num, (int) backList[totBack].pid, backList[totBack].com);
		int grandFork = fork();
		if (grandFork != 0) {// fork for multiple bg processes
			while(bgpid != -1) { // loops until bg process is done -> leads to some issues with too many children currently
				bgpid = waitpid(bgpid, NULL, WNOHANG);
				if (bgpid > 0) { // checks for finish
					clock_gettime(CLOCK_MONOTONIC_RAW, &etime);
					printf("\n--- PROCESS FINISHED [%d] ---\n", bgpid);
					statPrint(sttime, etime);
					backList[totBack].completed = 1;
					kill(grandFork, SIGKILL); // attempt to fix
					break;
				}
			}
		}
	}
}

void executels(command *commandList, char *lsargs, char *lspath) { // executes ls taking the args and paths
	pid_t pid = fork();
	clock_gettime(CLOCK_MONOTONIC_RAW, &sttime);
	if (pid == -1) { //error
		perror("fork");
	}
	if (pid == 0) { //child
		if (strcmp(lsargs, "NULL") == 0 && strcmp(lspath, "NULL") == 0) { // args and path inputs
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

int runUser() { // pretty much main, don't know why i did it like this
	backcom *backList = malloc(sizeof(backcom) * 100);
	command *commandList = malloc(sizeof(command) * 100);
	commandList = OGList();

	char optionC[128];
	char addedCom[128];

	int totOption = 3;
	int totBack = 0;
	int option;
	int errorOption;

	printf("\n----- Mid-Day Commander, v0 -----\n");

	while (1) {
		// error checking for input
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
				&& strcmp(optionC, "p") != 0 && strcmp(optionC, "r") != 0) {
			// checks for valid option
			printf("\nError: Invalid Option\n\n");
			continue;
			// this is where the magic happens
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
				free(backList);
				exit(1);

			} else if (strcmp(optionC, "p") == 0) {
				char curDir[128];
				printf("\n-- Current Directory --\n\n");
				getcwd(curDir, 128);
				printf("Directory: %s", curDir);
				printf("\n\n");

			} else if (strcmp(optionC, "r") == 0) { //only shows the most recent running and completed tasks, probably some error with backList
				printf("\n--- Background Tasks ---\n");
				for(int i = 1; i < totBack + 1; i++) {
					if(backList[i].completed == 1) {
						printf("[Completed]");
					} else {
						printf("[Running]");
					}
					printf("[%d] [%d] %s\n", backList[i].num, (int) backList[i].pid, backList[i].com);
				}
				printf("\n");
			} else {
				option = atoi(optionC);
				if (option == 0) {
					printf("\n--- Who Am I? ---\n\n");
					execute(commandList, option);

				} else if (option == 1) {
					printf("\n--- Last Logins ---\n\n");
					execute(commandList, option);

				} else if (option == 2) { // ls shenanigans
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

				} else { // actual execs
					if (strstr(commandList[option].com, "&") != NULL) { // check if back or fore
						totBack++;
						printf("\n--- User Command, %s ---\n", commandList[option].com);
						backList = makeBackList(backList, commandList[option].com, totBack);
						runBack(backList, totBack);
					} else {
						printf("\n--- User Command, %s ---\n\n", commandList[option].com);
						execute(commandList, option);
					}
				}
			}
		}
	}
	free(commandList);
	free(backList);
	return 0;
}

int main(int argc, char *argv[]) {
	return runUser(); // again no clue why i did this
}
