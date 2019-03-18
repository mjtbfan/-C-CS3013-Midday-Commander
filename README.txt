Nicholas Delli Carpini | CS 3013 | Project 1

------Tests-----

MC0 https://pastebin.com/3kGgZ7Zt

MC1 https://pastebin.com/Kr7q6G3M

MC2 https://pastebin.com/PCMwy5J1

-----Running-----
Should run like the instructions specify, with the only exception being that if the user doesn't want any arguments when using option 2, the user can type NULL to only run ls with 1 or no arguments.

Also, using fgets for all inputs allows input files to be processed the same way as user input, as long as they are passed in through stdin
------HEADERS-----
typedef struct commandStruct { //struct for commandList
typedef struct backgroundStruct { //struct for background tasks

command *OGList() { //builds the initial list of commands
command *makeCommandList(command *oldList, int num, char *com) { // builds additional list of commands
backcom *makeBackList(backcom *oldList, char *command, int totBack) { // builds the list of background tasks (not perfect)
void intro(command *commandList, int totOption) { // Option select
void statPrint(struct timespec sttime, struct timespec etime) { // prints time and other stats
void execute(command *commandList, int option) { // executes all commands except option 2
void runBack(backcom *backList, int totBack) { // runs the background commands
void executels(command *commandList, char *lsargs, char *lspath) { // executes ls taking the args and paths
int runUser() { // pretty much main, don't know why i did it like this
---------------------------

-----ISSUES-----
Both Major Issues Involving the code were to do with background tasks. For starters, if you call sleep more than once as a background task it will only work the first time; however, other commands that I have tested don't seem prone to thsi problem.
The r function has some strange things when involving sleep. If you call sleep, all past processes will be wiped and the only one on the list will be sleep; however, it shows the processes fine before sleep finishes.
Also, due to time constraints I was not able to put in a way to stop the user from exiting the program during any background process, instead e will exit the background process, but the program will still be running.
----------------

-----Background Task Code------
typedef struct backgroundStruct { //struct for background tasks
	int completed; // 0 or 1
	int num; // number of the task
	pid_t pid; //pid of task (due to the code is very volatile)
	char com[128]; // string of command
} backcom;

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
-------------------------

