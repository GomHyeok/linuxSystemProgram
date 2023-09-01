 #include "remove.h"


int main(int argc, char* argv[]){
    char buf[15];
    char *username;
    char *check_path;
    char** copyfile = (char**)malloc(sizeof(char*) * argc);
    char real_path[MAX_LENGTH];
    char backup_path[MAX_LENGTH]; 
    char backupRoot[MAX_LENGTH];
    struct stat filestat;
    struct FileNodes *head = NULL;
    int opt;
    int option_a_position = -1;
    int option_c_position = -1;

    for (int i = 0; i < argc; i++) {
        copyfile[i] = argv[i];
    }

    struct option long_options[] = {
        {"all", no_argument, NULL, 'a'},
        {"clearbackup", required_argument, NULL, 'c'} ,
        {0,0,0,0}
    };

    while((opt = getopt_long(argc, argv, "ac", long_options, NULL)) != -1){
        switch(opt){
            case 'a' :
                option_a_position = optind-1;
                break;

            case 'c' :
                option_c_position = optind-1;
                break;

            case '?' :
                printUsage();
                exit(1);
        }
    }

    username = getlogin();

    sprintf(backupRoot, "/home/%s/backup", username);

    if(argc<3 || argc>4){//파일명 없거나 옵션이 과하게 설정된 경우
        printUsage();
        exit(1);
    }
    
    if(strlen(copyfile[1]) > MAX_LENGTH){ //파일명의 크기가 255바이트를 초과하는 경우
        printf("\"%s\" can't be backuped\n", copyfile[1]);
        exit(1);
    }

    if(option_a_position > 0 && option_a_position != 2 && argc > 23){//-a옵션의 위치가 잘못 설정된 경우
        printUsage();
        exit(1);
    }

    if(option_c_position > 0 && option_c_position != 1 && argc > 2){//-c옵션의 사용이 잘못된 경우
        printUsage();
        exit(1);
    }

    if(option_c_position > 0){//option c 사용시
        if(argc != 3){
            printUsage();
            exit(1);
        }
        snprintf(backup_path, MAX_LENGTH, "/home/%s/backup", username);
        compareDirAll(backup_path, &head);

        if(count_nodes(head) == 0){//backupdir이 비워져 있는 경우
            printf("no file(s) in the backup\n");
            exit(1);
        }

        removeFileAll(&head);
        exit(0);
    }

    else{
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

        if(strstr(real_path, "/backup") != NULL){
            printf("\"%s\" can't be backuped\n", real_path);
            exit(1);
        }

        char *p = strstr(real_path, username);
        int pos = p-real_path + strlen(username);

        //backup 경로 생성
        if(snprintf(backup_path, MAX_LENGTH, "%.*s/backup%s", pos, real_path, real_path+pos)>MAX_LENGTH){
            printf("\"%s/%s\" can't be backuped\n", real_path, basename(copyfile[1]));
            exit(1);
        }

        if(compareFiles(backup_path, &head)){//backup 디렉토리에 동일한 파일 또는 디렉토리가 있는지 검증
            printUsage();
            exit(1);
        }
    }
    
    if(access(real_path, 0) == 0){//작업 dir에 파일 또는 dir이 존재하는 경우
        int result = stat(real_path, &filestat); //file과 dir 구분

        if(result == -1){
            printf("stat error\n");
            exit(1);
        }
        else if(S_ISDIR(filestat.st_mode)){//directoy case


            if(copyfile[2]==NULL || strcmp(copyfile[2], "-a") != 0){// 디렉토리일 때 -a 옵션이 없는 경우
                
                if(copyfile[2]!=NULL && strcmp(copyfile[2], "-c") == 0){
                    printUsage();
                    return 0;
                }
                printf("\"%s\" is a diretory file\n", real_path);
                exit(1);
            }
            
            removeFileDir(&head);
            
        }
        else{ //file case
            long long option;
            
            if((argc == 4 && strcmp(copyfile[2], "-a")==0) || count_nodes(head) == 1){//a옵션 사용하는 경우
                removeFile(&head);
            }
            else if(argc == 4 && strcmp(copyfile[2], "-c")==0){//-c가 동시에 있는 경우
                printUsage();
                return 0;
            }
            else{
                printf("backup file list of \"%s\"\n", real_path);
                printNodes(&head);
                printf("Choose file to remove\n");
                printf(">> ");
                scanf("%lld", &option);

                if(option == 0){
                    exit(EXIT_SUCCESS);
                }
                removeFileNum(&head, option);
            }
        }
    }
    //현제 dir에 file이나 dir이 존재하지 않는 경우 -> 파일인 경우
     else if(isAccess(backup_path, backupRoot) == 1){
        long long option;
            
        if((argc == 4 && strcmp(copyfile[2], "-a")==0) || count_nodes(head) == 1){
            removeFile(&head);
        }
        else if(argc == 4 && strcmp(copyfile[2], "-c")==0){
            printUsage();
            exit(1);
        }
        else{
            printf("backup file list of \"%s\"\n", real_path);
            printNodes(&head);
            printf("Choose file to remove\n");
            printf(">> ");
            scanf("%lld", &option);

            if(option == 0){
                exit(EXIT_SUCCESS);
            }
            removeFileNum(&head, option);
        }
    }
    //dir인 경우
    else if(isAccess(backup_path, backupRoot) == 10){
        if(copyfile[2]==NULL || strcmp(copyfile[2], "-a") != 0){// 디렉토리일 때 -a 옵션이 없는 경우
                
            if(copyfile[2]!=NULL && strcmp(copyfile[2], "-c") == 0){
                printUsage();
                return 0;
            }
            printf("\"%s\" is a diretory file\n", real_path);
            exit(1);
        }
        
        removeFileDir(&head);
    }
    //backup dir에도 존재하지 않는 경우
    else{
        printf("\"%s\" is'n in backup dir\n", real_path);
        exit(1);
    }

    return 0;
}