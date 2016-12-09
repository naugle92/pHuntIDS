#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h> 
#include <sys/resource.h>

#define ERR -1
#define logSize 200
#define tempStringSize 6
#define startupStringSize 26
#define SUCCESS 1
#define FAILURE 0
#define LOWERPRIORITY -20

//struct used to get the linux version
struct utsname linVersion;

int keepRunning = 1;


//call this if Ctrl-c is pressed, it will cause the while loop to exit and program to end gracefully
void youDontControlMe(int signal) {
	printf(" Program ended\n");
	keepRunning = 0;
}


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
	*logFile = fopen(log, "a");
	if (*logFile == NULL) {
		printf("Error opening log file\n");
		exit(0);
	}
	if( access( config, R_OK ) != -1 ) {
    // file exists and we are able to read it
		*configFile = fopen(config, "r");
		if (*configFile == NULL) {
			printf("Error opening config file\n");
			fclose(*logFile);	
			exit(0);
		}
	} else {
    // file doesn't exist
		printf("Config file %s does not exist or is not accessible\n", config);
		fclose(*logFile);
		exit(0);
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


//put together a string of all important information and use it on a log
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

//prints the given string to the logfile
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

int isSuspended(char * processID) {
	char * procpath = concatenate(concatenate("/proc/", processID), "/status");
	FILE * statusFile = fopen(procpath, "r");	

	if(statusFile == NULL) {
		printf("No status file for this document\n");
		fclose(statusFile);
		return 0;
	}

	char status[4][256];
	

	fscanf(statusFile, "%s %s %s %s", status[0],status[1],status[2],status[3]);
	fclose(statusFile);
	//if the status is T (stopped) send a 1
	if (strcmp(status[3], "T") == 0) 
		return 1;
	else
		return 0;
}


//This program handles the command. It kills, suspends, or nices appropriate files
int killit(char * action, char * type, char * param, FILE **logFile) {
	
	char *name;
	int processID;

	//placeholder, literally used for nothing else
	long nothing;
	long actualMemoryUsed;
	//statm file found in /proc/xx/ directory
	FILE *statmFile;

	//stat struct that holds the uid needed for username
	struct stat st;
	//open /proc directory holding the process files
	//DIR used to "navigate" directories
	DIR *d;
	struct dirent *dir;
	d = opendir("/proc");
	if (d) {
		//read through all files and find the owner, if it is blacklist owner, kill it
		while ((dir = readdir(d)) != NULL) {

			stat(concatenate("/proc/",dir->d_name), &st);
			struct passwd *pass;
			pass = getpwuid(st.st_uid);

			processID = atoi(dir->d_name);

			//if we need to mess with the user
			if (strcmp(type, "user") == 0) {
	
				//if the process is owned by the blacklisted user
				if (strcmp(pass->pw_name, param) == 0) {
					
					//kill the process and report it
					if (strcmp("kill", action) == 0) {
						enterLog(logFile, createEntry(concatenate("Killing process number ", dir->d_name)));
						if (kill(processID, SIGKILL) == 0) 
							enterLog(logFile, createEntry(concatenate("Successfully killed process number ", dir->d_name)));
						else
							enterLog(logFile, createEntry(concatenate("Did NOT successfully kill process number ", dir->d_name)));
					

					//suspend the process and report it
					} else if ((strcmp("suspend", action) == 0) && isSuspended(dir->d_name) == 0) {
						enterLog(logFile, createEntry(concatenate("Suspending process number ", dir->d_name)));
						if (kill(processID, SIGTSTP) == 0)
							enterLog(logFile, createEntry(concatenate("Successfully suspended process number ", dir->d_name)));
						else
							enterLog(logFile, createEntry(concatenate("Did NOT successfully suspend process number ", dir->d_name)));
					

					//nice the process and report it
					} else if (strcmp("nice", action) == 0) {
						if (getpriority(PRIO_PROCESS, processID) > -20) {
						
							enterLog(logFile, createEntry(concatenate("Nicing process number ", dir->d_name)));
							if (setpriority(PRIO_PROCESS, processID, LOWERPRIORITY) == 0) 
								enterLog(logFile, createEntry(concatenate("Successfully niced process number ", dir->d_name)));
							else
								enterLog(logFile, createEntry(concatenate("Did NOT successfully nice process number ", dir->d_name)));
						}
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

				//get first n characters of the string
				strncpy(root, callingDir, strlen(param));
				//null terminate destination
				root[strlen(param)] = 0;
				 
				//if the directory matches the listed one
				if (strcmp(root, param) == 0) {
					
					//kill the process, report it
					if (strcmp("kill", action) == 0) {
						enterLog(logFile, createEntry(concatenate("Killing process number ", dir->d_name)));
						if (kill(processID, SIGKILL) == 0) 
							enterLog(logFile, createEntry(concatenate("Successfully killed process number ", dir->d_name)));
						else
							enterLog(logFile, createEntry(concatenate("Did NOT successfully kill process number ", dir->d_name)));
					

					//suspend the process, report it
					} else if ((strcmp("suspend", action) == 0) && isSuspended(dir->d_name) == 0) {
						enterLog(logFile, createEntry(concatenate("Suspending process number ", dir->d_name)));
						if (kill(processID, SIGTSTP) == 0)
							enterLog(logFile, createEntry(concatenate("Successfully suspended process number ", dir->d_name)));
						else
							enterLog(logFile, createEntry(concatenate("Did NOT successfully suspend process number ", dir->d_name)));
					
					
					//nice the process
					} else if (strcmp("nice", action) == 0) {
						

						if (getpriority(PRIO_PROCESS, processID) > -20) {
							enterLog(logFile, createEntry(concatenate("Nicing process number ", dir->d_name)));
							if (setpriority(PRIO_PROCESS, processID, LOWERPRIORITY) == 0) 
								enterLog(logFile, createEntry(concatenate("Successfully niced process number ", dir->d_name)));
							else
								enterLog(logFile, createEntry(concatenate("Did NOT successfully nice process number ", dir->d_name)));
						}
					}
				}



			//if we are affecting based on memory
			} else if (strcmp(type, "memory") == 0) {	
				//open the statm file of each process to read the mem usage
				statmFile = fopen(concatenate(concatenate("/proc/", dir->d_name), "/statm"),"r");
				if(statmFile == NULL){
					continue;

				} else {
					//place the rss into a variable we can use. multiply by 4K to get the bytes used
					fscanf(statmFile,"%ld %ld", &nothing ,&actualMemoryUsed);
					actualMemoryUsed *= 4096;
					char * stringBytes = concatenate(param, "000000");
					int numberBytesAllowed = atoi(stringBytes);

					//if the process is using more than is allowed, handle it
					if (actualMemoryUsed > numberBytesAllowed) {


						if (strcmp("kill", action) == 0) {
							enterLog(logFile, createEntry(concatenate("Killing process number ", dir->d_name)));
							if (kill(processID, SIGKILL) == 0) 
								enterLog(logFile, createEntry(concatenate("Successfully killed process number ", dir->d_name)));
							else
								enterLog(logFile, createEntry(concatenate("Did NOT successfully kill process number ", dir->d_name)));
						

						} else if ((strcmp("suspend", action) == 0) && isSuspended(dir->d_name) == 0) {
							
							enterLog(logFile, createEntry(concatenate("Suspending process number ", dir->d_name)));
							if (kill(processID, SIGTSTP) == 0)
								enterLog(logFile, createEntry(concatenate("Successfully suspended process number ", dir->d_name)));
							else
								enterLog(logFile, createEntry(concatenate("Did NOT successfully suspend process number ", dir->d_name)));
						
						

						} else if (strcmp("nice", action) == 0) {
							//if the nice value is not already the lowest value, change it
							if (getpriority(PRIO_PROCESS, processID) > -20) {
								enterLog(logFile, createEntry(concatenate("Nicing process number ", dir->d_name)));
								if (setpriority(PRIO_PROCESS, processID, LOWERPRIORITY) == 0) 
									enterLog(logFile, createEntry(concatenate("Successfully niced process number ", dir->d_name)));
								else
									enterLog(logFile, createEntry(concatenate("Did NOT successfully nice process number ", dir->d_name)));
							}
						}
					}
				}
				fclose(statmFile);
			}	    
		}
	closedir(d);
	}
	return SUCCESS;
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

	signal(SIGINT, youDontControlMe);

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
	
	while(keepRunning) {
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
	    //wait one second and rewind to the top of the config file
	    sleep(1);
	    fseek(configFile, 0, SEEK_SET);
	}


	//enter the end of the program	
	enterLog(&logFile, createEntry("phunt ended, closing files."));

	//close files when done
	fclose(logFile);
	fclose(configFile);

}