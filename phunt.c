#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>

#define ERR -1

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

void openFiles (FILE *logFile, FILE *configFile, char *log, char *config) {
	printf("%s\n", log);
	logFile = fopen(log, "a");
	
	if( access( config, R_OK ) != -1 ) {
    // file exists
		printf("%s\n", config);
		configFile = fopen(config, "r");
	} else {
    // file doesn't exist
		printf("Config file %s does not exist or is not accessible\n", config);
		fclose(logFile);
		exit(ERR);
	}
}

int main (int argc, char *argv[]) {
	char *log    = "/var/log/phunt.log";
	char *config = "/etc/phunt.conf";

	parse_inputs(argc, argv, &log, &config);

	FILE *logFile;
	FILE *configFile;

	openFiles(logFile, configFile, log, config);
	
	
	//close files when done
	fclose(logFile);
	fclose(configFile);

	return 1;
}