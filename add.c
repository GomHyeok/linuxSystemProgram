#include "add.h"

int main(int argc, char* argv[]){
    char buf[15];
    char backupPath[MAX_LENGTH];
    char** copyfile = (char**)malloc(sizeof(char*) * argc);
    int opt;
    char *username;
    char *check_path;
    char real_path[MAX_LENGTH]; //backup file 또는 dir의 경로
    char backup_path[MAX_LENGTH];  //backup dir 저장 경로
    time_t now = time(NULL); //현제 시간 탐색
    struct tm *t = localtime(&now);
    struct stat filestat;
    FileNodes *head;

    sprintf(buf, "%02d%02d%02d%02d%02d%02d", t->tm_year + 1900-2000, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);  //현제 시간 int to string

    username = getlogin();

    sprintf(backupPath, "/home/%s/backup", username);

    makeLinkedList(&head, backupPath);

    for (int i = 0; i < argc; i++) {
        copyfile[i] = argv[i];
    }



    int isOptionD=0;
    int option_d_position = -1;
    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"optionD", no_argument, 0, 'd'},
        {0, 0, 0, 0}
    };
    while ((opt = getopt_long(argc, argv, "hd", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                option_d_position = optind - 1;
                break;
            case 'h':
                printUsage();
                exit(0);
            case '?':
                printUsage();
                exit(1);
        }
    }

    if ((option_d_position==-1) && argc!=3) { //optionD가 없으면 무조건 argc == 3여야함
        printUsage();
        exit(1);
    }
    if ((option_d_position!=-1) && argc!=4) { //optionD가 들어왔으면 무조건 argc == 4여야함 
        printUsage();
        exit(1);
    }
    if ((option_d_position!=-1) && option_d_position!=2) { //optionD가 들어왔으면 optionD가 filename 다음에 나오지 않는 경우
        printUsage();
        exit(1);
    }

    if(argc < 3 || argc > 4){ //입련된 파일명이 없거나 옵션이 과하게 설정된 경우
        printUsage();
        exit(1);
    }

    if(strlen(copyfile[1]) > MAX_LENGTH ){ //첫 번째 인자로 입력받은 경로(절대경로)가 길이 제한을 넘는경우
        printf("RealPath is wrong\n");
        exit(1);
    }
    else {
        check_path = realpath(copyfile[1], NULL);

        if (check_path == NULL) { //실제 경로 생성
            realpath(copyfile[1], real_path);
        } 
        else {
            realpath(copyfile[1], real_path);
        }

        char home_dir[MAX_LENGTH];
        strcpy(home_dir,"/home/");
        strcat(home_dir,username);
        
        if(strncmp(real_path, home_dir, strlen(home_dir)) != 0){//home에서 벗어나는 경우
            printf("\"%s\" can't be backuped\n", real_path);
            exit(1);
        }

        if(strstr(real_path, "/backup") != NULL){//backup 디렉토리를 포함한 경우
            printf("\"%s\" can't be backuped\n", real_path);
            exit(1);
        }

        if(access(real_path,0)==-1){//파일이나 디렉토리가 존재하지 않는 경우
            printUsage();
            exit(1);
        }
        
        if(access(real_path, F_OK) == -1){ //접근권한이 없거나 정상적이지 않은 경우
            printf("\"%s\" can't be backuped\n", real_path);
            exit(1);
        }

        char *p = strstr(real_path, username);
        int pos = p-real_path + strlen(username);

        //backup 경로 생성
        snprintf(backup_path, MAX_LENGTH, "%.*s/backup%s_%s", pos, real_path, real_path+pos, buf);//backup 경로 구함

        int result = stat(real_path, &filestat); //file과 dir 구분
        if(result == -1){
            printf("\"%s\" can't be backuped\n", real_path);
        }
        else if(S_ISDIR(filestat.st_mode)){//directoy case
            if(copyfile[2]==NULL || strcmp(copyfile[2], "-d") != 0){// 디렉토리일 때 -d 옵션이 없는 경우
                printf("\"%s\" is a diretory file\n", real_path);
                exit(1);
            }

            scanDir(real_path, buf, copyfile[3], head);
            
        }
        else{ //file case
            if(compareFiles(real_path, backup_path, copyfile[2])){
                copyFile(real_path, backup_path, head);
            }
        }

    }
}
