#include "ssu_monitor.h"

//링크드 리스트의 메모리를 해제하는 함수
void free_list(struct file_info **head) {
    struct file_info *current = *head;
    struct file_info *next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    *head = NULL;
}

//링크드리스트에 노드를 삽입하는 함수
void input_node(char *path, struct file_info **head, time_t mtime) {
    struct file_info * newNode = malloc(sizeof(struct file_info));
    strcpy(newNode->name, path);
    newNode->mtime = mtime;
    newNode->next = NULL;

    if(*head == NULL) {
        *head = newNode;
    }
    else {
        struct file_info *current = *head;
        while(current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

//디렉토리 내부를 탐색하는 함수
void scan_directory(const char *path, struct file_info **head){
    DIR *dir = opendir(path);
    struct dirent *entry;
    struct stat file_stat;
    char file_path[PATH_MAX];

    if (dir == NULL) {
        fprintf(stderr, "Cannot open directory %s\n", path);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if(strcmp(entry->d_name, "log.txt") == 0) 
            continue;

        snprintf(file_path, PATH_MAX, "%s/%s", path, entry->d_name);

        if(entry->d_type == DT_DIR) {
            scan_directory(file_path, head);
        }
        else{
            if (stat(file_path, &file_stat) == 0) {
                input_node(file_path, head, file_stat.st_mtime);
            }
        }
    }

    closedir(dir);
}

//이전과의 내용을 비교하고 변경사항을 log파일에 입력하는 함수
int compare(struct file_info **head, struct file_info **newhead, char *logPath) {
    struct file_info *cur = *head;
    struct file_info *cur2 = NULL;
    int result = 0;
    FILE *log;

    //log 파일 오픈
    if((log = fopen(logPath, "a")) == NULL) {
        fprintf(stderr, "log.txt open error\n");
        return 10;
    }
    
    //과거 디렉토리 구조를 현재 디렉토리 구조와 비교후 삭제와 변경사항 검사
    while(cur != NULL) {
        int flag = 0;
        cur2 = *newhead;
        while(cur2!=NULL) {
            if(strcmp(cur->name, cur2->name) == 0) {
                flag = 1;
                //변경 사항이 발생한 경우
                if(difftime(cur->mtime, cur2->mtime)!= 0) {
                    result = 1;
                    time_t t = cur2->mtime;
                    struct tm tm = *localtime(&t);
                    char time_str[20];
                    //현제 시간 작성
                    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);
                    fprintf(log, "[%s][modify][%s]\n", time_str, cur->name);
                }
                break;
            }
            cur2 = cur2->next;
        }
        //파일이 삭제된 경우
        if(!flag) {
            result = 1;
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char time_str[20];
            //현제 시간 작성
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);
            fprintf(log, "[%s][remove][%s]\n", time_str, cur->name);
        }
        cur  = cur->next;
    }

    //현재 디렉토리 구조와 과거 디렉토리 구조 비교 후 생성 사항 검사
    cur2 = *newhead;
    while(cur2 != NULL) {
        int flag = 0;
        cur = *head;
        while(cur!=NULL) {
            if(strcmp(cur->name, cur2->name) == 0) {
                flag = 1;
                break;
            }
            cur = cur->next;
        }
        //파일이 생성된 경우
        if(!flag) {
            result = 1;
            time_t t = cur2->mtime;
            struct tm tm = *localtime(&t);
            char time_str[20];
            //현제 시간 작성
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);
            fprintf(log, "[%s][create][%s]\n", time_str, cur2->name);
        }
        cur2  = cur2->next;
    }

    fclose(log);

    return result;
}

