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
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <getopt.h>
#include <openssl/evp.h>

#define MAX_LENGTH 4096
#define FILE_LENGTH 1000

typedef struct FileNodes {
    char name[255];
    char path[MAX_LENGTH];
    struct FileNodes *next;
} FileNodes;

void printUsage();
void scanDir(char *real_path, char *buf, char *kind, FileNodes *head);
void copyFile(char *real_path, char *backup_path, FileNodes *head);
void makeDir(char *path);
int compareFilesMD5(char* filePath1, char* filePath2);
int compareFilesSHA(char* filePath1, char* filePath2);
bool compareFiles(char *real_path, char *backup_path, char *kind);
void add_files(FileNodes **head, char *name, char *path);
void makeLinkedList(FileNodes **head, char *bakcupPath);

//usage출력 함수
void printUsage(){
    printf("Usage : add <FILENAME> [OPTION]\n");
    printf(" -d : add directory recursive\n");
}

//file copy목적 함수
void copyFile(char *real_path, char *backup_path, FileNodes *head){
    //복사하기 위한 파일 오픈
    int real_path_fd = open(real_path, O_RDONLY);
    char tmp_path[MAX_LENGTH];

    strcpy(tmp_path, backup_path);

    if(real_path_fd == -1){
        perror("open");
        return ;
    }

    //backupfile 오픈
    int dest_fd = open(backup_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    //backupfile이 존재하지 않는 경우
    if(dest_fd == -1){

        if(errno == ENOENT){
            char *dir_path = dirname(tmp_path);
            //경로상의 dir 생성
            makeDir(dir_path);
            //재오픈
            dest_fd = open(backup_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if(dest_fd == -1){
                perror("open");
                return;
            }
        }

    }

    char buf[1];
    while(read(real_path_fd, buf, sizeof(buf))>0){//file의 복사
        write(dest_fd, buf, 1);
    }

    //linked list에 파일 추가
    add_files(&head, basename(backup_path), backup_path);

    close(real_path_fd);
    close(dest_fd);

    printf("\"%s\" backuped\n", backup_path);
    return;
}

//directory 탐색 과 디렉토리 내부 파일 추가 함수
void scanDir(char *real_path, char *buf, char *kind, FileNodes *head){
    if(strcmp(basename(real_path), "backup")==0){
        return ;
    }
    
    //directory를 읽기 위한 링크드 리스트 생성
    struct dirent **namelist;
    int n;
    int cnt = 0;
    char **dir_path = (char**)malloc(FILE_LENGTH * sizeof(char*));

    for(int i=0; i<FILE_LENGTH; i++){
        dir_path[i] = (char *)malloc(MAX_LENGTH * sizeof(char));
    }

    //directoty내부의 파일 및 directory 개수 탐색 + namelist 2중 linkedlist 에 저장
    n = scandir(real_path, &namelist, NULL, alphasort);

    if(n<0) {
        perror("scandir");
        return ;
    }

    for(int i=0; i<n; i++){
        //directory내부 탐색
        struct dirent *entry = namelist[i];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(entry);
            continue;
        }

        if(strcmp(entry->d_name, "backup") == 0){
            continue;
        }

        //해당 파일or디렉토리 절대경로 생성
        char entry_path[MAX_LENGTH];
        sprintf(entry_path, "%s/%s", real_path, entry->d_name);

        if(entry->d_type != DT_DIR){//file인 경우
            char *username = getlogin();
            char *p = strstr(entry_path, username);
            int pos = p-entry_path + strlen(username);
            char backup_path[MAX_LENGTH];

            snprintf(backup_path, MAX_LENGTH, "%.*s/backup%s_%s", pos, entry_path, entry_path+pos, buf);

            //중복되지 않는다면 backup
            if(compareFiles(entry_path, backup_path, kind)){
                copyFile(entry_path, backup_path, head);
            }
        }
        else{//dir인 경우 dir_path에 경로 추가 ->파일 우선 탐색
            strcpy(dir_path[cnt], entry_path);
            cnt++;
        }
    }

    for(int i=0; i<cnt; i++){
        scanDir(dir_path[i], buf, kind, head);
    }

    return;
}

//directory 생성 함수
void makeDir(char *path){
    char *ptr;
    char backup_path[MAX_LENGTH]={0};

    ptr = strtok(path,"/");

    while(ptr!=NULL){
        //path상의 directory 확인
        strcat(backup_path,"/");
        strcat(backup_path, ptr);
        ptr = strtok(NULL, "/");

        //directory 존재하지 않을 시 생성
        if(access(backup_path, 0)==- 1){
            if(mkdir(backup_path,  0777) == -1){
                perror("mkdir");
                return ;
            }
        }

    }
}

//해시함수 MD5
int compareFilesMD5(char* filePath1, char* filePath2) {
    FILE* file1 = fopen(filePath1, "r");
    if (file1 == NULL) {
        return -1;
    }

    FILE* file2 = fopen(filePath2, "r");
    if (file2 == NULL) {
        fclose(file1);
        return -1;
    }

    EVP_MD_CTX *ctx1 = EVP_MD_CTX_new();
    EVP_MD_CTX *ctx2 = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx1, EVP_md5(), NULL);
    EVP_DigestInit_ex(ctx2, EVP_md5(), NULL);

    char buffer1[1024];
    char buffer2[1024];
    size_t bytesRead1, bytesRead2;

    do {
        bytesRead1 = fread(buffer1, 1, sizeof(buffer1), file1);
        bytesRead2 = fread(buffer2, 1, sizeof(buffer2), file2);
        if (bytesRead1 != bytesRead2 || memcmp(buffer1, buffer2, bytesRead1)) {
            fclose(file1);
            fclose(file2);
            return 1; // 파일 내용이 다르면 1 반환
        }
        EVP_DigestUpdate(ctx1, buffer1, bytesRead1);
        EVP_DigestUpdate(ctx2, buffer2, bytesRead2);
    } while (bytesRead1 > 0);

    unsigned char hash1[EVP_MAX_MD_SIZE];
    unsigned char hash2[EVP_MAX_MD_SIZE];
    unsigned int hash1_len, hash2_len;
    EVP_DigestFinal_ex(ctx1, hash1, &hash1_len);
    EVP_DigestFinal_ex(ctx2, hash2, &hash2_len);

    if (hash1_len != hash2_len || memcmp(hash1, hash2, hash1_len)) {
        fclose(file1);
        fclose(file2);
        return 1; // 해시값이 다르면 1 반환
    }

    fclose(file1);
    fclose(file2);
    return 0; // 파일 내용과 해시값이 모두 같으면 0 반환
}

