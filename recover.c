#include "recover.h"

int main(int argc, char* argv[]) {
    char *username;
    struct FileNodes *head;
    char *check_path;
    char *new_check_path;
    char backup_root[MAX_LENGTH];
    char** copyfile = (char**)malloc(sizeof(char*) * argc);  
    char real_path[MAX_LENGTH];
    char new_path[MAX_LENGTH];
    char backup_path[MAX_LENGTH];
    struct stat filestat;
    int unreal = 0;
    int opt;
    int option_d_position = -1;
    int option_n_position = -1;

    for (int i = 0; i < argc; i++) {
        copyfile[i] = argv[i];
    }

    struct option long_options[] = {
        {"directorys", no_argument, NULL, 'd'},
        {"naming", required_argument, NULL, 'n'} ,
        {0,0,0,0}
    };

    username = getlogin();

    sprintf(backup_root, "/home/%s/backup", username);

    while((opt = getopt_long(argc, argv, "dn:", long_options, NULL)) != -1){
        switch(opt){
            case 'd' :
                option_d_position = optind-1;
                break;

            case 'n' :
                option_n_position = optind-1;
                break;

            case '?' :
                printUsage();
                exit(0);
        }
    }

    if(argc > 6 || argc < 3){//인자 개수의 예외 경우
        printUsage();
        exit(0);
    }

    if(argc > 3 && strcmp(copyfile[2], "-d") != 0 && strcmp(copyfile[2], "-n") != 0){//잘못된 옵션 사용의 경우
        printUsage();
        exit(0);
    }

    if(strlen(copyfile[1]) > MAX_LENGTH) { //첫 번째 인자로 입력받은 경로가 길이제한을 넘는 경우
        printf("Error : Too long file name\n");
        exit(0);
    }

    check_path = realpath(copyfile[1], NULL);
    if (check_path == NULL) { //실제 경로 생성
        realpath(copyfile[1], real_path);
    } 
    else {
        unreal = 1;
        realpath(copyfile[1], real_path);
    }

    if(option_n_position > 0){// -n 옵션 경로 생성
        if(strcmp(copyfile[option_n_position], "md5") == 0 || strcmp(copyfile[option_n_position], "sha1") == 0){
            printUsage();
            exit(1);
        }
        new_check_path = realpath(copyfile[option_n_position], NULL);

        if (new_check_path == NULL) {
            realpath(copyfile[option_n_position], new_path);
        } else {
            realpath(copyfile[option_n_position], new_path);
        }

        char home_dir[MAX_LENGTH];
        strcpy(home_dir,"/home/");
        strcat(home_dir,username);
        
        if(strncmp(new_path, home_dir, strlen(home_dir)) != 0){//home에서 벗어나는 경우
            printf("\"%s\" can't be backuped\n", real_path);
            exit(1);
        }

        if(strstr(new_path, "/backup") != NULL){
            printf("\"%s\" can't be backuped\n", real_path);
            exit(1);
        }

    }

    if(option_d_position == 1 || option_n_position == 1){//옵션 위치 오류
        printUsage();
        exit(0);
    }

    char home_dir[MAX_LENGTH];
    strcpy(home_dir,"/home/");
    strcat(home_dir,username);
    
    if(strncmp(real_path, home_dir, strlen(home_dir)) != 0){//home에서 벗어나는 경우
        printf("\"%s\" can't be backuped\n", real_path);
        exit(1);
    }

    if(strstr(real_path, "/backup") != NULL){//backup 디렉토리 포함하는 경우
        printf("\"%s\" can't be backuped\n", real_path);
        exit(1);
    }

    if(strlen(real_path) >= 4077 ){//파일길이 오류
        printf("Error : Too long file path");
        exit(0);
    }

    char *p = strstr(real_path, username);
    int pos = p-real_path + strlen(username);

    //backup 경로 생성
    if(snprintf(backup_path, MAX_LENGTH, "%.*s/backup%s", pos, real_path, real_path+pos)>MAX_LENGTH){
        printf("\"%s/%s\" can't be backuped\n", real_path, basename(copyfile[1]));
        return 0;
    }
////////////////////////////////////// 예외처리 + 파일 경로 설정 ////////////////////////////////////////////

    if(access(real_path, 0) == 0){//작업 dir에 파일 또는 dir이 존재하며 접근이 가능한 경우
        int result = stat(real_path, &filestat); //file과 dir 구분

        if(result == -1){
            printf("stat error\n");
        }
        else if(S_ISDIR(filestat.st_mode)){//directoy case

            if(option_d_position < 0 || option_d_position != 2) {//-d옵션이 없거나 위치가 잘못 설정된 경우
                printf("Error : Please Check Usage\n");
                exit(0);
            }

            if(option_n_position >= 0){//n 옵션 사용시 복구 위치 변환
                strcpy(real_path, new_path);
            }
            else{
                if(argc > 4){
                    printUsage();
                    exit(1);
                }
            }
            

            compareDir(backup_path, real_path, copyfile[argc-1]);
            
        }
        else{ //file case
            if(option_d_position >= 0){//잘못된 옵션 사용 경우
                printUsage();
                exit(0);
            }

            if(option_n_position >= 0){//n 옵션 사용시 복구 위치 변환
                strcpy(real_path, new_path);
            }

            //file이 존재하지 않는 경우
            if(compareFiles(backup_path, real_path, &head, copyfile[argc-1])){
                printf("Error: File is not in backup \n");
                return 0;
            }

            //일치하는 file이 하나만 존재하는 경우
            if(count_nodes(head) == 1){
                int result;

                if(strcmp(copyfile[argc -1], "sha1") == 0){
                    result = compareFilesSHA(real_path, head->path);

                    if(result == 0){
                        printf("This is a same File\n");
                        exit(0);
                    }

                }
                else{
                    result = compareFilesMD5(real_path, head->path);

                    if(result == 0){
                        printf("This is a same File\n");
                        exit(0);
                    }

                }

                dirname(backup_path);
                strcat(backup_path, "/");
                strcat(backup_path, head->name);

                recover(backup_path, real_path);

            }
            //일치하는 파일이 존재하지 않는 경우
            else if(count_nodes(head) == 0 ){
                printf("This is a same File\n");
                exit(1);
            }
            //일치하는 파일이 여러개 존재하는 경우
            else {
                long long choosed_num;
                printNodes(real_path, &head);
                printf("Choose file to recover\n");
                printf(">> ");
                scanf("%lld", &choosed_num);

                if(choosed_num == 0){
                    exit(EXIT_SUCCESS);
                }

                recoverFileNum(&head, choosed_num, real_path);

            }
        }
    }
    //현제 dir에 file이나 dir이 존재하지 않는 경우 -> 파일인 경우
    else if(isAccess(backup_path, backup_root) == 1){
        if(option_d_position >= 0){
            printUsage();
            exit(0);
        }

        if(option_n_position >= 0){//n 옵션 사용시 복구 위치 변환
            strcpy(real_path, new_path);
        }
        //잘못된 경로거나 파일에 접근할 수 없는 경우
        if(compareFiles(backup_path, real_path, &head, copyfile[argc-1])){
            printf("Error: access error\n");
            return 0;
        }
        //백업 파일이 한개 존제하는 경우
        if(count_nodes(head) == 1){
            int result;

            if(strcmp(copyfile[argc -1], "sha1") == 0){
                result = compareFilesSHA(real_path, head->path);

                if(result == 0){
                    printf("This is a same File\n");
                    exit(0);
                }

            }
            else{
                result = compareFilesMD5(real_path, head->path);

                if(result == 0){
                    printf("This is a same File\n");
                    exit(0);
                }

            }

            dirname(backup_path);
            strcat(backup_path, "/");
            strcat(backup_path, head->name);

            recover(backup_path, real_path);

        }
        //백업 파일이 여러개 존재하는 경우
        else {
            long long choosed_num;
            printNodes(real_path, &head);
            printf("Choose file to recover\n");
            printf(">> ");
            scanf("%lld", &choosed_num);

            if(choosed_num == 0){
                exit(EXIT_SUCCESS);
            }

            recoverFileNum(&head, choosed_num, real_path);

        }
    }
    //dir인 경우
    else if(isAccess(backup_path, backup_root) == 10){
        if(option_d_position < 0 || option_d_position != 2){//-d옵션이 없거나 위치가 잘못 설정된 경우
                printf("Error : Please Check Usage\n");
                exit(0);
            }

            if(option_n_position >= 0){//n 옵션 사용시 복구 위치 변환
                strcpy(real_path, new_path);
            }else{
                if(argc >3){
                    printUsage();
                    exit(1);
                }
            }

            compareDir(backup_path, real_path, copyfile[argc-1]);
    }
    //존재하지 않는 경우
    else{
        printf("\"%s\" is'n in backup dir\n", real_path);
        exit(1);
    }


}
