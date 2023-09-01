#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <stdbool.h>
#include <dirent.h>
#include <getopt.h>

#define MAX_LENGTH 4096
#define FILE_LENGTH 1000

typedef struct FileNodes {
    char name[255];
    char path[MAX_LENGTH];
    struct FileNodes *next;
} FileNodes;

int compareFiles(char *backup_path, struct FileNodes **files);
void compareDir(char *backup_path, struct FileNodes **files);
void printUsage();
void add_files(FileNodes **head, char *name, char *path);
void removeFile(FileNodes **head);
void printNodes(FileNodes **head);
void removeFileNum(FileNodes **head, int option);
void removeFileDir(FileNodes **head);
int count_nodes(FileNodes *head);
void removeFileAll(FileNodes **head);
int isAccess(char *backup_path, char *backup_root);
void compareDirAll(char *backup_path, FileNodes **files);

//usage 출력 함수
void printUsage(){
    printf("Usage : remove <FILENAME> [OPTION]\n");
    printf(" -a : remove all file(recursive)\n");
    printf(" -c : clear backup directory\n");
}

//backup dir에 해당하는 file, dir이 존재하는지 검색
int compareFiles(char *backup_path, FileNodes **files){
    char dirPaths[FILE_LENGTH][MAX_LENGTH];
    char dir_path[MAX_LENGTH];
    char *fileName;
    struct dirent **namelist;
    bool flag = true;
    int n;
    int cnt = 0;

    strcpy(dir_path, backup_path);
    dirname(dir_path);
    fileName = basename(backup_path);

    if(access(dir_path, 0)==-1){
        return true;
    }

    //directory linkedlist를 이용해 탐색
    n = scandir(dir_path, &namelist, NULL, alphasort);

    for(int i=0; i<n; i++){
        struct dirent *entry = namelist[i];
        char compare_path[MAX_LENGTH];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(entry);
            continue;
        }

        if(strcmp(entry->d_name, "backup") == 0){
            continue;
        }

        snprintf(compare_path, sizeof(dir_path) + sizeof(entry->d_name), "%s/%s", dir_path, entry->d_name);

        if(entry->d_type == DT_DIR){
            if(strcmp(backup_path, compare_path) == 0){
                //dir의 경우 경루 복사
                flag = false;
                strcpy(dirPaths[cnt], compare_path);
                cnt++;
            }
        }
        else{
            //파일명 같은 파일이 존재할시 검사
            if(strncmp(fileName, entry->d_name, strlen(entry->d_name)-13)==0){
                flag = false;
                //동일 이름 파일 Linkedlist에 추가 관리
                add_files(files, entry->d_name,compare_path);
            }
        }

    }

    for(int i = 0; i<cnt; i++){
        //dir에 대하여 재귀적 탐색
        compareDir(dirPaths[i], files);
    }

    return flag;
}

//directory 탐색 함수
void compareDir(char *backup_path, FileNodes **files){
    char **dirPath = (char **)malloc(FILE_LENGTH * sizeof(char*));
    struct dirent **namelist;
    int n;
    int cnt = 0;

    for(int i = 0; i<FILE_LENGTH; i++){
        dirPath[i] = (char*)malloc(MAX_LENGTH * sizeof(char));
    }

    n = scandir(backup_path, &namelist, NULL, alphasort);

    if(n==-1){
        perror("scandir");
    }

    for(int i=0; i<n; i++){
        struct dirent *entry = namelist[i];
        char compare_path[MAX_LENGTH];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(entry);
            continue;
        }

        if(strcmp(entry->d_name, "backup") == 0){
            continue;
        }

        snprintf(compare_path, MAX_LENGTH-1, "%s/%s", backup_path, entry->d_name);

        if(entry->d_type == DT_DIR){
            strcpy(dirPath[cnt], compare_path);
            cnt++;
        }
        else{
            add_files(files, entry->d_name, compare_path);
        }
    }

    for(int i = 0; i<cnt; i++){
        compareDir(dirPath[i], files);
    }
}

//linkedlist 파일 추가 함수
void add_files(FileNodes **head, char *name, char *path) {
    struct FileNodes *new_file = malloc(sizeof(struct FileNodes));
    strncpy(new_file->name, name, sizeof(new_file->name));
    strncpy(new_file->path, path, sizeof(new_file->path));
    new_file->next = NULL;

    // 리스트의 끝에 추가
    if (*head == NULL) {
        *head = new_file;
    } else {
        struct FileNodes *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_file;
    }

    return ;
}

//링크드 리스트의 파일 삭제
void removeFile(FileNodes **head) {
    FileNodes *current = *head;
    while(current -> next != NULL){
        remove(current->path);
        printf("\"%s\" backup file removed\n", current->path);
        current = current ->next;
    }

    remove(current ->path);
    printf("\"%s\" backup file removed\n", current->path);

    return ;
}

