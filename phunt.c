#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>

#define ERR -1

//parse the arguments
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

//open the log and config files if they can be opened, otherwise throw an error
void openFiles (FILE **logFile, FILE **configFile, char *log, char *config) {
	printf("%s\n", log);
	*logFile = fopen(log, "a");
	if (*logFile == NULL) 
		perror ("Error opening log file");
	
	if( access( config, R_OK ) != -1 ) {
    // file exists and we are able to read it
		printf("%s\n", config);
		*configFile = fopen(config, "r");
		if (*configFile == NULL) 
			perror ("Error opening config file");
	} else {
    // file doesn't exist
		printf("Config file %s does not exist or is not accessible\n", config);
		fclose(*logFile);
		exit(ERR);
	}
}

//return the current time as a string
char * timeStamp() {
	time_t currentTime;
	currentTime = time(NULL);
	return asctime(localtime(&currentTime));
}

void main (int argc, char *argv[]) {
	char *log    = "/var/log/phunt.log";
	char *config = "/etc/phunt.conf";

	parse_inputs(argc, argv, &log, &config);

	FILE *logFile;
	FILE *configFile;

	openFiles(&logFile, &configFile, log, config);
	
	printf("%s", timeStamp());



	//close files when done
	fclose(logFile);
	fclose(configFile);
}