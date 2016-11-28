#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>



void parse_inputs (int argc, char *argv[], char **logFile, char **configFile) {
	for (int i = 0; i < argc; i++) {
		//if we are not at the last argument
		if (i != argc - 1) {
			//if -l or -c, the file is the next argument
			if (!strcmp(argv[i], "-l")) {
				*logFile = argv[i+1];
			}
			else if (!strcmp(argv[i], "-c")) {
				*configFile = argv[i+1];
			}
		}
	}
}

void main (int argc, char *argv[]) {
	char *logFile    = NULL;
	char *configFile = NULL;

	parse_inputs(argc, argv, &logFile, &configFile);
	printf("%s\n", logFile);
	printf("%s\n", configFile);
}