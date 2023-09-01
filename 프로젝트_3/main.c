#include "ssu_monitor.h"

int main(int argc, char *argv[]) {
    int ftmp;
    char *cwd;
    char monitorPath[PATH_MAX];

    //monitor_list.txt 존재하지 않을 시 생성
    realpath("monitor_list.txt", monitorPath);
    
    if(access(monitorPath, F_OK) != 0) {
        ftmp = creat(monitorPath, 0644);
        close(ftmp);
    }

    while(1){
        //프롬프트
        printf("20193115> ");
		
        char input[100];
        char *arguments[5];
        int idx=0;

        fgets(input, sizeof(input), stdin);


        //엔터 입력시 프롬프트 재출력
        if(input[0] == '\n'){
            continue;
        }

        // 개행 문자 제거
        input[strcspn(input, "\n")] = '\0';

        // 공백을 기준으로 문자열 분리
        char *token = strtok(input, " ");
        while (token != NULL) {
            arguments[idx++] = token;
            token = strtok(NULL, " ");
        }

        //add 명령어 실행시
        if(strcmp(arguments[0],"add")==0){
            char absPath[PATH_MAX];
            FILE* file;
            char line[2048];
            char logPath[PATH_MAX + 10];
            //입력받은 디렉토리 없는 경우
            if(arguments[1] == NULL) {
                printf(" Usage : add <DIRPATH> [OPTION]\n");
                continue;
            }

            // 입력 경로가 상대경로인지 절대경로인지 확인 + 존재하지 않거나 접근할 수 없는 디렉토리의 경우 예외처리
            if(realpath(arguments[1], absPath)== NULL) {
                fprintf(stderr, " Usage : add <DIRPATH> [OPTION]\n");
                continue;
            }

            struct stat buf;
            if(stat(absPath, &buf) != 0) {
                fprintf(stderr, "stat error\n");
                continue;
            }

            //디렉토리가 아닌경우 예외처리
            if(!(S_ISDIR(buf.st_mode))) {
                printf(" Usage : add <DIRPATH> [OPTION]\n");
                continue;
            }

            if((file = fopen("monitor_list.txt", "r")) == NULL) {
                fprintf(stderr, "monitor_list.txt open error\n");
                continue;
            }

            //기존에 있는 디렉토리인지 검사
            int flag = 0;
            while((fgets(line, sizeof(line), file)) != NULL) {
                char *ptr;
                ptr = strtok(line, " ");
                if(strcmp(absPath, ptr) == 0) {
                    flag = 1;
                    break;
                }
                //하위 디렉토리 검사
                flag = checkPath(ptr, absPath);
                if(flag == 1){
                    break;
                }      
            }

            //기존에 있는 디렉토리인 경우
            if(flag == 1) {
                fprintf(stderr, "%s is in monitoring_list.txt\n", absPath);
                continue;
            }

            fclose(file);

            snprintf(logPath , sizeof(logPath), "%s/log.txt", absPath);
            //로그 파일이 존재하지 않는 경우 생성
            if(access(logPath, F_OK) != 0) {
                int ltmp = creat(logPath, 0644);
                close(ltmp);
            }

            int interval = 1;
            //t옵션 존재 유무 검사 및 예외처리
            if(arguments[2] != NULL && strcmp(arguments[2], "\0") != 0){
                if(strcmp(arguments[2], "-t") == 0 && arguments[3] != NULL) {
                    if(checkDigit(arguments[3])) {
                        fprintf(stderr, " Usage : add <DIRPATH> [OPTION]\n");
                        continue;
                    }
                    interval = atoi(arguments[3]);

                    if(interval <= 0){
                        fprintf(stderr, " Usage : add <DIRPATH> [OPTION]\n");
                        continue;
                    }
                }
                else {
                    fprintf(stderr, " Usage : add <DIRPATH> [OPTION]\n");
                    continue;
                }
            }
            //디몬 프로세스 생성 및 실행
            printf("monitoring started (%s)\n", absPath);
            pid_t pid = fork();
            if (pid == 0) {
                // 자식 프로세스
                monitor_directory(absPath, logPath, interval);
                exit(0);
            } else if (pid < 0) {
                perror("fork");
                exit(1);
            }
        }
        else if (strncmp(arguments[0], "delete", strlen("delete")) == 0) {
            char line[2048];
            FILE *file;

            //pid 입력이 없는 경우
            if(arguments[1] == NULL) {
                printf("Usage : delete <DAEMON_PID>\n");
                continue;
            }

            if(idx > 2) {
                printf("Usage : delete <DAEMON_PID>\n");
                continue;
            }
            
            //pid 
            int killpid = atoi(arguments[1]);
            
            if((file = fopen("monitor_list.txt", "r")) == NULL) {
                fprintf(stderr, "monitor_list.txt open error\n");
                continue;
            }
            //monitor_list.txt에서 일치하는 pid 탐색 및 경로저장
            int flag = 0;
            char prepath[PATH_MAX];
            char ids[20];
            while((fgets(line, sizeof(line), file)) != NULL) {
                int cnt = 0;
                char *token = strtok(line, " ");
                
                strcpy(prepath, token);
                while(token!=NULL) {
                    token = strtok(NULL," ");
                    if(!cnt) {
                        strcpy(ids, token);
                    }
                    cnt++;
                }
                ids[strlen(ids)-1] = '\0';

                if(strncmp(ids, arguments[1], sizeof(arguments[1])-1) == 0){
                    flag = 1;
                    break;
                }
                    
            }

            if(!flag) {
                fprintf(stderr, "%s is not in monitoring_list.txt\n", arguments[1]);
                continue;
            }
            fclose(file);

            // SIGUSR1 시그널을 해당 PID의 프로세스에 보냅니다.
            if (kill(killpid, SIGUSR1) == 0) {
                printf("monitoring ended (%s)\n", prepath);
            } else {
                perror("Failed to send SIGUSR1");
            }
            // monitor_list.txt에서 해당 PID를 제거합니다.
            remove_pid_from_file(killpid);
        }
        else if (strncmp(arguments[0], "exit", strlen("exit")) == 0) {
            if(idx>1) {
                printf("Usage : exit\n");
                continue;
            }
            exit(1);
        }
        else if (strncmp(arguments[0], "tree", strlen("tree")) == 0) {
            char absPath[PATH_MAX];
            char *path;
            char line[4096];
            char *pid_str;
            FILE *fp;
            int flag = 0;
            
            //디렉토리 입력 오류
            if(arguments[1] == NULL) {
                printf("Usage : tree <DIRPATH>\n");
                continue;
            }

            if(idx>2) {
                printf("Usage : tree <DIRPATH>\n");
                continue;
            }

            //존재하지 않거나 잘못된 경로의 디렉토리 예외처리
            if(realpath(arguments[1], absPath) == NULL){
                printf("%s is not in monitor_list.txt\n", arguments[1]);
                continue;
            }
            //monitor_list.txt에 존재하지 않는 경우 탐색
            if((fp= fopen("monitor_list.txt", "r"))==NULL){
                char current_path[PATH_MAX];
                char list_file[PATH_MAX];
                getcwd(current_path, sizeof(current_path));
                snprintf(list_file, PATH_MAX + 18, "%s/monitor_list.txt", current_path);
                if((fp = fopen(list_file, "w+")) == NULL) {
                    fprintf(stderr, "Fail to open monitor_list.txt\n");
                }
            }
            

            while(fgets(line, 4096, fp)) {
                path = strtok(line, " ");
                pid_str = strtok(line, " ");

                if(strcmp(path, absPath) == 0) {
                    flag = 1;
                    break;
                }
            }

            fclose(fp);

            if(flag == 0) {
                printf("%s is not in monitor_list.txt\n", arguments[1]);
                continue;
            }
            char tmp[PATH_MAX];
            strcpy(tmp, absPath);
            printf("%s\n", basename(tmp));
            ssu_tree(absPath, 0);
        }
        else {
            ssu_help();
        }
    }

    return 0;
}