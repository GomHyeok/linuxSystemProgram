#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libgen.h>

#define MAX_COMMAND_LENGTH 4096
#define MAX_PATH_LENGTH 255

int parseArguments(char* input, char** arguments, int maxArgs);

int parseArguments(char* input, char** arguments, int maxArgs) {
    int i = 0;
    char* token = strtok(input, " \n");
    int sum = 0;
    while(token != NULL && i < maxArgs) {
        arguments[i] = token;
        sum+= strlen(token)+1;
        if (sum >= 4096) break;
        token = strtok(NULL, " \n");
        i++;
      
    }
    
    // Null terminate the arguments list
    arguments[i] = NULL;

    if (sum>=4096) return 1;
    else return 0;
}