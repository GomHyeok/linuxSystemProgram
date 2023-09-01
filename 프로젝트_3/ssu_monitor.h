#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + NAME_MAX + 1))
#define MAX_COMMAND_LENGTH 1048

//파일 정보 저장 구조체
struct file_info {
    char name[PATH_MAX];
    time_t mtime;
    struct file_info *next;
};

void free_list(struct file_info **head);//링크드 리스트의 메모리를 해제하는 함수
void input_node(char *path, struct file_info **head, time_t mtime);//링크드리스트에 노드를 삽입하는 함수
void scan_directory(const char *path, struct file_info **head);//디렉토리 내부를 탐색하는 함수
int compare(struct file_info **head, struct file_info **newhead, char *logPath);//이전과의 내용을 비교하고 변경사항을 log파일에 입력하는 함수
void remove_pid_from_file(int pid);//delete 명령어 실행시 monitor_list.txt의 해당 pid 삭제 함수
void monitor_directory(const char *path, char *logPath, int interval);//add 명령어 실행시 실행 함수 -> 디렉토리를 모니터링함
void ssu_tree(char *path, int space);//트리 출력 함수
void ssu_help();//Usage 출력 함수
int checkDigit(char *path);//입력받은 옵션의 값이 정수인지 확인한는 함수
int checkPath (char *path, char *cmp);//monitor_list.txt에 존재하는 디렉토리의 하위 디렉토리와 입력받은 경로 비교 함수
