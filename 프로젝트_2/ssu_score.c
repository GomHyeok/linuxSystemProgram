#include "ssu_score.h"
#include "blank.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <dirent.h> // DIR ..
#include <sys/types.h>  //struct stat buf; 
#include <sys/stat.h> //struct stat buf; 
#include <fcntl.h> //O_RDONLY
#include <signal.h> //SIGKILL
#include <unistd.h> //read(), write() 
#include <stdlib.h> // system(); atoi() exit()
#include <time.h> //difftime() time()
#include <sys/types.h>    // pid_t

#define MAX_LINE_LENGTH 1024//길이의 최대값
#define MAX_ROWS 1000//행의 최대값
#define MAX_COLS 100//열의 최대값

int compare_int_asc(const void *a, const void *b) {//소팅 컴패어 함수
    return (*(int*)a - *(int*)b);//내림찬순
}

int compare_int_desc(const void *a, const void *b) {//소팅 컴페어 함수
    return (*(int*)b - *(int*)a);//오름차순
}

extern struct ssu_scoreTable score_table[QNUM];//문제별 점수 데이터 테이블(extern 변수)
extern char id_table[SNUM][10];//100개의 id string table

struct ssu_scoreTable score_table[QNUM];
char id_table[SNUM][10];

char saved_path[BUFLEN];//저장 경로
char stuDir[BUFLEN];//학생 디렉토리 변수
char ansDir[BUFLEN];//답변 디렉토리 변수
char newDir[BUFLEN];//새로운 디렉토리 경로
char errorDir[BUFLEN];//에러 디렉토리 변수
char threadFiles[ARGNUM][FILELEN];//t옵션 해당 파일 저장 경로
char iIDs[ARGNUM][FILELEN];//학번 저장 변수
//option 실행 여부 변수
int argStuIdx=0;//argument student 개수
int nOption = false;//n 옵션 여부
int mOption = false;//m 옵션 여부
int cOption = false;//c 옵션 여부
int cCnt=0;//c 옵션 변수 개수
int pOption = false;//p 옵션 여부
int pCnt=0;//P 옵션 변수 개수
int cpCnt=0;//c,p의 최대값
int tOption = false;//t옵션여부
int tCnt=0;//t옵션 변수 개수
int sOption = false;//soption 여부
int eOption = false;//e option 여부
int tableSize = 0;//score_table size
int idSize = 0;

int sortType;//sorting type
int sortStandard;//sorting 변수

void ssu_score(int argc, char *argv[])
{
	int i;

	// 들어온 인자 개수만큼 탐색 - h옵션의 경우 어느위치에서든지 들어올 수 있음
	for(i = 0; i < argc; i++){
		// 해당 인자가 -h 옵션인 경우
		if(!strcmp(argv[i], "-h")){
			// usage출력
			print_usage();
			return;
		}
	}

	//saved_path를 BUFLEN 길이만큼 0으로 초기화 
	memset(saved_path, 0, BUFLEN);

	// 들어온 인자가 3개 이상이고(./ssu_score 1개, stuDir 1개, ansDir1개) 첫번째 인자가 -p가 아닌 경우에 각 dir이름 지정(-p 옵션의 경우 stuDir, ansDir 생략가능)
	if(argc >= 3 && strcmp(argv[1], "p") != 0){
		// stuDir = argv[1]
		strcpy(stuDir, argv[1]);
		// ansDir = argv[2]
		strcpy(ansDir, argv[2]);
	}
	
	//모르는 option이 들어온경우 return false이므로, 모르는 option 들어온 경우 프로그램 종료
	if(!check_option(argc, argv))
		exit(1);
	
	// // dir없이 optionP만 들어온 경우 -> 채점 과정 생략, 채점 결과 파일을 토대로 프로그램 실행
	// if(!mOption && !eOption && !tOption && pOption 
	// 		&& !strcmp(stuDir, "") && !strcmp(ansDir, "")){
	// 	// p옵션만 실행 후 프로그램 종료
	// 	do_pOption(iIDs);
	// 	return;
	// }
	
	// 현재 pwd를 saved_path에 저장
	getcwd(saved_path, BUFLEN);

	// stuDir이 존재 여부 확인 
	if(chdir(stuDir) < 0){
		// 존재하지 않는경우 error문 출력 및 종료
		fprintf(stderr, "%s doesn't exist\n", stuDir);
		return;
	}
	// stuDir 존재하는 경우에 stdDir변수에 stdDir 변수 저장 
	getcwd(stuDir, BUFLEN);

	// 다시 이전에 저장해둔 현재 디렉터리로 pwd 옮김
	chdir(saved_path);
	// ansDir 존재 여부 확인
	if(chdir(ansDir) < 0){
		// 존재하지 않는경우 error문 출력 및 종료
		fprintf(stderr, "%s doesn't exist\n", ansDir);
		return;
	}
	//ansDir 존재하는 경우에 ansDir변수에 ansDir 변수 저장
	getcwd(ansDir, BUFLEN);

	// 다시 이전에 저장해둔 현재 디렉터리로 pwd 옮김
	chdir(saved_path);

	// scoreTable 생성여부 확인 및 생성 
	set_scoreTable(ansDir);

	// 학생별 채점표 생성
	set_idTable(stuDir);

	// m옵션이 들어온 경우 m옵션(scoretable의 문제 배점 수정) 진행 후 채첨
	if(mOption)
		do_mOption();

	// 두옵션의 경우 첫 입력 학생이 std_dir에 존재하지 않는경우 에러출력 후 종료
	if ((pOption&&pCnt!=0) || (cOption&&cCnt!=0)) {
		//size = 학생수
		int size = sizeof(id_table) / sizeof(id_table[0]);
		// exist 변수선언
		int exist = 0;
		// 학생 수만큼 진행
		for(int num = 0; num < size; num++)
		{	
			// id_table에 첫번째 인자로 들어온 학생이 존재한다면 exist =1하고 break
			if(!strcmp(id_table[num], iIDs[0])) {
				exist = 1;
				break;
			}	
		}
		//존재 하지 않는경우 
		if (!exist) {
			// 에러문 출력 후에 프로그램 종료
			printf("first students arg doesn't exist\n");
			return ;
		}
	}
	// 두옵션이 들어온 경우 student는 한번만 나와야함 
	if (pOption && cOption){
		if (cCnt>0 && pCnt==0) cpCnt=cCnt;//c가 최대값
		else if (pCnt>0 && cCnt==0) cpCnt=pCnt;//p가 최대값
		// 둘다 students list인자를 받은 경우 에러
		else if (pCnt>0 && cCnt >0){
			printf("double elements of [STUDENTIDS ...]\n");
			return ;
		}
	}
	
	printf("grading student's test papers..\n");
	// 학생별 채점 진행
	score_students(stuDir);

	return;
}

