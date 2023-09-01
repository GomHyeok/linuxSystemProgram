#include <stdio.h>
#include <stdlib.h>
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
int compareFiles(char *backup_path, char *real_path, FileNodes **files, char *way);
void compareDir(char *backup_path, char *real_path, char *way);
void printNodes(char *real_path, FileNodes **head);
int compareFilesMD5(char* filePath1, char* filePath2);
int compareFilesSHA(char* filePath1, char* filePath2);
void add_files(FileNodes **head, char *name, char *path, char *way, char *real_path);
int count_nodes(FileNodes *head);
void makeDir(char *path);
void recoverFileNum(FileNodes **head, int option, char *real_path);
void recover(char *src_path, char *dst_path);
int isAccess(char *backup_path, char *backup_root);

//usage 출력 함수
void printUsage(){
    printf("Usage : recover <FILENAME> [OPTION]\n");
    printf(" -d : recover directory recursive\n");
    printf(" -n <NEWNAME> : recover file with new name\n");
}

//backup dir에 일치하는 file, dir존재하는지 검색
int compareFiles(char *backup_path, char *real_path, FileNodes **files, char *way){
    char dir_path[MAX_LENGTH];
    char *fileName;
    struct dirent **namelist;
    bool flag = true;
    int n;

    strcpy(dir_path, backup_path);
    dirname(dir_path);
    fileName = basename(backup_path);

    if(access(dir_path, 0)==-1){
        return true;
    }

    //디렉토리 재귀적 탐색 및 링크드 리스트 생성
    n = scandir(dir_path, &namelist, NULL, alphasort);


    for(int i=0; i<n; i++){
        struct dirent *entry = namelist[i];
        char compare_path[MAX_LENGTH];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(entry);
            continue;
        }

        snprintf(compare_path,sizeof(dir_path) + sizeof(entry->d_name), "%s/%s", dir_path, entry->d_name);

        //동일한 dir 존재시 재귀탐색
        if(entry->d_type == DT_DIR){
            if(strcmp(backup_path, compare_path) == 0){
                flag = false;
                compareDir(compare_path, real_path, way);
            }
        }
        else{
            //파일명 같은 파일이 존재할시 검사
            if(strncmp(fileName, entry->d_name, strlen(entry->d_name)-13)==0){

                flag = false;
                add_files(files, entry->d_name, compare_path, way, real_path);
            }
        }

    }

    return flag;
}

//디렉토리 내부의 file recover 함수
void compareDir(char *backup_path, char *real_path, char *way){
    struct dirent **namelist;
    struct dirent *tmp;
    char past[255];
    char past_s[255];
    int n;
    int cnt = 0;
    int fileCnt = 0;
    FileNodes *head = NULL;

    //디렉토리 재귀적 탐색 및 링크드 리스트 생성
    n = scandir(backup_path, &namelist, NULL, alphasort);

    if(n==-1){
        perror("scandir");
    }

    int dirCnt = 0;
    int maxDirs = FILE_LENGTH;
    char** dirPath = (char**)malloc(maxDirs * sizeof(char*));
    char** realDirPath = (char**)malloc(maxDirs * sizeof(char*));
    
    for(int i = 0; i < maxDirs; i++) {
        dirPath[i] = (char*)malloc(MAX_LENGTH * sizeof(char));
        realDirPath[i] = (char*)malloc(MAX_LENGTH * sizeof(char));
    }

    for(int i=0; i<n; i++){
        struct dirent *entry = namelist[i];
        char compare_path[MAX_LENGTH];
        char copy_real_path[MAX_LENGTH];
        char *ptr;
        char copy_name[255];

        strcpy(copy_real_path, real_path);
        strcpy(copy_name, entry->d_name);

        ptr = strtok(copy_name, "_");
        strcat(copy_real_path, "/");
        strcat(copy_real_path, copy_name);

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(entry);
            continue;
        }

        snprintf(compare_path, MAX_LENGTH-1, "%s/%s", backup_path, entry->d_name);

        //dir의 경우 백업 경로와 실제 경로 복사 후 재귀탐색
        if(entry->d_type == DT_DIR){
            strcpy(dirPath[dirCnt], compare_path);
            strcpy(realDirPath[dirCnt], copy_real_path);
            dirCnt++;
        }
        //file의 경우 linkelist 사용해 recover
        else{
            fileCnt++;
            //처음 탐색인 경우 past 초기화
            if(cnt == 0){
                strcpy(past_s, entry->d_name);
                strtok(past_s, "_");
                strcpy(past, past_s);
                cnt++;
            }

            //이전 file의 이름과 이름이 다른경우
            if(strcmp(past, copy_name) != 0){
                //이전 file이 한개인 경우
                if(count_nodes(head) == 1){
                    dirname(copy_real_path);
                    strcat(copy_real_path, "/");
                    strcat(copy_real_path, past);

                    recover(head -> path, copy_real_path);
                }
                //이전 file이 한개도 없는 경우
                else if(count_nodes(head) == 0){
                    dirname(copy_real_path);
                    strcat(copy_real_path, "/");
                    strcat(copy_real_path, past);
                    printf("\"%s\" is same file\n", copy_real_path);
                }
                //이전 file이 여러개인 경우
                else{
                    long long option;
                    dirname(copy_real_path);
                    strcat(copy_real_path, "/");
                    strcat(copy_real_path, past);
                    printNodes(copy_real_path, &head);
                    printf("Choose file to recover\n");
                    printf(">> ");
                    scanf("%lld", &option);

                    if(option == 0){
                        exit(EXIT_SUCCESS);
                    }

                    dirname(copy_real_path);
                    strcat(copy_real_path, "/");
                    strcat(copy_real_path, past);

                    recoverFileNum(&head, option, copy_real_path);
                }
                //head초기화
                strcpy(past_s,entry->d_name);
                strcpy(past, copy_name);
                head = NULL;
            }

            dirname(copy_real_path);
            strcat(copy_real_path, "/");
            strcat(copy_real_path, past);

            //linklist에 node추가
            add_files(&head, entry->d_name, compare_path, way, copy_real_path);
        }
    }


    //linkelist에 남은 file이 존재하는 경우
    if(fileCnt > 0){
        char compare_path[MAX_LENGTH];
        char copy_real_path[MAX_LENGTH];

        snprintf(compare_path, MAX_LENGTH-1, "%s/%s", backup_path, past_s);
        strcpy(copy_real_path, real_path);

        if(count_nodes(head) == 1){
            strcat(copy_real_path, "/");
            strcat(copy_real_path, past);
            recover(head->path, copy_real_path);
        }
        else if(count_nodes(head) == 0){
            dirname(copy_real_path);
            strcat(copy_real_path, "/");
            strcat(copy_real_path, past);
            printf("\"%s\" is same file\n", copy_real_path);
        }
        else if(count_nodes(head) > 0){
            long long option;
            dirname(copy_real_path);
            strcat(copy_real_path, "/");
            strcat(copy_real_path, past);
            printNodes(copy_real_path, &head);
            printf("Choose file to recover\n");
            printf(">> ");
            scanf("%lld", &option);

            if(option == 0){
                exit(EXIT_SUCCESS);
            }

            strcat(copy_real_path, "/");
            strcat(copy_real_path, past);

            recoverFileNum(&head, option, copy_real_path);
        }
        head = NULL;
    }

    // Dir만 재귀 탐색
    for(int i=0; i<dirCnt; i++){
        compareDir(dirPath[i], realDirPath[i], way);
    }
    
}

