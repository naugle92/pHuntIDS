#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h> 
#include <sys/time.h> 
#include <sys/resource.h>
#include <sys/ptrace.h>

#define ERR -1
#define logSize 200
#define tempStringSize 6
#define startupStringSize 26
#define SUCCESS 1
#define FAILURE 0
#define LOWERPRIORITY -20

//struct used to get the linux version
struct utsname linVersion;
typedef struct {
    long size,resident,share,text,lib,data,dt;
} stats;


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

//figures out what to kill and kills it, returns 1 for success, 0 for failure
int killit(char * action, char * type, char * param, FILE **logFile) {
	
	char *name;
	int processID;

	long nothing;
	long actualMemoryUsed;
	FILE *statmFile;

	//stat struct that holds the uid needed for username
	struct stat st;
	//open /proc directory holding the process files
	DIR *d;
	struct dirent *dir;
	d = opendir("/proc");
	if (d) {
		//read through all files and find the owner, if it is blacklist owner, kill it
		while ((dir = readdir(d)) != NULL) {

			stat(concatenate("/proc/",dir->d_name), &st);
		    struct passwd *pass;
			pass = getpwuid(st.st_uid);

			//printf("%s\n", dir->d_name);
			processID = atoi(dir->d_name);

			//if we need to mess with the user
			if (strcmp(type, "user") == 0) {
	
				//kill the process if it is the blacklisted user
				if (strcmp(pass->pw_name, param) == 0) {
					
					//kill the user and report it
					if (strcmp("kill", action) == 0) {
						enterLog(logFile, createEntry(concatenate("Killing process number ", dir->d_name)));
						printf("%lld memory, bitches\n", (long long)st.st_blocks);
						if (kill(processID, SIGKILL) == 0) 
							enterLog(logFile, createEntry(concatenate("Successfully killed process number ", dir->d_name)));
						else
							enterLog(logFile, createEntry(concatenate("Did NOT successfully kill process number ", dir->d_name)));
					//suspendthe user and report it
					} else if (strcmp("suspend", action) == 0) {
						enterLog(logFile, createEntry(concatenate("Suspending process number ", dir->d_name)));
						if (kill(processID, SIGTSTP) == 0)
							enterLog(logFile, createEntry(concatenate("Successfully suspended process number ", dir->d_name)));
						else
							enterLog(logFile, createEntry(concatenate("Did NOT successfully suspend process number ", dir->d_name)));
					
					//nice the user and report it
					} else if (strcmp("nice", action) == 0) {
						enterLog(logFile, createEntry(concatenate("Nicing process number ", dir->d_name)));
						if (setpriority(PRIO_PROCESS, processID, LOWERPRIORITY) == 0) 
							enterLog(logFile, createEntry(concatenate("Successfully niced process number ", dir->d_name)));
						else
							enterLog(logFile, createEntry(concatenate("Did NOT successfully nice process number ", dir->d_name)));
					}
				} 



			//if we are affecting based on path
			} else if (strcmp(type, "path") == 0) {
				//pathname is the path found by readlink based on the pid
				char * callingDir = malloc(70);
				char * procpath = concatenate(concatenate("/proc/", dir->d_name), "/exe");
				char   root[256];
				//read all of the exe files within the /proc/*/ directory
				readlink(procpath, callingDir, 70);
						
				//printf("%s is the path\n", pathname);

				if (strcmp(param, "/Documents") == 0) {
					printf("works\n");
				}

				//get first n characters of the string
				strncpy(root, callingDir, strlen(param));
				//null terminate destination
				root[strlen(param)] = 0;
				//printf("%s\n", root);
				 
				//if the directory matches the listed one
				if (strcmp(root, param) == 0) {
					//printf("hello\n");
					if (strcmp("kill", action) == 0) {
						enterLog(logFile, createEntry(concatenate("Killing process number ", dir->d_name)));
						if (kill(processID, SIGKILL) == 0) 
							enterLog(logFile, createEntry(concatenate("Successfully killed process number ", dir->d_name)));
						else
							enterLog(logFile, createEntry(concatenate("Did NOT successfully kill process number ", dir->d_name)));
					} else if (strcmp("suspend", action) == 0) {
						
						enterLog(logFile, createEntry(concatenate("Suspending process number ", dir->d_name)));
						if (kill(processID, SIGTSTP) == 0)
							enterLog(logFile, createEntry(concatenate("Successfully suspended process number ", dir->d_name)));
						else
							enterLog(logFile, createEntry(concatenate("Did NOT successfully suspend process number ", dir->d_name)));
					
					} else if (strcmp("nice", action) == 0) {
						
						enterLog(logFile, createEntry(concatenate("Nicing process number ", dir->d_name)));
						if (setpriority(PRIO_PROCESS, processID, LOWERPRIORITY) == 0) 
							enterLog(logFile, createEntry(concatenate("Successfully niced process number ", dir->d_name)));
						else
							enterLog(logFile, createEntry(concatenate("Did NOT successfully nice process number ", dir->d_name)));
					}
				}



			//if we are affecting based on memory
			} else if (strcmp(type, "memory") == 0) {
				//printf("%d memory, bitches\n", (int)st.st_size);		
				//open the statm file of each process to read the mem usage
				statmFile = fopen(concatenate(concatenate("/proc/", dir->d_name), "/statm"),"r");
				if(!statmFile){
					printf("no statm file\n");;
					//break;
				} else {
					//place the rss into a variable we can use
					fscanf(statmFile,"%ld %ld", &nothing ,&actualMemoryUsed);
					//printf("process %s using %ld memory\n", dir->d_name, actualMemoryUsed);
					
				}
			}	    
		}

	closedir(d);
	}

	
	return FAILURE;
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

	char * line = NULL;
	size_t length = 0;
	ssize_t read;
	char * command = " ";
	char * action = " ";
	char * type = " ";
	char * param = " ";



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
	while ((read = getline(&line, &length, configFile)) != -1) {
    	//if this line is an actual command
        if (line[0] != '#' && read > 1) {
        	command = strtok(line, " ");
		    action = command;
		    command = strtok (NULL, " ");
		    type = command;
		    command = strtok (NULL, " \n");
		    param = command;
		    killit(action, type, param, &logFile);
		}	
    }



	//enter the end of the program	
	enterLog(&logFile, createEntry("phunt ended, closing files."));

	//close files when done
	fclose(logFile);
	fclose(configFile);

}