int check_option(int argc, char *argv[])//option 확인 함수
{
	int i, j;//다양한 위치 저장 변수
	int c;//getopt 실행 결과 저장
	int first =0;

	while((c = getopt(argc, argv, "n:mce:tps:")) != -1)
	{
		switch(c){
			// 옵션 n이 입력된 경우
			case 'n':
				// nOption 변수를 true로 변경
				nOption = true;
				// option argument로 입력된 csvfilename을 newDir변수에 저장 
				strcpy(newDir, optarg);
				// 입력받은 새 파일의 이름의 확장자가 csv가 아닌경우에 에러메세지 출력 후 종료
				if (get_file_type(newDir) != CSV) {
					printf("new table extension must .csv\n");
					return false;
				}
				break;
			// 옵션 m이 입력된 경우
			case 'm':
				//mOption 변수를 true로 변경
				mOption = true;
				break;
			case 'c':
				//cOption 변수를 true로 변경
				cOption = true;
				// i변수에 optind값(getopt()함수가 처리한 마지막 옵션인자 다음 인덱스) 저장
				i = optind;
				// 들어온 id의 cnt
				j = 0;
				first =1;
				// i가 argc보다 작고 다음 인자의 시작이 -(옵션)이 아닌경우 까지 진행
				while(i < argc && argv[i][0] != '-'){
					// id의 cnt가 ARGNUM을 초과한 경우
					if(j >= ARGNUM)
						// 초과했다는 워닝 메세지 출력
						if (first) {printf("Maximum Number of Argument Exceeded. :: %s", argv[i]); first =0;}
						else printf(" %s", argv[i]);
					else {// iID배열에 argv로 입력된 id값 저장
						strcpy(iIDs[j], argv[i]);
						idSize++;
					}
					i++;
					j++;
				}
				if(j >= ARGNUM) printf("\n");
				cCnt = j;
				break;
			// 옵션 p가 입력된 경우
			case 'p':
				//pOption 변수를 true로 변경
				pOption = true;
				// i변수에 optind값(getopt()함수가 처리한 마지막 옵션인자 다음 인덱스) 저장
				i = optind;
				// 들어온 id의 cnt
				j = 0;
				first = 1;
				// i가 argc보다 작고 다음 인자의 시작이 -(옵션)이 아닌경우 까지 진행
				while(i < argc && argv[i][0] != '-'){
					// id의 cnt가 ARGNUM을 초과한 경우
					if(j >= ARGNUM)
						// 초과했다는 워닝 메세지 출력
						if (first) {printf("Maximum Number of Argument Exceeded. :: %s", argv[i]); first =0;}
						else printf(" %s", argv[i]);
					else // iID배열에 argv로 입력된 id값 저장
						strcpy(iIDs[j], argv[i]);
					i++;
					j++;
				}
				//모두 출력한 이후
				if(j >= ARGNUM) printf("\n");
				//개수 저장
				pCnt = j;
				break;
			case 't'://t option 실행시
				tOption = true; //toption ture
				i = optind;// i변수에 optind값(getopt()함수가 처리한 마지막 옵션인자 다음 인덱스) 저장
				j = 0;// 들어온 id의 cnt
				first=1;//처음 확인 flag
				while(i < argc && argv[i][0] != '-'){ //인자 확인
					if(j >= ARGNUM) //인자를 넘은 경우
						if (first) {printf("Maximum Number of Argument Exceeded. :: %s", argv[i]); first =0;}//에러문 출력
						else printf(" %s", argv[i]);//넘은 변수 출력
					else{
						strcpy(threadFiles[j], argv[i]);//넘지 않은 변수 저장
					}
					i++; 
					j++;
				}
				if(j >= ARGNUM) printf("\n");//종료시 출력
				tCnt = j;//개수 저장
				break;
			case 's'://soption 실행서
				sOption = true;//soption ture
				i = optind;//i 변수에 optind
				char category[BUFLEN];//기준 값
				strcpy(category, optarg);//저장
				
				char input_type[BUFLEN];//정렬 기준
				
				if (argv[i]) strcpy(input_type, argv[i]);//정렬기준 예외처리
				else {//예외시 예외문 출력
					printf("input type error\n");
					return false;
				}

				if (strcmp(category, "stdid")!=0 && strcmp(category, "score")!=0){//카테고리 예외처리
					printf("category error\n");//예외문 출력
					return false;
				}
				if (strcmp(input_type, "-1")!=0 && strcmp(input_type, "1")!=0){//기준 예외처리
					printf("input type error\n");//예외문 출력
					return false;
				}

				if (!strncmp(category, "stdid", strlen("stdid"))) sortStandard = STDID;//학번기준
				else sortStandard = SCORE; //점수기준

				if (input_type[0] == '-') sortType = ASC;//내림차순
				else sortType = DESC;//오름차순
				// option Id type 뒤로 이동
				optind+=1;

				break;
			case 'e'://eoption 
				eOption = true;//eoptino true
				strcpy(errorDir, optarg);//디랙토리 이름 저장
				
				if(access(errorDir, F_OK) < 0){//존재하지 않는 경우
					char *path = errorDir;//경로 
					char *p = path;//경로
					while(*p) {
						if(*p == '/') {//경로 검사
							*p = '\0';//디렉토리 하나씩 검사
							mkdir(path,0755);//디렉토리 생성
							*p='/';
						}
						p++;
					}
					mkdir(path,0755);//최종 디렉토리 생성
				}
				else{//존재시
					rmdirs(errorDir);//삭제후
					mkdir(errorDir, 0755);//재생성
				}
				break;

			case '?'://알수 없는 옵션 예외처리
				printf("Unkown option %c\n", optopt);
				return false;
		}
	}

	return true;
}

// -p 옵션
void do_pOption(struct wrongTable *head){
	struct wrongTable *current = head;//틀린 문제 링크드리스크 헤더
	int flag = 1;//처음인 경우 
	printf("wrong problem : ");//출력

	while(current->next != NULL) {//링크드 리스크 탐색
		if(flag){//처음인경우
			if(current->qscore == (int)current->qscore)//정수인 경우 
				printf("%s(%d)", current->qname, (int)current->qscore);
			else//실수인 경우
				printf("%s(%.2lf)", current->qname, current->qscore);
			flag = 0;//초기화
		}
		else {//처음이 아닌 경우
			if(current->qscore == (int)current->qscore) //정수인 경우
				printf(", %s(%d)", current->qname, (int)current->qscore);
			else//실수인경우
				printf(", %s(%.2lf)", current->qname, current->qscore);
		}
		current=current->next;//이동
	}
	if(current->qscore == (int)current->qscore) //마지막 노드 출력//정수
		printf(", %s(%d)", current->qname, (int)current->qscore);
	else//실수인 경우
		printf(", %s(%f)", current->qname, current->qscore);

	printf("\n");
}

void do_mOption()//moption 실행 함수
{
	double newScore;
	char modiName[FILELEN];
	char filename[2048];
	char *ptr;
	int i;
	// 파일의 최대 길이 만큼 ptr수정
	ptr = malloc(sizeof(char) * FILELEN);

	while(1){
		//정상적인 문제 번호 확인 플래그
		int flag = 0;
		// 배점을 바꾸고 싶은 q 번호를 입력 받음
		printf("Input question's number to modify >> ");
		scanf("%s", modiName);
		// no가 입력으로 들어온 경우 더이상 입력받지 않음
		if(strcmp(modiName, "no") == 0)
			break;
		// score_table을 돌면서 입력받은 문제번호와 같은 행을 찾음
		for(i=0; i < tableSize; i++){
			// ptr에 문제 번호를 저장하고
			strcpy(ptr, score_table[i].qname);
			// 뒤에 붙은 확장자를 제외한 문제의 이름만 ptr에 저장
			ptr = strtok(ptr, ".");
			// ptr이 입력받은 modiName변수와 같은 경우
			if(!strcmp(ptr, modiName)){
				// 현재 지정된 배점을 출력 후
				printf("Current score : %.2f\n", score_table[i].score);
				// 사용자에게 바꾸고 싶은 점수를 입력 받음
				printf("New score : ");
				scanf("%lf", &newScore);
				// 버퍼를 비움(개행 제거)
				getchar();
				// 해당 문제의 score_table에 위치에서 점수를 newscore로 바꿔줌
				score_table[i].score = newScore;
				//플래그 변경
				flag = 1;
				// 저장했으므로 for문 탈출
				break;
			}
		}
		if(flag == 0){
			fprintf(stderr, "wrong question name\n");
			exit(1);
		}
	}
	// score_table의 경로를 filename에 저장
	sprintf(filename, "%s/%s", ansDir, "score_table.csv");
	// 새로 변경된 내역을 scoreTable에 덮어쓰기 함
	write_scoreTable(filename);
	// ptr 메모리 반납
	free(ptr);
}