//delete 명령어 실행시 monitor_list.txt의 해당 pid 삭제 함수
void remove_pid_from_file(int pid) {
    char current_path[PATH_MAX];
    char list_file[PATH_MAX + 19];
    getcwd(current_path, sizeof(current_path));
    snprintf(list_file, PATH_MAX + 18, "%s/monitor_list.txt", current_path);

    FILE *file = fopen(list_file, "r");
    if (!file) {
        perror("Failed to open monitor_list.txt for reading");
        return;
    }

    // 파일의 내용을 메모리에 저장합니다.
    char **lines = NULL;
    size_t count = 0;
    char line[1024];
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        int current_pid;
        sscanf(line, "%*s %d", &current_pid);

        if (current_pid != pid) {
            lines = realloc(lines, (count + 1) * sizeof(*lines));
            lines[count] = strdup(line);
            count++;
        } else {
            found = 1;
        }
    }
    fclose(file);

    if (!found) {
        printf("PID %d not found in monitor_list.txt\n", pid);
    } else {
        // 원래 파일을 덮어쓰면서 입력된 PID와 일치하지 않는 줄만 씁니다.
        file = fopen("monitor_list.txt", "w");
        if (!file) {
            perror("Failed to open monitor_list.txt for writing");
            return;
        }

        for (size_t i = 0; i < count; i++) {
            fputs(lines[i], file);
            free(lines[i]);
        }

        fclose(file);
    }

    free(lines);
}

//add 명령어 실행시 실행 함수 -> 디렉토리를 모니터링함
void monitor_directory(const char *path, char *logPath, int interval) {
    char *current;
    char list_file[PATH_MAX];
    FILE *mlist;
    current = getcwd(NULL, PATH_MAX);
    snprintf(list_file, PATH_MAX, "%s/monitor_list.txt", current);

    mlist = fopen(list_file, "a");
    fprintf(mlist, "%s %d\n", path, getpid());
    fclose(mlist);


    int result = 0;
    struct file_info *head = NULL;
    struct file_info *newhead = NULL;

    //과거 파일 디렉토리 구조 링크드리스트 생성
    scan_directory(path, &head);

    while(1){
        sleep(interval);
        //현재 파일 디렉토리 구조 링크드리스트 생성
        scan_directory(path, &newhead);
        result = compare(&head, &newhead, logPath);
        free_list(&newhead);
        //변경사항 존재시 과거 파일 디렉토리 구조 초기화
        if(result == 1){
            free_list(&head);
            scan_directory(path, &head);
        }
        //에러 발생시 종료
        else if(result == 10) {
            break;
        }
    }
    
}

//트리 출력 함수
void ssu_tree(char *path, int space) {
        int n;
        struct dirent **namelist;
        n = scandir(path, &namelist, NULL, alphasort);

        for(int i=0; i<n; i++) {
            struct dirent *entry = namelist[i];
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
                free(entry);
                continue;
            }

            if(strcmp(entry->d_name, "log.txt") == 0){
                continue;
            }

            //디렉토리 구조에 맞는 공백
            for(int i=0; i<space; i++)
                printf(" ");

            printf("|--%s\n", entry->d_name);

            if(entry->d_type == DT_DIR) {
                char new_path[1024];
                snprintf(new_path, sizeof(new_path), "%s/%s", path, entry->d_name);
                ssu_tree(new_path, space+4);
            }
        }
}

//Usage 출력 함수
void ssu_help() {
    printf("Usage : add <DIRPATH> [OPTION]\n");
    printf("Usage : delete <DAEMON_PID>\n");
    printf("Usage : tree <DIRPATH>\n");
    printf("Usage : help\n");
    printf("Usage : exit\n");
}

//monitor_list.txt에 존재하는 디렉토리의 하위 디렉토리와 입력받은 경로 비교 함수
int checkPath (char *path, char *cmp) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    char file_path[PATH_MAX];
    
    if(dir == NULL) {
        fprintf(stderr, "Cannot open directory %s\n", path);
        return 1;
    }

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..")==0) {
            continue;
        }
        snprintf(file_path, PATH_MAX, "%s/%s", path, entry->d_name);

        if(entry->d_type==DT_DIR){
            if(strcmp(cmp, file_path) == 0) return 1;
            checkPath(file_path, cmp);
        }
    }
}

//입력받은 옵션의 값이 정수인지 확인한는 함수
int checkDigit(char *path) {
    int cnt = 0;
    while(path[cnt] != '\0') {
        if(!isdigit(path[cnt])) {
            return 1;
        }
        cnt++;
    }
    return 0;
}