//linkedlist에 node 추가 함수
void add_files(FileNodes **head, char *name, char *path, char *way, char *real_path) {
    struct FileNodes *new_file = malloc(sizeof(struct FileNodes));
    strncpy(new_file->name, name, sizeof(new_file->name));
    strncpy(new_file->path, path, sizeof(new_file->path));

    int result;
    new_file->next = NULL;

    //node추가시 현재 file과 중복되는 경우 추가하지 않음
    if(strcmp(way, "sha1") == 0){
        result = compareFilesSHA(real_path, path);

        if(result == 0){
            return;
        }

    }
    else{
        result = compareFilesMD5(real_path, path);

        if(result == 0){
            return;
        }

    }

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



//linkedlist의 노드 출력
void printNodes(char *real_path, FileNodes **head) {
    FileNodes *current = *head;
    int i = 1;
    printf("backup file list of \"%s\" \n", real_path);
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


//linkedlist의 노트 개수 확인
int count_nodes(FileNodes *head) {
    int count = 0;
    FileNodes *current = head;

    while (current != NULL) {
        count++;
        current = current->next;
    }

    return count;
}

//backup 경로에 존재하지 않는 dir 생성
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

//파일 recover 함수 -> file open후 덮어쓰기
void recover(char *src_path, char *dst_path){
    int src_fd;
    int dst_fd;
    char tmp_path[MAX_LENGTH];
    
    strcpy(tmp_path, dst_path);
    
    src_fd = open(src_path, O_RDONLY);

    if(src_fd == -1){
        perror("open error");
        return ;
    }

    dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if(dst_fd == -1){

        if(errno == ENOENT){
            char *dir_path = dirname(tmp_path);
            makeDir(dir_path);

            dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if(dst_fd == -1){
                perror("open error");
                return;
            }
        }
    }

    char buf[1];
    while(read(src_fd, buf, sizeof(buf))>0){
        write(dst_fd, buf, 1);
    }

    close(src_fd);
    close(dst_fd);

    printf("\"%s\" backup recover to \"%s\"\n", src_path, dst_path);
}

//file이 여러개인 경우 해당 file recover 함수
void recoverFileNum(FileNodes **head, int option, char *real_path){
    FileNodes *current = *head;
    while(current -> next != NULL && option > 1){
        current = current ->next;
        option--;
    }

    recover(current ->path, real_path);
    return ;
}


//현제 dir에 찾는 file이나 dir이 존재하지 않는 경우 backupdir에서 해당 파일 검색
//file = 1, dir = 10, 존재하지 않는 경우 = 0
int isAccess(char *backup_path, char *backup_root){
    struct dirent **namelist;
    int n;
    char *username =getlogin();

    //디렉토리 재귀적 탐색 및 링크드 리스트 생성
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

        if(entry->d_type == DT_DIR){
            int result;
            if(strcmp(backup_path, compare_path)==0){
                return 10;
            }

            if((result = isAccess(backup_path, compare_path)) > 0){
                return result;
            }
        }
        else{
            if(strncmp(backup_path, compare_path, strlen(compare_path)-13)==0){
                return 1;
            }
        }
    }
    return 0;
}