//점수 테이블 생성
void set_scoreTable(char *ansDir)
{
	char filename[FILELEN];

	//점수 테이블 파일은 “./ANS/score_table.csv” 이름으로 생성
	sprintf(filename, "%s/%s", ansDir, "score_table.csv");
	
	// check csv file exist 
	// 파일이 존재하는 경우
	if(access(filename, F_OK) == 0)
		// scoretable 읽음 
		read_scoreTable(filename);
	else{ 
		// score_table이 존재하지 않는경우
		// scoretable 생성
		make_scoreTable(ansDir);
		// scoretable 저장
		write_scoreTable(filename);
	}
}

// scoretable 읽는 함수
void read_scoreTable(char *path)
{
	FILE *fp;
	char qname[FILELEN];
	char score[BUFLEN];
	int idx = 0;

	// 파일 read전용으로 오픈 (write 진행 x)
	if((fp = fopen(path, "r")) == NULL){
		// 파일 오픈 실패시 에러메세지 출력 후 종료
		fprintf(stderr, "file open error for %s\n", path);
		return ;
	}

	// csv 파일 끝까지 읽기 
	// , 쉼표를 기준으로 쉼표 이전 문자열은 qname/ 쉼표 이후 문자열은 score 변수로 읽음 
	while(fscanf(fp, "%[^,],%s\n", qname, score) != EOF){
		//구조체 배열인 score_table idx번째에 .qname 에 qname저장
		strcpy(score_table[idx].qname, qname);
		// 마찬가지로 score까지 double형식의 실수로 변환해서 저장 후 idx+1 처리
		score_table[idx++].score = atof(score);
		tableSize++;
	}

	// 파일 닫기
	fclose(fp);
}

// scoreTable이 존재하지 않는 경우 csv파일 생성하는 함수 
void make_scoreTable(char *ansDir)
{
	int type, num;
	double score, bscore, pscore;
	struct dirent *dirp;
	DIR *dp;
	int idx = 0;
	int i;

	// 1번형식, 2번형식중에 선택 받음
	num = get_create_type();

	// 1번 형식인 경우 
	if(num == 1)
	{
		// blank 형식의 질문인 경우 점수 설정 
		printf("Input value of blank question : ");
		scanf("%lf", &bscore);
		// program 형식의 질문인 경우 점수 설정 
		printf("Input value of program question : ");
		scanf("%lf", &pscore);
	}

	//ansDir을 열고 dp에 저장
	if((dp = opendir(ansDir)) == NULL){
		// 디렉터리를 열수 없는 경우 에러 메시지 출력 후 종료
		fprintf(stderr, "open dir error for %s\n", ansDir);
		return;
	}

	// 디렉터리내의 파일을 하나씩 가져옴
	while((dirp = readdir(dp)) != NULL){
		// 현재 디렉토리 자체이거나 부모인 경우는 건너뛰기함
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		// 파일의 확장자를 가져옴 (확장자가 .c, .csv, .txt가 아닌 경우 -1를 반환)
		if((type = get_file_type(dirp->d_name)) < 0)
			// 원하는 확장자가 아닌경우 continue
			continue;

		//score_table에 파일명 저장
		strcpy(score_table[idx].qname, dirp->d_name);
		idx++;
	}
	// open한 디렉터리를 닫음 
	closedir(dp);

	// score table 배열을 문제 이름순으로 정렬
	sort_scoreTable(idx);

	for(i = 0; i < idx; i++)
	{	
		// qname의 확장자를 type변수에 받아옴 (확장자의 경우 define으로 저장되어있음)
		type = get_file_type(score_table[i].qname);

		// 저장형식이 1번인 경우( txt vs c )
		if(num == 1)
		{	
			// 현재 파일의 타입이 .txt인 경우
			if(type == TEXTFILE)
				// txt타입에 해당하는 점수 배정
				score = bscore;
			// 현재 파일의 타입이 .c인 경우
			else if(type == CFILE)
				// c타입에 해당하는 점수 배정
				score = pscore;
		}
		// 저장형식이 1번인 경우( 각 문제별로 배점 지정 )
		else if(num == 2)
		{	
			// 현재 문제에 배정할 점수를 입력 받음 
			printf("Input of %s: ", score_table[i].qname);
			scanf("%lf", &score);
		}

		// 배정받은 점수를 해당 문제에 저장해줌 
		score_table[i].score = score;
	}
}

// scoreTable에 write 해주는 함수
void write_scoreTable(char *filename)
{
	int fd;
	char tmp[BUFLEN];
	int i;
	// scoretable의 행 계산 (총 문제수 = num)
	int num = sizeof(score_table) / sizeof(score_table[0]);

	// filename 생성 
	if((fd = creat(filename, 0666)) < 0){
		fprintf(stderr, "creat error for %s\n", filename);
		return;
	}

	for(i = 0; i < num; i++)
	{
		if(score_table[i].score == 0)
			break;
		// csv파일이므로 ,가 구분자임 => ,를 기준으로 앞에는 qname을 오른쪽에는 score를 tmp에 저장
		sprintf(tmp, "%s,%.2f\n", score_table[i].qname, score_table[i].score);
		// tmp의 값을 file에 write
		write(fd, tmp, strlen(tmp));
	}
	// file descriptor 닫기
	close(fd);
}

// studir에서 학생 id정보 받아오는 함수 => 디렉터리 탐색
void set_idTable(char *stuDir)
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	char tmp[BUFLEN];
	int num = 0;

	// stuDir 디렉터리 열기
	if((dp = opendir(stuDir)) == NULL){
		// 디렉터리를 여는데 실패한 경우 에러메세지와 함께 종료
		fprintf(stderr, "opendir error for %s\n", stuDir);
		exit(1);
	}

	// 디렉터리 정보를 하나씩 받아옴
	while((dirp = readdir(dp)) != NULL){
		// 엔트리가 현재 디렉터리/상위 디렉터리인 경우 넘어가기
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		// stuDir과 d_name을 합쳐서 tmp변수에 저장
		sprintf(tmp, "%s/%s", stuDir, dirp->d_name);
		// tmp경로의 state를 받아옴
		stat(tmp, &statbuf);

		// 학생id로 이루어진 디렉터리여야 하기때문에 해당 엔트리가 디렉터리인지 검사
		if(S_ISDIR(statbuf.st_mode))
			// 디렉터리인 경우 해당 폴더는 학생 폴더이므로 id_table에 d_name을 저장해준 후 num+1진행
			strcpy(id_table[num++], dirp->d_name);
		else
			continue;
	}
	// 디렉터리 포인터 닫기
	closedir(dp);
	// 학번순으로 정렬
	sort_idTable(num);
}
// id_table을 정렬해주는 함수
void sort_idTable(int size)
{
	int i, j;
	char tmp[10];
	// 버블 정렬방법을 활용하여 학번을 오름차순으로 정렬 수행
	for(i = 0; i < size - 1; i++){
		// 이미 정렬된 요소는 제외하고 비교
		for(j = 0; j < size - 1 -i; j++){
			//j번째와 j+1번째를 비교해서 j가 j+1보다 큰경우에 두개 위치를 바꿔줌
			if(strcmp(id_table[j], id_table[j+1]) > 0){
				// tmp를 이용하여 swap진행
				strcpy(tmp, id_table[j]);
				strcpy(id_table[j], id_table[j+1]);
				strcpy(id_table[j+1], tmp);
			}
		}
	}
}

