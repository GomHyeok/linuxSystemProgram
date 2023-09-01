#include <sys/types.h>

#ifndef MAIN_H_
#define MAIN_H_

//상수값 설정
#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif
#ifndef STDOUT
	#define STDOUT 1
#endif
#ifndef STDERR
	#define STDERR 2
#endif
#ifndef TEXTFILE
	#define TEXTFILE 3
#endif
#ifndef CFILE
	#define CFILE 4
#endif
#ifndef OVER
	#define OVER 5
#endif
#ifndef CSV
	#define CSV 6
#endif
#ifndef ASC
	#define ASC 7
#endif
#ifndef DESC
	#define DESC 8
#endif
#ifndef STDID
	#define STDID 9
#endif
#ifndef SCORE
	#define SCORE 10
#endif
#ifndef WARNING
	#define WARNING -0.1
#endif
#ifndef ERROR
	#define ERROR 0
#endif

// #define FILELEN 128
#define FILELEN 255
#define BUFLEN 1024
#define SNUM 100 // students 최대 값
#define QNUM 100 // question 최대 값
#define ARGNUM 5 // argnum 최대값 
#define IDMAX 100//stdid의 최대값

//scoreTable 구조체 선언 
struct ssu_scoreTable{
	char qname[FILELEN];
	double score;
};

//sort 구조체
struct ssu_sort {
	char *stdid;//학번
	char *questions[QNUM];//문제별 점수
	struct ssu_sort *next;//포인터
};

//poption 구조체
struct wrongTable {
	char qname[FILELEN];//문제 번호
	double qscore;//문제 점수
	struct wrongTable *next;//포인터
};

void ssu_score(int argc, char *argv[]);
int check_option(int argc, char *argv[]);
void print_usage();

void score_students();
double score_student(int fd, char *id, int cScores[]);
void write_first_row(int fd);

char *get_answer(int fd, char *result);
int score_blank(char *id, char *filename);
double score_program(char *id, char *filename);
double compile_program(char *id, char *filename);
int execute_program(char *id, char *filname);
pid_t inBackground(char *name);
double check_error_warning(char *filename);
int compare_resultfile(char *file1, char *file2);

void do_pOption();
void do_mOption();
void do_sOption();

int is_thread(char *qname);
void redirection(char *command, int newfd, int oldfd);
int get_file_type(char *filename);
void rmdirs(const char *path);
void to_lower_case(char *c);

void set_scoreTable(char *ansDir);
void read_scoreTable(char *path);
void make_scoreTable(char *ansDir);
void write_scoreTable(char *filename);
void set_idTable(char *stuDir);
int get_create_type();

void sort_idTable(int size);
void sort_scoreTable(int size);
void get_qname_number(char *qname, int *num1, int *num2);

void add_Node(struct ssu_sort **head, struct ssu_sort node, int cols);
void add_question(struct wrongTable **head, char *qname, double score);

#endif
