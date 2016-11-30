#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <sys/utsname.h>

#define ERR -1
#define logSize 200
#define tempStringSize 6
#define startupStringSize 26

//struct used to get the linux version
struct utsname linVersion;

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


//return the current time as a string and remove the newline at the end.
char * timeStamp() {
	time_t currentTime;
	currentTime = time(NULL);
	char *stringTime = asctime(localtime(&currentTime));
	stringTime[strlen(stringTime) - 1] = '\0';
	return stringTime;
}


//put together a string of all important information and log it
char * createEntry (char *news) {
	char *entry = malloc(logSize);
	entry[0] = '\0';
	//add timeStamp
	strcat(entry, timeStamp());
	strcat(entry, " ");
	strcat(entry, linVersion.nodename);
	strcat(entry, " ");
	strcat(entry, "phunt: ");
	strcat(entry, news);
	return entry;
}

//create the string that is logged on startup, contains the PID of this phunt
char * startup() {
	int pid = (int)getpid();
	char temp[tempStringSize];
	sprintf(temp, "%d", pid);

	char * startupString = malloc(startupStringSize);
	startupString[0] = '\0';
	strcat(startupString, "phunt startup (PID=");
	strcat(startupString, temp);
	strcat(startupString, ")");

	return startupString;
}

void enterLog(FILE **logFile, char *logEntry) {
	fprintf(*logFile, "%s\n", logEntry);
}


//appends two to the end of one, returns the result
char * concatenate(char *one, char *two) {
	char * result = malloc(50);
	strcpy(result, one);
	strcat(result, two);
	return result;
}


void main (int argc, char *argv[]) {

	//get the linux distribution for logging
	uname(&linVersion);
	//record when the program started
	char *pHuntStartString = createEntry(startup());
	char *tempString =  malloc(50);
	//default log and config files
	char *log    = "/var/log/phunt.log";
	char *config = "/etc/phunt.conf";

	parse_inputs(argc, argv, &log, &config);

	FILE *logFile;
	FILE *configFile;

	openFiles(&logFile, &configFile, log, config);

	//start logging important events
	enterLog(&logFile, pHuntStartString);

	//log the opening of the log file
	enterLog(&logFile, createEntry(concatenate("opened logfile ", log)));

	//log the start of parsing
	enterLog(&logFile, createEntry(concatenate("parsing configuration file ", config)));	
	
	//start actually parsing




	//enter the end of the program	
	enterLog(&logFile, createEntry("phunt ended, closing files."));

	//close files when done
	fclose(logFile);
	fclose(configFile);

}