// score table 배열을 문제 이름순으로 정렬하는 함수 -> 버블 정렬 알고리즘을 사용하여 정렬함
void sort_scoreTable(int size)
{
	int i, j;
	struct ssu_scoreTable tmp;
	int num1_1, num1_2;
	int num2_1, num2_2;

	// 반복문 실행
	for(i = 0; i < size - 1; i++){
		// 이미 정렬된 부분을 제외하고 배열의 첫번째 요소부터 비교
		for(j = 0; j < size - 1 - i; j++){
			// j번째 요소에서 qname, num1, num2를 받아옴
			get_qname_number(score_table[j].qname, &num1_1, &num1_2);
			// j+1번째 요소에서 qname, num1, num2를 받아옴
			get_qname_number(score_table[j+1].qname, &num2_1, &num2_2);

			// 문제 번호를 정렬 한쪽의 num1이 크거나, 각 num1이 같은경우 num2가 큰경우에 
			if((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2))){
				// tmp변수를 이용해서 swap과정 진행 
				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));
				memcpy(&score_table[j], &score_table[j+1], sizeof(score_table[0]));
				memcpy(&score_table[j+1], &tmp, sizeof(score_table[0]));
			}
		}
	}
}

void get_qname_number(char *qname, int *num1, int *num2)
{
	char *p;
	char dup[FILELEN];
	// qname 을 dup에 copy
	strncpy(dup, qname, strlen(qname));
	// -와 .를 기준으로 dup을 자름 => 가장 먼저 나온 번호를 num1에 저장
	*num1 = atoi(strtok(dup, "-."));
	// 다시 -.를 기준으로 자름 
	p = strtok(NULL, "-.");
	// 두번째 숫자가 없는 경우 num2는 0으로 저장 
	if(p == NULL)
		*num2 = 0;
	// -가 등장하여 두번째 숫자가 있는 경우 atoi를 사용하여 num2를 저장 
	else
		*num2 = atoi(p);
}

// make_scoreTable시에 호출되는 함수, 점수를 산정할 방법을 선택받음
int get_create_type()
{
	int num;

	// 제대로 입력받을 때까지 반복
	while(1)
	{	
		// 출력문 (타입별 설명)
		printf("score_table.csv file doesn't exist in \"%s\"!\n", ansDir);
		printf("1. input blank question and program question's score. ex) 0.5 1\n");
		printf("2. input all question's score. ex) Input value of 1-1: 0.1\n");
		printf("select type >> ");
		scanf("%d", &num);
		// 입력받은 번호가 1,2가 아닌경우
		if(num != 1 && num != 2)
			printf("not correct number!\n");
		// 제대로 입력받은 경우 반복문 탈출
		else
			break;
	}
	// 점수 배정 타입 반환
	return num;
}
// 학생들채점 함수
void score_students()
{
	double score = 0;
	int num;
	int fd;
	char tmp[2048];
	// size = 학생 수
	int size = sizeof(id_table) / sizeof(id_table[0]);
	int qsize = sizeof(score_table) / sizeof(score_table[0]);
	int cScores[SNUM][QNUM];
	
	char filename[2050];
	// n옵션이 들어오지 않은 경우
	if (!nOption) {
		sprintf(filename, "%s/%s", ansDir, "score.csv");
	}
	// n옵션이 들어온 경우
	else {
		// n옵션이 절대경로로 들어온 경우
		if (!strncmp(ansDir, newDir, strlen(ansDir))) snprintf(filename, sizeof(filename), "%s", newDir);
		// n옵션이 상대경로로 들어온경우
		else {
			sprintf(filename, "%s/%s", ansDir, newDir);
		}
	}

	// 채점 테이블 파일 열기
	if((fd = creat(filename, 0666)) < 0){
		// 열지 못하는 경우 에러 메세지 출력 후 종료
		fprintf(stderr, "creat error for score.csv");
		return;
	}
	// 문제번호를 나타내는 첫번째 행 먼저 wrtie(column명 부터 적어 두기)
	write_first_row(fd);

	for(int i=0; i<qsize; i++){
		if(score_table[i].score == 0){
			break;
		}

		int type = get_file_type(score_table[i].qname);

		if(type < 0) {
			continue;
		}
		sprintf(tmp, "%s/%s", ansDir, score_table[i].qname);

		// 해당 파일의 접근권한 확인
		if(access(tmp, F_OK) == 0){
			if(type < 0){
				continue;
			}
			else if(type == CFILE){
				for(int j=0; j<size; j++){
					if(!strcmp(id_table[j], "")){
						break;
					}
					double result = score_program(id_table[j], score_table[i].qname);
					cScores[j][i] = result;
				}
			}
		}
		
	}

	// 학생 수만큼 진행
	for(num = 0; num < size; num++)
	{	
		// 테이블에 학생 이름이 없다면 break
		if(!strcmp(id_table[num], ""))
			break;
		// 학생 id정보를 먼저 작성(구분자 포함)후 tmp에 저장
		sprintf(tmp, "%s,", id_table[num]);
		// tmp를 file에 write
		write(fd, tmp, strlen(tmp)); 
		// 해당 학생의 점수를 채점 후, 학생별 점수를 score변수에 저장
		score += score_student(fd, id_table[num], cScores[num]);
	}

	if (cOption) printf("Total average : %.2f\n", score / num);

	// 해당 파일이 저장에 성공 했다는 문구 출력
	printf("result saved.. (%s)\n", filename);

	if(eOption) {
		char resolved_errorDir[PATH_MAX];
		realpath(errorDir, resolved_errorDir);
		strcpy(errorDir, resolved_errorDir);
		printf("error saved.. (%s)\n", errorDir);
	}
	// fd 닫기
	close(fd);
	
	if (sOption) do_sOption();
		
}

//string type의 변수를 double 형식의 변수로 변환 함수
double parseDouble(char s[]) {
    return atof(s);//atof()함수 확용
}

