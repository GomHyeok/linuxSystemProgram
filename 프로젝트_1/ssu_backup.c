#include "ssu_backup.h"

int main(int argc, char* argv[]){
	char *username;
	char message[1024];

	if(argc != 2){
		printf("Usage: ssu_backup <md5 | sha1>\n");
		return 0;
	}
	else if(strcmp(argv[1], "md5")==0 || strcmp(argv[1], "sha1") == 0){
		
	}
	else{
		printf("Usage: ssu_backup <md5 | sha1>\n");
		return 0;
	}

	username = getlogin();
	snprintf(message, 1024, "/home/%s/backup", username);

	if(access(message, F_OK) == -1){
		if(mkdir(message, 0755) == -1){
			perror("mkdir Error");
			exit(EXIT_FAILURE);
		}
	}

	int res = 0;
	while(1){

		if(!res){
			printf("20193115> ");
		}

		char* myCommand = (char *)malloc(MAX_COMMAND_LENGTH);
		fgets(myCommand, MAX_COMMAND_LENGTH, stdin);

		int maxArgs = 10; // maximum number of arguments
		char** arguments = (char**)malloc(maxArgs * sizeof(char*));
		for(int i = 0; i < maxArgs; i++) {
			arguments[i] = (char*)malloc(MAX_COMMAND_LENGTH * sizeof(char));
		}

		res = parseArguments(myCommand, arguments, maxArgs);
		if (res) {
			printf("error filename length: more than 4096\n");
			memset(myCommand, 0, sizeof(myCommand)); 
			continue;
		}

		int argCnt = 0;
		while(arguments[argCnt] != NULL) {
			argCnt++;
   		}

		// null이 들어간경우 비교 불가능 하기 때문에 null인지 먼저 check
		if (arguments[0] == NULL){
			continue;
		}

		arguments[argCnt] = (char*)malloc(5*sizeof(char));
		strcpy(arguments[argCnt], argv[1]);
		arguments[argCnt+1] = NULL;

		char workingPath[4096];
		char *check_path;

		check_path = realpath(argv[0], NULL);

        if (check_path == NULL) { //실제 경로 생성
            realpath(argv[0], workingPath);
        } 
        else {
            realpath(argv[0], workingPath);
        }
		dirname(workingPath);
		strcat(workingPath, "/");
		
		if(strcmp(arguments[0],"add")==0){
			pid_t pid = fork();
			if(pid<0){
				printf("fork error");
				exit(EXIT_FAILURE);
			}
			else if(pid == 0){
				execv(strcat(workingPath,"ssu_backup_add"), arguments);
				exit(EXIT_FAILURE);
			}
			else{
				wait(NULL);
			}
		}
		else if(strcmp(arguments[0],"remove")==0){
			pid_t pid = fork();

			if(pid<0){
				printf("Fork Error");
				exit(EXIT_FAILURE);
			}
			else if(pid == 0){
				execv(strcat(workingPath,"ssu_backup_remove"), arguments);
				exit(EXIT_FAILURE);
			}
			else{
				wait(NULL);
			}

		}
		else if(strcmp(arguments[0],"recover")==0){
			pid_t pid = fork();

			if(pid<0){
				printf("fork error");
				exit(EXIT_FAILURE);
			}
			else if(pid == 0){
				execv(strcat(workingPath,"ssu_backup_recover"), arguments);
				exit(EXIT_FAILURE);
			}
			else{
				wait(NULL);
			}

		}
		else if(strcmp(arguments[0],"ls")==0){
			arguments[argCnt] = NULL;
			pid_t pid;
			pid = fork();

			if(pid<0){
				printf("Fork Error");
				exit(EXIT_FAILURE);
			}
			else if(pid==0){
				execv("/bin/ls", arguments);
				exit(EXIT_FAILURE);
			}
			else{
				wait(NULL);
			}
		}
		else if(strcmp(arguments[0], "vi")==0 || strcmp(arguments[0], "vim")==0){
			arguments[argCnt] = NULL;
			pid_t pid = fork();

			if(pid<0){
				printf("Fork Error");
				exit(EXIT_FAILURE);
			}
			else if(pid==0){
				execv("/usr/bin/vi", arguments);
				exit(EXIT_FAILURE);
			}
			else{
				wait(NULL);
			}
		}
		else if(strcmp(arguments[0],"exit")==0){
			exit(EXIT_SUCCESS);
		}
		else{
			arguments[argCnt-1] =NULL;
			pid_t pid = fork();

			if(pid<0){
				printf("fork error");
				exit(EXIT_FAILURE);
			}
			else if(pid == 0){
				execv(strcat(workingPath,"ssu_backup_help"), arguments);
				exit(EXIT_FAILURE);
			}
			else{
				wait(NULL);
			}
		}
		printf("\n");
	}
}