//해시함수 SHA1
int compareFilesSHA(char* filePath1, char* filePath2) {
    FILE* file1 = fopen(filePath1, "r");
    if (file1 == NULL) {
        return -1;
    }

    FILE* file2 = fopen(filePath2, "r");
    if (file2 == NULL) {
        fclose(file1);
        return -1;
    }

    EVP_MD_CTX *ctx1 = EVP_MD_CTX_new();
    EVP_MD_CTX *ctx2 = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx1, EVP_sha256(), NULL);
    EVP_DigestInit_ex(ctx2, EVP_sha256(), NULL);

    char buffer1[1024];
    char buffer2[1024];
    size_t bytesRead1, bytesRead2;

    do {
        bytesRead1 = fread(buffer1, 1, sizeof(buffer1), file1);
        bytesRead2 = fread(buffer2, 1, sizeof(buffer2), file2);
        if (bytesRead1 != bytesRead2 || memcmp(buffer1, buffer2, bytesRead1)) {
            fclose(file1);
            fclose(file2);
            EVP_MD_CTX_free(ctx1);
            EVP_MD_CTX_free(ctx2);
            return 1; // 파일 내용이 다르면 1 반환
        }
        EVP_DigestUpdate(ctx1, buffer1, bytesRead1);
        EVP_DigestUpdate(ctx2, buffer2, bytesRead2);
    } while (bytesRead1 > 0);

    unsigned char hash1[EVP_MAX_MD_SIZE];
    unsigned char hash2[EVP_MAX_MD_SIZE];
    unsigned int hashLen1, hashLen2;
    EVP_DigestFinal_ex(ctx1, hash1, &hashLen1);
    EVP_DigestFinal_ex(ctx2, hash2, &hashLen2);

    if (hashLen1 != hashLen2 || memcmp(hash1, hash2, hashLen1)) {
        fclose(file1);
        fclose(file2);
        EVP_MD_CTX_free(ctx1);
        EVP_MD_CTX_free(ctx2);
        return 1; // 해시값이 다르면 1 반환
    }

    fclose(file1);
    fclose(file2);
    EVP_MD_CTX_free(ctx1);
    EVP_MD_CTX_free(ctx2);
    return 0; // 파일 내용과 해시값이 모두 같으면 0 반환
}