//soption 처리 함수
void do_sOption(){
	FILE *fp;//파일 포인터
	char tmp[BUFLEN];//임시저장변수
	char qname[BUFLEN];//문제 이름
	char *p;//아이디와 문자 자르기 변수
	
    int rows = 0, cols = 0;//행과 열

	struct ssu_sort *header;//링크드리스트 헤더

	char filename[2048];//저장 파일명
	sprintf(filename, "%s/%s", ansDir, "score.csv");//파일명 복사

	if((fp = fopen(filename, "r")) == NULL){//파일 오픈과 예외처리
		fprintf(stderr, "score.csv file doesn't exist\n");
		return;
	}
	// get qnames
	fscanf(fp, "%s\n", qname);//문제들 이름 획득
	rows = 0;//row 초기화
    while(fscanf(fp, "%s\n", tmp) != EOF){//파일 돌면서 정보 획득
		struct ssu_sort nodes;//정보 저장 변수
		cols = 0; //col 초기화
		strtok(tmp, ",");//,단위 정보 분리
		nodes.stdid = strdup(tmp);//학번 저장
		cols++;//열 개수
		nodes.next = NULL;//노드 초기화
        // tokenize line
		while((p = strtok(NULL, ",")) != NULL){//모든 점수 저장
			nodes.questions[cols] = strdup(p);//문제 변수에 점수 저장
			cols++;//열 개수 증가
		}
		add_Node(&header, nodes, cols);//노드 추가
        rows++;//행 개수 증가
    }
    fclose(fp);//파일 닫음

	// sort by column
	int col_index;

	for(int i=0; i<rows-1; i++)//학생수 만큼 반복
	{
		if (sortStandard == STDID) {//학번 기준 정렬
			struct ssu_sort *current = header;//현제 위치 노드
			while(current->next!=NULL){//반복
				if(current->next == NULL){//종료조건
					break;
				}
				struct ssu_sort *nextnode = current->next;//다음 노드 정보 저장
				double cmp = parseDouble(current->stdid) - parseDouble(nextnode->stdid);//다음 노드와 학번 비교
				if(sortType == ASC) {//내림차순 정렬
					if(cmp < 0.0){//변경해야 하는 경우
						char *tmpid;//임시 학번
						tmpid = malloc(strlen(current->stdid)+1);//메모리 할당
						strcpy(tmpid, current->stdid);//저장
						strcpy(current->stdid, nextnode->stdid);//변경
						strcpy(nextnode->stdid, tmpid);//변경
						for(int k=1; k<cols; k++){//문제 점수 변경
							char *tmpQuestion;//임시 변수
							tmpQuestion = malloc(strlen(current->questions[k])+1);//메모리 할당
							strcpy(tmpQuestion, current->questions[k]);//문제 복사
							strcpy(current->questions[k], nextnode->questions[k]);//변경
							strcpy(nextnode->questions[k], tmpQuestion);//변경
						}
					}
				}
				else{//오름차순 정렬
					if(cmp > 0.0){//변경해야하는경우
						char *tmpid;//학변 저장 변수
						tmpid = malloc(strlen(current->stdid)+1);//메모리 할당
						strcpy(tmpid, current->stdid);//학번 저장
						strcpy(current->stdid, nextnode->stdid);//변경
						strcpy(nextnode->stdid, tmpid);//변경
						for(int k=1; k<cols; k++){//문제 점수 저장
							char *tmpQuestion;//문제 점수 변수
							tmpQuestion = malloc(strlen(current->questions[k])+1);//메모리 할당
							strcpy(tmpQuestion, current->questions[k]);//점수 저장
							strcpy(current->questions[k], nextnode->questions[k]);//변경
							strcpy(nextnode->questions[k], tmpQuestion);//변경
						}
					}
				}
				current = current->next;//다음 노드로 이동
			}
		}
		else if (sortStandard == SCORE) {//점수 기준 정렬
			col_index = cols-1;//총점 위치
			struct ssu_sort *current = header;//현재 노드 위치
			while(current->next!=NULL){//반복
				if(current->next == NULL){//종료 조건
					break;
				}
				struct ssu_sort *nextnode = current->next;//다음 노드
				double cmp = parseDouble(current->questions[col_index]) - parseDouble(nextnode->questions[col_index]);//점수차이
				if(sortType == ASC) {//내림차순
					if(cmp < 0.0){//변경경우
						char *tmpid;//학번 변수
						tmpid = malloc(strlen(current->stdid)+1);//메모리 할당
						strcpy(tmpid, current->stdid);//학번 저장
						strcpy(current->stdid, nextnode->stdid);//변경
						strcpy(nextnode->stdid, tmpid);//변경
						for(int k=1; k<cols; k++){//문제 점수 저장
							char *tmpQuestion;//문제 점수 변수
							tmpQuestion = malloc(strlen(current->questions[k])+1);//메모리 할당
							strcpy(tmpQuestion, current->questions[k]);//점수 저장
							strcpy(current->questions[k], nextnode->questions[k]);//변경
							strcpy(nextnode->questions[k], tmpQuestion);//변경
						}
					}
				}
				else{//오름차순
					if(cmp > 0.0){//변경경우
						char *tmpid;//학번 변수
						tmpid = malloc(strlen(current->stdid)+1);//메모리 할당
						strcpy(tmpid, current->stdid);//학번 저장
						strcpy(current->stdid, nextnode->stdid);//변경
						strcpy(nextnode->stdid, tmpid);//변경
						for(int k=1; k<cols; k++){//문제 점수 저장
							char *tmpQuestion;//문제 점수 변수
							tmpQuestion = malloc(strlen(current->questions[k])+1);//메모리 할당
							strcpy(tmpQuestion, current->questions[k]);//점수 저장
							strcpy(current->questions[k], nextnode->questions[k]);//변경
							strcpy(nextnode->questions[k], tmpQuestion);//변경
						}
					}
				}
				current = current->next;//이동
			}
		}
	}
    
    // write output file
    fp = fopen(filename, "w");
    if (fp == NULL) {//예외처리
        printf("Failed to open file: %s", filename);
        exit(1);
    }
	fprintf(fp, "%s\n", qname);//문제 번호 입력

	struct ssu_sort *current = header;//현제 노드 위치
	while(current->next != NULL){//저장
		fprintf(fp, "%s", current->stdid);//학번 저장
		for(int i=1; i<cols; i++){//학생별 점수 기입
			fprintf(fp, ",");//, 기준 저장
			fprintf(fp, "%s", current->questions[i]);//점수 저장
		}
		fprintf(fp, "\n");//널문차 출력
		current = current->next;//이동
	}
	fprintf(fp, "%s", current->stdid);//마지막 학번 저장
	for(int i=1; i<cols; i++){//마지막 문제들 점수 저장
		fprintf(fp, ",");//,기준
		fprintf(fp, "%s", current->questions[i]);//점수저장
	}

    fclose(fp);//닫음

    return ;//함수 종료
}