//현제 링크드 리스트의 파일 출력
void printNodes(FileNodes **head) {
    FileNodes *current = *head;
    int i = 1;
    printf("0. exit\n");

    while(current -> next != NULL){
        char fileName[MAX_LENGTH];
        strcpy(fileName, current->path);
        char *fileTiem;

        fileTiem = strrchr(fileName, '_');

        if(fileTiem){
            fileTiem++;
        }

        FILE* fp = fopen(fileName, "rb");
        if(fp == NULL) {
            perror("fopen");
            return ;
        }

        fseek(fp, 0L, SEEK_END);  // 파일 끝으로 이동
        long size = ftell(fp);    // 파일 크기 계산
        fclose(fp);               // 파일 닫기


        printf("%d. %s   %ldbytes\n", i, fileTiem, size);
        current = current ->next;
        i++;
    }

    char fileName[MAX_LENGTH];
    strcpy(fileName, current->path);
    char *fileTiem;

    fileTiem = strrchr(fileName, '_');

    if(fileTiem){
        fileTiem++;
    }

    FILE* fp = fopen(fileName, "rb");
    if(fp == NULL) {
        perror("fopen");
        return ;
    }

    fseek(fp, 0L, SEEK_END);  // 파일 끝으로 이동
    long size = ftell(fp);    // 파일 크기 계산
    fclose(fp);               // 파일 닫기


    printf("%d. %s   %ldbytes\n", i, fileTiem, size);


    return;
}

//파일 선택 삭제
void removeFileNum(FileNodes **head, int option) {
    //linkelist에서 해당 파일 찾아서 삭제
    FileNodes *current = *head;
    while(current -> next != NULL && option > 1){
        current = current ->next;
        option--;
    }

    remove(current ->path);
    printf("\"%s\" backup file removed\n", current->path);

    return ;
}

void removeFileDir(FileNodes **head) {
    FileNodes *current = *head;
    while(current -> next != NULL){
        struct stat path_stat;

        if(stat(current->path, &path_stat) != 0){
            perror("stat");
            return;
        }

        //file인 경우
        if(!S_ISDIR(path_stat.st_mode)){
            remove(current->path);
            printf("\"%s\" backup file removed\n", current->path);
        }

        current = current ->next;
    }

    struct stat path_stat;

    if(stat(current->path, &path_stat) != 0){
        perror("stat");
        return;
    }

    //file인 경우
    if(!S_ISDIR(path_stat.st_mode)){
        remove(current->path);
        printf("\"%s\" backup file removed\n", current->path);
    }

    return ;
}

//c 옵션 사용시 해당하는 모든 파일 삭제
void removeFileAll(FileNodes **head) {
    FileNodes *current = *head;
    int cntFile = 0;
    int cntDir = 0;
    while(current -> next != NULL){
        struct stat path_stat;

        if(stat(current->path, &path_stat) != 0){
            perror("stat");
            return;
        }

        //file인 경우
        if(!S_ISDIR(path_stat.st_mode)){
            remove(current->path);
            cntFile++;
        }
        else{
            remove(current->path);
            cntDir++;
        }

        current = current ->next;
    }

    struct stat path_stat;

    if(stat(current->path, &path_stat) != 0){
        perror("stat");
        return;
    }

    //file인 경우
    if(!S_ISDIR(path_stat.st_mode)){
            remove(current->path);
            cntFile++;
        }
    else{
        remove(current->path);
        cntDir++;
    }

    printf("backup directory cleared(%d regular files and %d subdirectories totally).\n", cntFile, cntDir);

    return ;
}

//현제 linkedlist의 node 개수 확인
int count_nodes(FileNodes *head) {
    int count = 0;
    FileNodes *current = head;

    while (current != NULL) {
        count++;
        current = current->next;
    }

    return count;
}

//현제 dir에 찾는 file이나 dir이 존재하지 않는 경우 backupdir에서 해당 파일 검색
//file = 1, dir = 10, 존재하지 않는 경우 = 0
int isAccess(char *backup_path, char *backup_root){
    struct dirent **namelist;
    int n;
    char *username =getlogin();

    n = scandir(backup_root, &namelist, NULL, alphasort);

    if(n==-1){
        perror("scandir");
    }

    for(int i=0; i<n; i++){
        struct dirent *entry = namelist[i];
        char compare_path[MAX_LENGTH];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(entry);
            continue;
        }

        snprintf(compare_path, MAX_LENGTH-1, "%s/%s", backup_root, entry->d_name);

        //dir인 경우 재귀 탐색
        if(entry->d_type == DT_DIR){
            if(strcmp(backup_path, compare_path)==0){
                return 10;
            }

            if(isAccess(backup_path, compare_path) == 10){
                return 10;
            }

            if(isAccess(backup_path, compare_path) == 1){
                return 1;
            }
        }
        //file인 경우
        else{
            if(strncmp(backup_path, compare_path, strlen(compare_path)-13)==0){
                return 1;
            }
        }
    }
    return 0;
}

//전체 삭제시 시작 경로를 /home/username/backup부터 탐색하여 linkedlist 생성 함수
void compareDirAll(char *backup_path, FileNodes **files){
    char **dirPath = (char **)malloc(FILE_LENGTH * sizeof(char*));
    struct dirent **namelist;
    int n;
    int cnt = 0;

    for(int i = 0; i<FILE_LENGTH; i++){
        dirPath[i] = (char*)malloc(MAX_LENGTH * sizeof(char));
    }

    n = scandir(backup_path, &namelist, NULL, alphasort);

    if(n==-1){
        perror("scandir");
    }

    for(int i=0; i<n; i++){
        struct dirent *entry = namelist[i];
        char compare_path[MAX_LENGTH];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(entry);
            continue;
        }

        if(strcmp(entry->d_name, "backup") == 0){
            continue;
        }

        snprintf(compare_path, MAX_LENGTH-1, "%s/%s", backup_path, entry->d_name);

        if(entry->d_type == DT_DIR){
            strcpy(dirPath[cnt], compare_path);
            cnt++;
        }
        else{
            add_files(files, entry->d_name, compare_path);
        }
    }

    for(int i = 0; i<cnt; i++){
        compareDirAll(dirPath[i], files);
        add_files(files, basename(dirPath[i]), dirPath[i]);
    }
}