Nicholas Delli Carpini | CS 3013 | Project 1 Checkpoint 1

------Makefile-----
mc0: mc0.c
	gcc -o mc0 mc0.c
clean:
	rm -f mc0
all: mc0
-------------------

-----Running-----
Should run like the instructions specify, with the only exception being that if the user doesn't want any arguments when using option 2,
the user can type NULL to only run ls with 1 or no arguments.

------FUNCTION HEADERS-----
void intro() // Option select
void statPrint(struct timespec sttime, struct timespec etime) // prints time and other stats
void execute(int option, char* args[]) // executes the 3 different commands
---------------------------

-----Important Code------
double elapsed = ((double)(((etime.tv_sec - sttime.tv_sec) * 1000000000) + etime.tv_nsec - sttime.tv_nsec) / 1000000); //calculates the elapsed time in ns, then converts to ms
PAGE FAULT CODE
	getrusage(RUSAGE_CHILDREN, &usage); // Page Faults
	long pf = usage.ru_majflt; // Page Faults
	long pfr = usage.ru_minflt; // Page Faults Reclaimed
ARG AND PATH
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
-------------------------

-----ISSUES-----
The major issue I was unable to fix was that the clock would count the time it takes the user to input args and path for option 2.
This leads to inaccurate reading of option 2's time; however the time works perfect for the other two options
----------------