// 학생 id를 받아 해당 학생의 문제별 점수를 매기는 함수 
double score_student(int fd, char *id, int cScores[])
{
	int type;
	double result;
	double score = 0;
	int i;
	char tmp[2048];
	struct wrongTable *head = NULL;
	// size = 문제수
	int size = sizeof(score_table) / sizeof(score_table[0]);

	// 문제수 만큼 반복
	for(i = 0; i < size ; i++)
	{	
		// 해당 번호의 점수가 0이라면 break
		if(score_table[i].score == 0)
			break;
		// tmp변수에 stdDir 경로/학번/문제번호 => 학생의 제출한 답 파일까지의 경로를 저장 
		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname);

		// 해당 파일의 접근권한 확인
		if(access(tmp, F_OK) < 0)
			// 파일을 열지못하는 경우 false -> 0점처리
			result = false;
		else
		{	// scoretable에서 해당 문자의 확장자를 받아옴 -> -1을 return하는 경우는 무시
			if((type = get_file_type(score_table[i].qname)) < 0)
				continue;
			
			// 텍스트 파일인 경우
			if(type == TEXTFILE)
				// blank문제에 해당하는 채점 방식으로 채점 
				result = score_blank(id, score_table[i].qname);
			// C 파일인 경우
			else if(type == CFILE)
				// program문제에 해당하는 채점 방식으로 채점 
				result = cScores[i];
		}
		// 오답인 모든 경우에는 0점 처리
		if(result == false){
			//csv파일에 0점과 , 를 포함한 2byte작성
			write(fd, "0,", 2);
			add_question(&head, score_table[i].qname, score_table[i].score);

		}
		else{
			// 정답인 경우 
			if(result == true){
				score += score_table[i].score;
				sprintf(tmp, "%.2f,", score_table[i].score);
			}
			// warning으로 인해 -점수를 받은 경우 0.1점 감점 처리
			else if(result < 0){
				score = score + score_table[i].score + result;
				sprintf(tmp, "%.2f,", score_table[i].score + result);
			}
			// 해당 점수를 포함한 tmp변수를 file에 write
			write(fd, tmp, strlen(tmp));
		}
	}


	// 해당 학생 점수 매기기 끝내는 문구 출력
	// c와 P옵션이 동시에 들어온 경우 
	if (cOption && pOption){

		if ((cpCnt ==0)){
			printf("%s is finished.. score : %.2f\n", id, score);
			do_pOption(head);
		}
		else {
			int flag = 0;
			for(int k = 0; k <cpCnt; k++){
				if(!strcmp(id, iIDs[k])) {
					printf("%s is finished.. score : %.2f,  ", id, score); 
					do_pOption(head);
					flag = 1;
				}
			}
			if(flag == 0) printf("%s is finished..\n", id);
		}
	}
	//c옵션만 들어온 경우
	else if (cOption && !pOption) {
		//students인자의 idx가 cCnt보다 작고, 현재 id와 일치하는 id라인 경우 || cCnt가 0인경우 모든 학생 score출력
		if ((cCnt ==0)){
			printf("%s is finished.. score : %.2f\n", id, score); 
		}
		else {
			int flag = 0;
			for(int k = 0; k <cCnt; k++){
				if(!strcmp(id, iIDs[k])) {
					printf("%s is finished.. score : %.2f\n", id, score); 
					flag = 1;
				}
			}
			if(flag == 0) printf("%s is finished..\n", id);
		}
	}
	// pOption인 경우
	else if (!cOption && pOption) {
		//students인자의 idx가 cCnt보다 작고, 현재 id와 일치하는 id라인 경우 || pCnt가 0인경우 모든 학생 score출력

		if ((pCnt ==0)){
			printf("%s is finished.. ", id); 
			do_pOption(head);
		}
		else {
			int flag = 0;
			for(int k = 0; k <pCnt; k++){
				if(!strcmp(id, iIDs[k])) {
					printf("%s is finished.. ", id); 
					do_pOption(head);
					flag = 1;
				}
			}
			if(flag == 0) printf("%s is finished..\n", id);
		}
	}
	// 아무 옵션도 들어오지 않은 경우
	else printf("%s is finished..\n", id);


	// sum변수에 저장한 점수합 tmp에 저장 
	sprintf(tmp, "%.2f\n", score);
	// sum한 점수 file에 write
	write(fd, tmp, strlen(tmp));
	

	return score;
}
// scoretable의 첫번째 행 작성 
void write_first_row(int fd)
{
	int i;
	char tmp[BUFLEN];
	// 문제수 = size
	int size = sizeof(score_table) / sizeof(score_table[0]);
	// file descriptor에 첫번째 문자 ,(1byte)만큼 작성
	write(fd, ",", 1);
	// 문제수 만큼 반복
	for(i = 0; i < size; i++){
		// 점수가 0인경우에 break
		if(score_table[i].score == 0)
			break;
		// 문제번호와 구분자 ','를 포함하여 tmp에 저장
		sprintf(tmp, "%s,", score_table[i].qname);
		// 문제번호를 포함한 tmp변수를 file에 작성
		write(fd, tmp, strlen(tmp));
	}
	//마지막 열에 sum열 추가 (4byte)
	write(fd, "sum\n", 4);
}

//정답 획득 함수
char *get_answer(int fd, char *result)
{
	//문자 저장 변수
	char c;
	//index 변수
	int idx = 0;

	//메모리 초기화
	memset(result, 0, BUFLEN);
	//파일 읽는 반복문
	while(read(fd, &c, 1) > 0)
	{
		//:인경우 읽기 중지
		if(c == ':')
			//중지
			break;
		
		//결과 입력
		result[idx++] = c;
	}
	//결과의 마지막 개행에서 널문자로 변환
	if(result[strlen(result) - 1] == '\n')
		//변환
		result[strlen(result) - 1] = '\0';
	//결과 리턴
	return result;
}

//빈칸 문제 점수 계산 함수
int score_blank(char *id, char *filename)
{
	//토큰 변수
	char tokens[TOKEN_CNT][MINLEN];
	//루트 노드 생성
	node *std_root = NULL, *ans_root = NULL;
	//인덱스
	int idx;
	//임시 저장 변수
	char tmp[2048];
	//학생과 문제 정답
	char s_answer[BUFLEN], a_answer[BUFLEN];
	//문제 이름 저장 변수
	char qname[FILELEN];
	//학생, 문제 파일 디스크립터
	int fd_std, fd_ans;
	//결과
	int result = true;
	//세미콜론
	int has_semicolon = false;

	//메모리 공간을 특정 값으로 채움
	memset(qname, 0, sizeof(qname));
	//메모리 카피
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	//학생 답변 경로 생성
	sprintf(tmp, "%s/%s/%s", stuDir, id, filename);
	//학생 답변 오픈
	fd_std = open(tmp, O_RDONLY);
	//학생 정답을 s_answer에 복사
	strcpy(s_answer, get_answer(fd_std, s_answer));

	//정답이 없는 경우
	if(!strcmp(s_answer, "")){
		//파일 닫고
		close(fd_std);
		//false 리턴
		return false;
	}

	//괄호 정상 확인 ->정상이지 않은 경우
	if(!check_brackets(s_answer)){
		//파일 닫고 
		close(fd_std);
		//false return 
		return false;
	}

	//공백 제거 결과 카피
	strcpy(s_answer, ltrim(rtrim(s_answer)));

	//학생 정답 가장 끝 세미콜론인 경우
	if(s_answer[strlen(s_answer) - 1] == ';'){
		//세미콜론 보유 플레그 변경
		has_semicolon = true;
		//정답 가장 끝 널문자 변환
		s_answer[strlen(s_answer) - 1] = '\0';
	}

	//토큰 생성 오류 처리
	if(!make_tokens(s_answer, tokens)){
		//파일 닫음
		close(fd_std);
		//false 리턴
		return false;
	}

	//idx 0 초기화
	idx = 0;
	//트리 구조 생성
	std_root = make_tree(std_root, tokens, &idx, 0);

	//tmp 변수에 디렉토리 경로 설정
	sprintf(tmp, "%s/%s", ansDir, filename);
	//파일 오픈
	fd_ans = open(tmp, O_RDONLY);

	//반복문 실행
	while(1)
	{
		//ans 변수 초기화
		ans_root = NULL;
		//결과 plag true로 초기화
		result = true;

		//토큰 개수 만큼 초기화
		for(idx = 0; idx < TOKEN_CNT; idx++)
			memset(tokens[idx], 0, sizeof(tokens[idx]));

		//a_answer변수에 변수 카피
		strcpy(a_answer, get_answer(fd_ans, a_answer));

		//답이 공백이라면 멈춤
		if(!strcmp(a_answer, ""))
			//멈춤
			break;

		//공백 제거 뒤 결과 카피
		strcpy(a_answer, ltrim(rtrim(a_answer)));

		//세미콜론 보유 플레그 false인 경우
		if(has_semicolon == false){
			//뒤에 세미콜론이 존재힌다면
			if(a_answer[strlen(a_answer) -1] == ';')
				//진행
				continue;
		}
		//세미콜론 보유 플레그 true인 경우
		else if(has_semicolon == true)
		{
			//뒤에 세미콜론이 없는 경우
			if(a_answer[strlen(a_answer) - 1] != ';')
				//진행
				continue;
			//세미콜론이 있는 경우
			else
				//가장 뒤에 널문자로 변경
				a_answer[strlen(a_answer) - 1] = '\0';
		}

		//토큰 생성 오류 처리
		if(!make_tokens(a_answer, tokens))
			//반복문 진행
			continue;

		//인덱스 초기화
		idx = 0;
		//ans_root에 트리 생성
		ans_root = make_tree(ans_root, tokens, &idx, 0);

		//트리 비교
		compare_tree(std_root, ans_root, &result);

		//비교 결과 true인 경우
		if(result == true){
			//파일 닫음
			close(fd_std);
			//파일 닫음
			close(fd_ans);

			//루트가 널이 아닌경우
			if(std_root != NULL)
				//루트 널로 초기화
				free_node(std_root);
			//루트가 널이 아닌 경우
			if(ans_root != NULL)
				//루트 널로 초기화
				free_node(ans_root);
			//true 리턴
			return true;

		}
	}
	
	//파일 닫음
	close(fd_std);
	//파일 닫음
	close(fd_ans);

	//루트가 널이 아닌 경우
	if(std_root != NULL)
		//루트 널로 초기화
		free_node(std_root);
	//루트가 널이 아닌 경우
	if(ans_root != NULL)
		//루트 널로 초기화
		free_node(ans_root);

	//false 초기화
	return false;
}