//file의 중복 여부 확인을 위한 함수
bool compareFiles(char *real_path, char *backup_path, char *kind){
    char dir_path[MAX_LENGTH];
    char *fileName;
    struct dirent **namelist;
    int n;

    strcpy(dir_path, backup_path);
    dirname(dir_path);
    fileName = basename(backup_path);

    //상위 directory 존재하지 않을 시 중복되지 않음
    if(access(dir_path,0)==-1){
        return true;
    }

    n = scandir(dir_path, &namelist, NULL, alphasort);

    //backup directory의 탐색
    for(int i=0; i<n; i++){
        struct dirent *entry = namelist[i];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(entry);
            continue;
        }

        if(strcmp(entry->d_name, "backup") == 0){
            continue;
        }

        if(entry->d_type == DT_DIR){
            continue;
        }
        else{
            //파일명 같은 파일이 존재할시 검사
            if(strncmp(fileName, entry->d_name, strlen(fileName)-13)==0){
                char compare_path[MAX_LENGTH];
                snprintf(compare_path, sizeof(dir_path) + sizeof(entry->d_name), "%s/%s", dir_path, entry->d_name);

                if(strcmp(kind, "md5") == 0){
                    int result = compareFilesMD5(real_path, compare_path);
                    if(result == 0){
                        printf("\"%s\" is already backuped\n", compare_path);
                        return false;
                    }
                }
                else{
                    int result = compareFilesSHA(real_path, compare_path);
                    if(result == 0){
                        printf("\"%s\" is already backuped\n", compare_path);
                        return false;
                    }
                }

            }
        }
    }

    return true;
}

//linkedlist에 노드 추가를 위한 함수
void add_files(FileNodes **head, char *name, char *path) {
    struct FileNodes *new_file = malloc(sizeof(struct FileNodes));
    strncpy(new_file->name, name, sizeof(new_file->name));
    strncpy(new_file->path, path, sizeof(new_file->path));

    int result;
    new_file->next = NULL;

    // 리스트의 끝에 추가
    if (*head == NULL) {
        *head = new_file;
    } 
    else {
        struct FileNodes *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_file;
    }

    return ;
}

//add 실행시 관리한 linkedlist 생성 함수
void makeLinkedList(FileNodes **head, char *backupPath){
    char **dir_path = (char**)malloc(FILE_LENGTH*sizeof(char*));
    int n;
    int cnt = 0;
    struct dirent **namelist;

    for(int i=0; i<FILE_LENGTH; i++){
        dir_path[i] = (char*)malloc(MAX_LENGTH*sizeof(char));
    }

    n = scandir(backupPath, &namelist, NULL, alphasort);

    if(n<0) {
        perror("scandir");
        return ;
    }

    for(int i=0; i<n; i++){
        //directory내부 탐색
        struct dirent *entry = namelist[i];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(entry);
            continue;
        }

        if(strcmp(entry->d_name, "backup") == 0){
            continue;
        }

        //해당 파일or디렉토리 절대경로 생성
        char entry_path[MAX_LENGTH];
        sprintf(entry_path, "%s/%s", backupPath, entry->d_name);

        if(entry->d_type != DT_DIR){//file인 경우
            add_files(head, entry->d_name, entry_path);
        }
        else{//dir인 경우
            strcpy(dir_path[cnt], entry_path);
            cnt++;
        }
    }

    for(int i=0; i<cnt; i++){
        makeLinkedList(head, dir_path[i]);
    }

    return;

}