//프로그램 문제 점수 계산 함수
double score_program(char *id, char *filename)
{
	//컴파일 변수 선언
	double compile;
	//결과 변수
	int result;

	compile = compile_program(id, filename);//프로그램 컴파일

	if(compile == ERROR || compile == false) //에러가 존재하거나 실패한 경우
		return false;//false 리턴
	
	result = execute_program(id, filename);//파일 실행 결과 비교

	if(!result)//실행 결과가 false인 경우
		return false;//false 반환

	if(compile < 0)//컴파일이 완료 되지 않은 경우
		return compile;//compile 반환

	return true;//프로그램의 컴파일 실행 결과 반환
}

//toption 인자 여부 함수
int is_thread(char *qname)
{
	int i;//반복문 변수
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]);//파일 크기 변수

	if(!strcmp(threadFiles[0],"")){//t에 인자가 아무것도 없는 경우 (전체)
		return true;
	}

	for(i = 0; i < size; i++){//해당하는 문제 찾음
		if(!strcmp(threadFiles[i], qname))//일치하는 경우
			return true;//true
	}
	return false;//일치 문제 없는 경우 false
}

//프로그램 컴파일 함수
double compile_program(char *id, char *filename)
{
	//파일 디스크립터 변수
	int fd;
	//경로변수 선언
	char tmp_f[2000], tmp_e[2048];
	//명령어
	char command[4096];
	//파일 이름
	char qname[FILELEN];
	//동일 파일 존재 확인 변수
	int isthread;
	//크기 변수
	off_t size;
	//결과 변수
	double result;

	//q이름 메모리 초기화
	memset(qname, 0, sizeof(qname));
	//qname 변수에 filename 카피
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));
	
	//동일한 이름 있는지 확인
	isthread = is_thread(qname);

	if(!isthread) {
		fprintf(stderr, "wrong input\n");
		exit(1);
	}

	//답 파일 경로 변수 
	sprintf(tmp_f, "%s/%s", ansDir, filename);
	//정답 실행 파일 경로 확인
	sprintf(tmp_e, "%s/%s.exe", ansDir, qname);

	//파일이 존재하며 t 옵션을 사용한 경우
	if(tOption && isthread)
		//실행 command 작성
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	//파일이 존재하지 않거나 t 옵션을 사용하지 않은 경우
	else
		// 실행command 작성
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	//에러 작성 파일 생성
	sprintf(tmp_e, "%s/%s_error.txt", ansDir, qname);
	//파일 디스크립터 생성
	fd = creat(tmp_e, 0666);

	//gcc 명령어 실행
	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END); //에러 파일 크기 저장
	close(fd); //파일 닫음
	unlink(tmp_e); //tmp_e 파일 삭제

	if(size > 0)//에러가 존재할 경우 false 리턴
		return false;//에러가 존재할 경우 false 리턴

	sprintf(tmp_f, "%s/%s/%s", stuDir, id, filename); //학생 파일 경로
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname); //학생 실행 경로 획득

	if(tOption && isthread) //toption이 존재하고 파일이 존재하는 경우
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f); //gcc 명령어 생성
	else //t옵션이 존재하지 않거나 파일이 존재하지 않는 경우
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);//gcc 명령어 생성

	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname); //에러 저장 파일 경로 생성
	fd = creat(tmp_f, 0666); //에러 저장 파일 생성

	redirection(command, fd, STDERR); //gcc 명령어 실행
	size = lseek(fd, 0, SEEK_END); //에러 저장 파일 크기 측정
	close(fd); //파일 닫음

	if(size > 0){//에러가 존재하는 경우
		if(eOption) //e옵션이 존재하는 경우
		{
			sprintf(tmp_e, "%s/%s", errorDir, id); //에러 저장 디렉토리 경로 생성
			if(access(tmp_e, F_OK) < 0) // 디렉토리 존재하지 않는 경우
				mkdir(tmp_e, 0755); //디렉토리 생성
			else{//디렉토리가 존재하는 경후 삭제후 재입력
				rmdirs(tmp_e);//삭제
				mkdir(tmp_e, 0755);//생성
			}


			sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname); //에러 저장 파일 경로 생성
			rename(tmp_f, tmp_e); //파일의 이름 변경

			result = check_error_warning(tmp_e);//워닝 개수 파악
		}
		else{//e옵션이 존재하지 않는 경우
			result = check_error_warning(tmp_f);//워닝 개수 파악
			unlink(tmp_f);//파일 삭제
		}

		return result;//결과 리턴
	}

	unlink(tmp_f);//파일 삭제
	return true;//true 리턴
}

//파일에서 에러 워닝 찾아 확인하는 함수
double check_error_warning(char *filename)
{
	FILE *fp; //파일 스트림 포인터 변수
	char tmp[BUFLEN]; //파일 내용 임시 저장 변수
	double warning = 0; // warning 개수 저장 변수

	if((fp = fopen(filename, "r")) == NULL){ //파일 오픈과 예외처리
		fprintf(stderr, "fopen error for %s\n", filename); //에외문 출력
		return false; //false 리턴
	}

	while(fscanf(fp, "%s", tmp) > 0){//파일에서 문자열 읽어온다
		if(!strcmp(tmp, "error:"))//에러 발견한 경우
			return ERROR;//ERROR 리턴
		else if(!strcmp(tmp, "warning:"))//warning 발견한 경우
			warning += WARNING;//warning 개수 추가
	}

	return warning;//warning 개수 리턴
}

//주어진 파일 싱행하고 실행 결과 비교 함수
int execute_program(char *id, char *filename)
{
	char std_fname[2048], ans_fname[2048]; //학생 파일 이름 정답 파일 이름
	char tmp[2048]; //실행 경로 저장 변수
	char qname[FILELEN]; //문제 이름 저장 변수
	time_t start, end;//시작 시작 종료 시간
	pid_t pid;//fork 번호
	int fd;//파일 디스크립터 번호

	memset(qname, 0, sizeof(qname));//qname 초기화
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));//qname에 이름 저장

	sprintf(ans_fname, "%s/%s.stdout", ansDir, qname);//정답 파일의 경로 생성
	if(access(ans_fname, F_OK)!=0){//실행 결과가 없는 경우만 실행
		fd = creat(ans_fname, 0666);//정답 파일 새성 

		sprintf(tmp, "%s/%s.exe", ansDir, qname);//실행 파일의 경로 임시 저장
		redirection(tmp, fd, STDOUT);//실행 파일 실행
		close(fd);//파일 닫음
	}

	sprintf(std_fname, "%s/%s/%s.stdout", stuDir, id, qname);//학생의 결과 파일 경로 생성
	fd = creat(std_fname, 0666);//학생의 결과 파일 생성

	sprintf(tmp, "%s/%s/%s.stdexe &", stuDir, id, qname);//학생의 실행 파일 경로 생성

	start = time(NULL);//시작 시간
	redirection(tmp, fd, STDOUT);//학생 실행 파일 백그라운드에서 실행, 결과 저장
	
	sprintf(tmp, "%s.stdexe", qname);//실행 저장 이름 생성
	while((pid = inBackground(tmp)) > 0){//백그라운드 실행 파일 존재하는 경우
		end = time(NULL);//종료 시간 초기화

		if(difftime(end, start) > OVER){//시작시간 종료시간이 5이상인 경우
			kill(pid, SIGKILL);//프로세스 종료
			close(fd);//파일 닫기
			return false;//false 반환
		}
	}

	close(fd);//파일 닫기

	return compare_resultfile(std_fname, ans_fname);//실행 결과 비교 반환
}

//백그라운드 동작 프로세스 확인 함수
pid_t inBackground(char *name)
{
	pid_t pid;//프로세스 번호
	char command[64];//명령어 저장 변수
	char tmp[64];//파일에서 읽어오는 변수 저장 뱐수
	int fd;//파일 디스크립터
	
	memset(tmp, 0, sizeof(tmp));//tmp 변수 초기화
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);//background.txt파일 열기

	sprintf(command, "ps | grep %s", name);//명령어 생성
	redirection(command, fd, STDOUT);//명령어 실행

	lseek(fd, 0, SEEK_SET);//파일의 가장 앞으로 offset 위치 이동
	read(fd, tmp, sizeof(tmp));//앞에서 부터 tmp 크기만큼 읽기

	if(!strcmp(tmp, "")){//실행중인 프로세스가 없는 경우
		unlink("background.txt");//background 파일 삭제
		close(fd);//fd 닫음
		return 0;//0 return
	}

	pid = atoi(strtok(tmp, " "));//실행중인 프로세스가 있다면 pid번호
	close(fd);//파일 닫음

	unlink("background.txt");//파일 삭제
	return pid;//pid 반환
}

//두개의 파일이 동일한지 확인하는 함수
int compare_resultfile(char *file1, char *file2)
{
	int fd1, fd2; //파일 디스크립터 1,2
	char c1, c2; //문자 1,2
	int len1, len2; //읽은 1,2

	fd1 = open(file1, O_RDONLY); //file1 open
	fd2 = open(file2, O_RDONLY); //file2 open

	while(1)//반복문 실행
	{
		while((len1 = read(fd1, &c1, 1)) > 0){//파일 읽는 것이 성공한 경우
			if(c1 == ' ') //빈칸인 경우
				continue;//반복 실행
			else //빈칸이 아닌 경우
				break; //반복문 종료
		}
		while((len2 = read(fd2, &c2, 1)) > 0){//파일 읽는 것 성공
			if(c2 == ' ') //빈칸의 경우
				continue;//반복 실행
			else //빈칸이 아닌 경우
				break;//반복문 종료
		}
		
		if(len1 == 0 && len2 == 0)//파일의 끝까지 읽은 경우
			break;//반복문 종료

		to_lower_case(&c1);//소문자 변환
		to_lower_case(&c2);//소문자 변환

		if(c1 != c2){//두 문자가 다른 경우
			close(fd1);//파일 닫음
			close(fd2);//파일 닫음
			return false;//false 반환
		}
	}
	close(fd1);//파일 닫음
	close(fd2);//파알 닫음
	return true;//true 반환
}

//gcc 명령어 실행 함수
void redirection(char *command, int new, int old)
{	
	int saved; //에러 파일 디스크립터의 저장 변수

	saved = dup(old); //에러 파일 디스크립터 저장
	dup2(new, old); //에러 파일 디스크립터 변경

	system(command); //gcc 명령어 실행

	dup2(saved, old); // 에러 파일 디스크립터 복구
	close(saved); //에러 파일 닫음
}

int get_file_type(char *filename)
{
	// '.' 문자가 처음으로 등장하는 지점을 포인트 => 확장자를 나타냄
	char *extension = strrchr(filename, '.');

	// 확장자가 txt인 경우 
	if(!strcmp(extension, ".txt"))
		return TEXTFILE;
	// 확장자가 c인 경우
	else if (!strcmp(extension, ".c"))
		return CFILE;
	// 확장자가 csv인 경우
	else if (!strcmp(extension, ".csv"))
		return CSV;
	// 위세개가 아닌경우 -1
	else
		return -1;
}

//다렉토리 삭제 함수
void rmdirs(const char *path)
{
	//디렉토리 저장을 위한 dirent 구조체 변수
	struct dirent *dirp;
	//파일의 정보 저장을 위한 stat 구조체 변수
	struct stat statbuf;
	//DIR 스트리밍 변수
	DIR *dp;
	//경로 저장을 위한 변수
	char tmp[BUFLEN];
	
	//디렉토리 오픈 + 예외처리
	if((dp = opendir(path)) == NULL)
		return;

	//디렉토리 내부 read
	while((dirp = readdir(dp)) != NULL)
	{
		//본인과 상위 디렉토리 무시
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		//하위 디렉토리, 파일 경로 생성
		sprintf(tmp, "%s/%s", path, dirp->d_name);

		//stat구조체 변수 생성과 예외처리
		if(lstat(tmp, &statbuf) == -1)
			continue;

		//디렉토리인 경우 재귀적 반복
		if(S_ISDIR(statbuf.st_mode))
			//함수 실행
			rmdirs(tmp);
		//파일인 경우
		else
			//파일 unlink 통한 삭제
			unlink(tmp);
	}
	//디렉토리 close
	closedir(dp);
	//디렉토리 삭제
	rmdir(path);
}

//대소문자 변환 함수
void to_lower_case(char *c)
{
	//대문자인 경우 소문자로 변환
	if(*c >= 'A' && *c <= 'Z')
		//소문자 변환
		*c = *c + 32;
}
// usage를 출력하는 함수
//usage 출력 함수
void print_usage()
{
	//usage 출력
	printf("Usage : ssu_score <STUDENTDIR> <TRUEDIR> [OPTION]\n");//ssu_score 실행 방법
	printf("Option : \n");//option
	printf(" -m                modify question's score\n");//-m option
	printf(" -e <DIRNAME>      print error on 'DIRNAME/ID/qname_error.txt' file \n");//-e option
	printf(" -t <QNAMES>       compile QNAME.C with -lpthread option\n");//-toption
	printf(" -t <IDS>          print ID's wrong questions\n");//-toption
	printf(" -h                print usage\n");//-hoption
}


//sort 구조체 추가 함수
void add_Node(struct ssu_sort **head, struct ssu_sort node, int cols){
	struct ssu_sort *tmp = malloc(sizeof(struct ssu_sort));//구조체 생성
	tmp->stdid = malloc(strlen(node.stdid)+1);//메모리 할당
	strcpy(tmp->stdid, node.stdid);//추가할 노드 정보 복사
	tmp->next = NULL;//초기화
	for(int k=1; k<cols; k++){//추가할 노드 정보 복사
		tmp->questions[k] = malloc(strlen(node.questions[k]));//메모리 할당
		strcpy(tmp->questions[k], node.questions[k]);//복사
	}
	if(*head == NULL){//처음 추가하는 경우
		*head = tmp;//추가
	}
	else {//기존 노드 존재
		struct ssu_sort *current = *head;//현재 위치
		while(current->next != NULL){//마지막 위치 탐색
			current = current->next;//이동
		}
		current->next = tmp;//추가
	}

	return;
}

//p option 구조체 추가 함수
void add_question(struct wrongTable **head, char *qname, double score)
{
	struct wrongTable *nextnode = malloc(sizeof(struct wrongTable));//구조체 생성
	strcpy(nextnode->qname, qname);//정보 복사
	nextnode->qscore = score;//정보 복사
	nextnode->next = NULL;//초기화
	if(*head == NULL){//처음 추가 경우
		*head = nextnode;//추가
	}
	else {
		struct wrongTable *current = *head;//현재 위치
		while(current->next != NULL){//마지막 위치 탐색
			current = current->next;//이동
		}
		current->next = nextnode;//추가
	}

	return;
}