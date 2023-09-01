#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "blank.h"

//데이터 타입
char datatype[DATATYPE_SIZE][MINLEN] = {"int", "char", "double", "float", "long"
			, "short", "ushort", "FILE", "DIR","pid"
			,"key_t", "ssize_t", "mode_t", "ino_t", "dev_t"
			, "nlink_t", "uid_t", "gid_t", "time_t", "blksize_t"
			, "blkcnt_t", "pid_t", "pthread_mutex_t", "pthread_cond_t", "pthread_t"
			, "void", "size_t", "unsigned", "sigset_t", "sigjmp_buf"
			, "rlim_t", "jmp_buf", "sig_atomic_t", "clock_t", "struct"};


//연산자 타입
operator_precedence operators[OPERATOR_CNT] = {
	{"(", 0}, {")", 0}
	,{"->", 1}	
	,{"*", 4}	,{"/", 3}	,{"%", 2}	
	,{"+", 6}	,{"-", 5}	
	,{"<", 7}	,{"<=", 7}	,{">", 7}	,{">=", 7}
	,{"==", 8}	,{"!=", 8}
	,{"&", 9}
	,{"^", 10}
	,{"|", 11}
	,{"&&", 12}
	,{"||", 13}
	,{"=", 14}	,{"+=", 14}	,{"-=", 14}	,{"&=", 14}	,{"|=", 14}
};

//트리 비교 함수
void compare_tree(node *root1,  node *root2, int *result)
{
	node *tmp;

	if(root1 == NULL || root2 == NULL){//노드 비워져 있는 경우 false
		*result = false;
		return;
	}

	if(!strcmp(root1->name, "<") || !strcmp(root1->name, ">") || !strcmp(root1->name, "<=") || !strcmp(root1->name, ">=")){//비교 연산자 존재의 경우
		if(strcmp(root1->name, root2->name) != 0){//같지 않은 경우

			if(!strncmp(root2->name, "<", 1))//부등호 변경
			strcpy(root2->name, ">");

			else if(!strncmp(root2->name, ">", 1))//부등호 변경
				strcpy(root2->name, "<");

			else if(!strncmp(root2->name, "<=", 2))//부등호 변경
				strcpy(root2->name, ">=");

			else if(!strncmp(root2->name, ">=", 2))//부등호 변경
				strcpy(root2->name, "<=");


			root2 = change_sibling(root2);//변경 적용
		}
	}

	if(strcmp(root1->name, root2->name) != 0){//같은 경우 false
		*result = false;
		return;
	}

	if((root1->child_head != NULL && root2->child_head == NULL)//root의 자식이 없는경우 false
		|| (root1->child_head == NULL && root2->child_head != NULL)){
		*result = false;
		return;
	}

	else if(root1->child_head != NULL){//root1의 자식이 있고
		if(get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)){//형제 개수 다른 경우 false
			*result = false;
			return;
		}

		if(!strcmp(root1->name, "==") || !strcmp(root1->name, "!="))//등호 연산인 경우
		{
			compare_tree(root1->child_head, root2->child_head, result);//재귀호출

			if(*result == false)//false인 경우
			{
				*result = true;//result 변경
				root2 = change_sibling(root2);//형제 변경
				compare_tree(root1->child_head, root2->child_head, result);//재귀호출
			}
		}
		else if(!strcmp(root1->name, "+") || !strcmp(root1->name, "*")//덧셈 곱샘 오얼, 앤드 연산
				|| !strcmp(root1->name, "|") || !strcmp(root1->name, "&")
				|| !strcmp(root1->name, "||") || !strcmp(root1->name, "&&"))
		{
			if(get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)){//형제 개수 다른경우 false
				*result = false;
				return;
			}

			tmp = root2->child_head;//임시 변수 할당

			while(tmp->prev != NULL)//가장 앞으로 이동
				tmp = tmp->prev;

			while(tmp != NULL)//뒤로 이동하며
			{
				compare_tree(root1->child_head, tmp, result);//재취 호출
			
				if(*result == true)//true인 경우 멈춤
					break;
				else{//아닌경우 
					if(tmp->next != NULL)//result 초기화
						*result = true;
					tmp = tmp->next;//이동
				}
			}
		}
		else{//그 외위 경우
			compare_tree(root1->child_head, root2->child_head, result);//재귀호출
		}
	}	


	if(root1->next != NULL){//root의 다음이 있는 경우

		if(get_sibling_cnt(root1) != get_sibling_cnt(root2)){//형제 개수 다르면 false
			*result = false;
			return;
		}

		if(*result == true)//true인 경우
		{
			tmp = get_operator(root1);//operator 획득
	
			if(!strcmp(tmp->name, "+") || !strcmp(tmp->name, "*")//덧셈 곱샘 오얼, 앤드 연산
					|| !strcmp(tmp->name, "|") || !strcmp(tmp->name, "&")
					|| !strcmp(tmp->name, "||") || !strcmp(tmp->name, "&&"))
			{	
				tmp = root2;//임시 변수 할당
	
				while(tmp->prev != NULL)//앞의 노드 존재 할 경우
					tmp = tmp->prev;//이동

				while(tmp != NULL)//재귀 호출
				{
					compare_tree(root1->next, tmp, result);//재귀호출

					if(*result == true)//true인 경우 멈춤
						break;
					else{//아닌 경우 이동
						if(tmp->next != NULL)
							*result = true;//초기화
						tmp = tmp->next;//이동
					}
				}
			}

			else//재귀호출
				compare_tree(root1->next, root2->next, result);
		}
	}
}

//token 분석 함수
int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN])
{
	char *start, *end;
	char tmp[BUFLEN];
	char *op = "(),;><=!|&^/+-*\""; 
	int row = 0;
	int i;
 	int isPointer;
	int lcount, rcount;
	int p_str;
	
	clear_tokens(tokens);//토큰 초기화

	start = str;//변수 초기화
	
	if(is_typeStatement(str) == 0)//문자열의 데이터 탑입인지 확인하고 기존 데이터 타입과 일치하는지 확인 
		return false; //일치하지 않는 경우
	
	while(1)
	{
		if((end = strpbrk(start, op)) == NULL)
			break;

		if(start == end){//시작과 끝이 같은 경우

			if(!strncmp(start, "--", 2) || !strncmp(start, "++", 2)){// ++-- 연산자의 경우
				if(!strncmp(start, "++++", 4)||!strncmp(start,"----",4))//잘못된 구문 찾음
					return false;

				// ex) ++a
				if(is_character(*ltrim(start + 2))){//앞에 위치하는 겨우
					if(row > 0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]))//잘못된 경우
						return false; //ex) ++a++

					end = strpbrk(start + 2, op);//현재 토큰 처리
					if(end == NULL)//없는 경우
						end = &str[strlen(str)];//할당
					while(start < end) {//끝이 더 큰 경우
						if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))//공백으로 시작하며 토큰이 char인경우
							return false;
						else if(*start != ' ')//공백이 아닌
							strncat(tokens[row], start, 1);//토큰에 결합
						start++;	
					}
				}
				// ex) a++
				else if(row>0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){// 뒤에 오는 경우
					if(strstr(tokens[row - 1], "++") != NULL || strstr(tokens[row - 1], "--") != NULL)	//잘못된 경우
						return false;

					memset(tmp, 0, sizeof(tmp));//이전 토큰에 ++, -- 추가
					strncpy(tmp, start, 2);//복사
					strcat(tokens[row - 1], tmp);//병합
					start += 2;//위치 이동
					row--;//감소
				}
				else{// 일반적인 경우
					memset(tmp, 0, sizeof(tmp));//메모리 초기화
					strncpy(tmp, start, 2);//복사
					strcat(tokens[row], tmp);//병합
					start += 2;//위치 이동
				}
			}

			else if(!strncmp(start, "==", 2) || !strncmp(start, "!=", 2) || !strncmp(start, "<=", 2)
				|| !strncmp(start, ">=", 2) || !strncmp(start, "||", 2) || !strncmp(start, "&&", 2) 
				|| !strncmp(start, "&=", 2) || !strncmp(start, "^=", 2) || !strncmp(start, "!=", 2) 
				|| !strncmp(start, "|=", 2) || !strncmp(start, "+=", 2)	|| !strncmp(start, "-=", 2) 
				|| !strncmp(start, "*=", 2) || !strncmp(start, "/=", 2)){//논리 연산자의 경우

				strncpy(tokens[row], start, 2);//토큰 복사
				start += 2;//위치 이동
			}
			else if(!strncmp(start, "->", 2))//이동 연산자 경우
			{
				end = strpbrk(start + 2, op);//뒤 2개 삭제

				if(end == NULL)//없는경우
					end = &str[strlen(str)];//할당

				while(start < end){//end가 더 큰 경우
					if(*start != ' ')//공백이 아닌 경우
						strncat(tokens[row - 1], start, 1);//병합
					start++;//이동
				}
				row--;
			}
			else if(*end == '&')//&연산자 경우
			{
				// ex) &a (address)
				if(row == 0 || (strpbrk(tokens[row - 1], op) != NULL)){
					end = strpbrk(start + 1, op);//뒤에 삭제
					if(end == NULL)//없는 경우
						end = &str[strlen(str)];//할당
					
					strncat(tokens[row], start, 1);//병합
					start++;//위치 이동

					while(start < end){//end가 더 큰 경우
						if(*(start - 1) == ' ' && tokens[row][strlen(tokens[row]) - 1] != '&')//공백과 함께 사용
							return false;
						else if(*start != ' ')//공백 없는 경우
							strncat(tokens[row], start, 1);//결합
						start++;
					}
				}
				// ex) a & b (bit)
				else{
					strncpy(tokens[row], start, 1);//병합
					start += 1;
				}
				
			}
		  	else if(*end == '*')//곱셈 연산
			{
				isPointer=0;//포인터인 경우 플래그

				if(row > 0)
				{
					//ex) char** (pointer)
					for(i = 0; i < DATATYPE_SIZE; i++) {//토큰 순환
						if(strstr(tokens[row - 1], datatype[i]) != NULL){//토큰과 타입 분리
							strcat(tokens[row - 1], "*");//병합
							start += 1;	//위치 이동
							isPointer = 1;//포인터인 경우
							break;
						}
					}
					if(isPointer == 1)//포인터인 경우
						continue;
					if(*(start+1) !=0)//끝이 아닌 경우
						end = start + 1;//끝지점 이동

					// ex) a * **b (multiply then pointer)
					if(row>1 && !strcmp(tokens[row - 2], "*") && (all_star(tokens[row - 1]) == 1)){
						strncat(tokens[row - 1], start, end - start);
						row--;
					}
					
					// ex) a*b(multiply)
					else if(is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]) == 1){ 
						strncat(tokens[row], start, end - start);   
					}

					// ex) ,*b (pointer)
					else if(strpbrk(tokens[row - 1], op) != NULL){		
						strncat(tokens[row] , start, end - start); 
							
					}
					else
						strncat(tokens[row], start, end - start);

					start += (end - start);
				}

			 	else if(row == 0)//행의 시작인 경우
				{
					if((end = strpbrk(start + 1, op)) == NULL){//초기화
						strncat(tokens[row], start, 1);
						start += 1;
					}
					else{
						while(start < end){//end가 더 큰 경우
							if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))//공백 존재
								return false;
							else if(*start != ' ')//공백 없음
								strncat(tokens[row], start, 1);//병합
							start++;	
						}
						if(all_star(tokens[row]))//모두 *인경우
							row--;
						
					}
				}
			}
			else if(*end == '(') //(시작
			{
				lcount = 0;//괄호 개수
				rcount = 0;//괄호 개수
				if(row>0 && (strcmp(tokens[row - 1],"&") == 0 || strcmp(tokens[row - 1], "*") == 0)){//포인터 연산 경우
					while(*(end + lcount + 1) == '(')//(인 경우
						lcount++;//개수 증가
					start += lcount;//위치 이동

					end = strpbrk(start + 1, ")");//)하나 삭제

					if(end == NULL)//)남은게 없는 경우 
						return false;
					else{
						while(*(end + rcount +1) == ')')//)개수 
							rcount++;//증가
						end += rcount;//위치 이동

						if(lcount != rcount)//괄호 개수 다른 경우
							return false;

						if( (row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1])) || row == 1){ //row가 1 이상 그리고 모두 캐릭터가 아니거나  또는 row가 1인경우
							strncat(tokens[row - 1], start + 1, end - start - rcount - 1);//병합
							row--;//감소
							start = end + 1;//위치 이동
						}
						else{
							strncat(tokens[row], start, 1);//병합
							start += 1;//위치이동
						}
					}
						
				}
				else{
					strncat(tokens[row], start, 1);//병합
					start += 1;//위치이동
				}

			}
			else if(*end == '\"') //\연산의 경우
			{
				end = strpbrk(start + 1, "\"");//삭제
				
				if(end == NULL)//나누는 수 없는 경우
					return false;

				else{
					strncat(tokens[row], start, end - start + 1);//병합
					start = end + 1;//위치 이동
				}

			}

			else{
				// ex) a++ ++ +b
				if(row > 0 && !strcmp(tokens[row - 1], "++"))
					return false;

				// ex) a-- -- -b
				if(row > 0 && !strcmp(tokens[row - 1], "--"))
					return false;
	
				strncat(tokens[row], start, 1);
				start += 1;
				
				// ex) -a or a, -b
				if(!strcmp(tokens[row], "-") || !strcmp(tokens[row], "+") || !strcmp(tokens[row], "--") || !strcmp(tokens[row], "++")){


					// ex) -a or -a+b
					if(row == 0)
						row--;

					// ex) a+b = -c
					else if(!is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){
					
						if(strstr(tokens[row - 1], "++") == NULL && strstr(tokens[row - 1], "--") == NULL)
							row--;
					}
				}
			}
		}
		else{ 
			//모두 '*'이거나 캐리터가 이닌 경우
			if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))   
				row--;				
			//row가 1인경우 감소
			if(all_star(tokens[row - 1]) && row == 1)   
				row--;	

			for(i = 0; i < end - start; i++){//start에서 end까지 문자를 토큰에 ㅊ가
				if(i > 0 && *(start + i) == '.'){//.인경우
					strncat(tokens[row], start + i, 1);//병합

					while( *(start + i +1) == ' ' && i< end - start )//공백 무시
						i++; 
				}
				else if(start[i] == ' '){//공백인 경우
					while(start[i] == ' ')//공백 무시
						i++;
					break;
				}
				else
					strncat(tokens[row], start + i, 1);//병합
			}

			if(start[0] == ' '){//공백인 경우
				start += i;//공백 무시
				continue;
			}
			start += i;
		}
			
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));//좌측 공백 제거

		 if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1]) 
				&& (is_typeStatement(tokens[row - 1]) == 2 
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.' ) ){//다양한 조건

			if(row > 1 && strcmp(tokens[row - 2],"(") == 0)//우측 괄호
			{
				if(strcmp(tokens[row - 1], "struct") != 0 && strcmp(tokens[row - 1],"unsigned") != 0)//구조체 또는 unsgined
					return false;
			}
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {//문자면서 구조체 또는 unsgined
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)	
					return false;
			}
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){//특정 타입과 토큰의 경우 구조체 또는 unsgined
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
			
		}

		if((row == 0 && !strcmp(tokens[row], "gcc")) ){//gcc는 새로운 문자열로 변경
			clear_tokens(tokens);//gcc 지움
			strcpy(tokens[0], str);	//새로운 문자열 대입
			return 1;
		} 

		row++;
	}

	if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))//모두 별이며 row>1이고 캐릭터가 아닌 경우 
		row--;//감소
	if(all_star(tokens[row - 1]) && row == 1)   //모두 별이며 row=1인경우
		row--;	

	for(i = 0; i < strlen(start); i++)   //문자열 탐색
	{
		if(start[i] == ' ')  //공백
		{
			while(start[i] == ' ')//무시
				i++;
			if(start[0]==' ') {//공백 피함
				start += i;
				i = 0;
			}
			else
				row++;//row 증가
			
			i--;
		} 
		else
		{
			strncat(tokens[row], start + i, 1);//병합
			if( start[i] == '.' && i<strlen(start)){//. 이며 길이 만족
				while(start[i + 1] == ' ' && i < strlen(start))//공백인 경우 다음이 무시
					i++;

			}
		}
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));// 토큰에 공백 제거 병합

		if(!strcmp(tokens[row], "lpthread") && row > 0 && !strcmp(tokens[row - 1], "-")){//gcc option 
			strcat(tokens[row - 1], tokens[row]);//병합
			memset(tokens[row], 0, sizeof(tokens[row]));//병합
			row--;
		}
	 	else if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1]) 
				&& (is_typeStatement(tokens[row - 1]) == 2 
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.') ){//다양한 조건 확인
			
			if(row > 1 && strcmp(tokens[row-2],"(") == 0)//좌측 괄호 구조체 또는 unsgined
			{
				if(strcmp(tokens[row-1], "struct") != 0 && strcmp(tokens[row-1], "unsigned") != 0)
					return false;
			}
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {//캐릭터면서 구조체 또는 unsgined
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)	
					return false;
			}
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){//특정 타입이면서 구조체 또는 unsgined
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
		} 
	}


	if(row > 0)
	{

		// ex) #include <sys/types.h>
		if(strcmp(tokens[0], "#include") == 0 || strcmp(tokens[0], "include") == 0 || strcmp(tokens[0], "struct") == 0){ 
			clear_tokens(tokens); 
			strcpy(tokens[0], remove_extraspace(str)); 
		}
	}

	if(is_typeStatement(tokens[0]) == 2 || strstr(tokens[0], "extern") != NULL){
		for(i = 1; i < TOKEN_CNT; i++){
			if(strcmp(tokens[i],"") == 0)  
				break;		       

			if(i != TOKEN_CNT -1 )
				strcat(tokens[0], " ");
			strcat(tokens[0], tokens[i]);
			memset(tokens[i], 0, sizeof(tokens[i]));
		}
	}
	
	//change ( ' char ' )' a  ->  (char)a
	while((p_str = find_typeSpecifier(tokens)) != -1){ 
		if(!reset_tokens(p_str, tokens))
			return false;
	}

	//change sizeof ' ( ' record ' ) '-> sizeof(record)
	while((p_str = find_typeSpecifier2(tokens)) != -1){  
		if(!reset_tokens(p_str, tokens))
			return false;
	}
	
	return true;
}

//노드의 트리 생성 함수
node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses)
{
	node *cur = root;//현재 위치
	node *new;//새로운 노드
	node *operator;//오퍼레이터
	int fstart;
	int i;

	while(1)	
	{
		if(strcmp(tokens[*idx], "") == 0)//토큰이 존재하지 않는 경우
			break;
	
		if(!strcmp(tokens[*idx], ")"))//오른쪽 괄호의 경우
			return get_root(cur);

		else if(!strcmp(tokens[*idx], ","))//,의 경우
			return get_root(cur);

		else if(!strcmp(tokens[*idx], "("))//왼쪽 괄호의 경우
		{
			// function()
			if(*idx > 0 && !is_operator(tokens[*idx - 1]) && strcmp(tokens[*idx - 1], ",") != 0){//오퍼레이터인 경우
				fstart = true;

				while(1)
				{
					*idx += 1;

					if(!strcmp(tokens[*idx], ")"))//우측 괄호의 경우 종료
						break;
					
					new = make_tree(NULL, tokens, idx, parentheses + 1);//재귀적 호출
					
					if(new != NULL){//노드 이동 함수
						if(fstart == true){//오퍼레이터의 경우
							cur->child_head = new;//이동
							new->parent = cur;//이동
	
							fstart = false;//falsef로 변경
						}
						else{
							cur->next = new;//추가
							new->prev = cur;//추가
						}

						cur = new;
					}

					if(!strcmp(tokens[*idx], ")"))//우측 괄호 종료 조건
						break;
				}
			}
			else{//어떤 것에도 해당하지 않는 경우(논리 연산)
				*idx += 1;
	
				new = make_tree(NULL, tokens, idx, parentheses + 1);//트리 생성 재귀적 호출

				if(cur == NULL)//현제 위치가 없는 경우
					cur = new;//새 노드 추가

				else if(!strcmp(new->name, cur->name)){
					if(!strcmp(new->name, "|") || !strcmp(new->name, "||") 
						|| !strcmp(new->name, "&") || !strcmp(new->name, "&&"))//논리 연산의 경우
					{
						cur = get_last_child(cur);//마지막 부분 이동

						if(new->child_head != NULL){//부모가 존재하는 경우
							new = new->child_head;//이동

							new->parent->child_head = NULL;//초기화
							new->parent = NULL;//초기화
							new->prev = cur;//변수 입력
							cur->next = new;//변수 입력
						}
					}
					else if(!strcmp(new->name, "+") || !strcmp(new->name, "*"))//일반 연산의 경우
					{
						i = 0;

						while(1)
						{
							if(!strcmp(tokens[*idx + i], ""))//마지막 인 경우
								break;

							if(is_operator(tokens[*idx + i]) && strcmp(tokens[*idx + i], ")") != 0)//우측 괄호 경우
								break;

							i++;
						}
						
						if(get_precedence(tokens[*idx + i]) < get_precedence(new->name))
						{//현재 노드이 마지막 자식으로 이동
							cur = get_last_child(cur);
							//새로운 연결 생성
							cur->next = new;
							//연결
							new->prev = cur;
							cur = new;
						}
						else
						{
							//마지막 자식으로 이동
							cur = get_last_child(cur);
							//새 노드에 자식이 있는 경우
							if(new->child_head != NULL){
								new = new->child_head;//첫 자식으로 이동

								new->parent->child_head = NULL;//기존 부모와의 연결 끊는다.
								new->parent = NULL;//연결 끊는다
								new->prev = cur;//새 노드와 연결
								cur->next = new;//새 노드와 연결
							}
						}
					}
					else{
						cur = get_last_child(cur);//마지막 자식으로 이동
						cur->next = new;//새로운 연결
						new->prev = cur;//새로운 연결
						cur = new;//새로운 연결
					}
				}
	
				else
				{
					cur = get_last_child(cur);//마지막 자식으로 이동

					cur->next = new;//연결
					new->prev = cur;//연결
	
					cur = new;//연결
				}
			}
		}
		else if(is_operator(tokens[*idx]))
		{
			if(!strcmp(tokens[*idx], "||") || !strcmp(tokens[*idx], "&&")
					|| !strcmp(tokens[*idx], "|") || !strcmp(tokens[*idx], "&") 
					|| !strcmp(tokens[*idx], "+") || !strcmp(tokens[*idx], "*"))
			{//현재 노드의 이름이 연산자이며 토큰과 같다면
				if(is_operator(cur->name) == true && !strcmp(cur->name, tokens[*idx]))
					operator = cur;
		
				else
				{//다른경우
					new = create_node(tokens[*idx], parentheses);//새 노드를 생성하고 우선순위 높은 노드 찾는다
					operator = get_most_high_precedence_node(cur, new);//우선순위

					if(operator->parent == NULL && operator->prev == NULL){//연산자 우선순위가 가장 높은경우

						if(get_precedence(operator->name) < get_precedence(new->name)){//새로운 것이 더 큰 경우
							cur = insert_node(operator, new);// 추가
						}

						else if(get_precedence(operator->name) > get_precedence(new->name))//작은 경우
						{
							if(operator->child_head != NULL){//자식 있는 경우
								operator = get_last_child(operator);//마지막 자식 이동
								cur = insert_node(operator, new);//추가
							}
						}
						else
						{//같은 경우
							operator = cur;//현제 위치
	
							while(1)
							{
								if(is_operator(operator->name) == true && !strcmp(operator->name, tokens[*idx]))//우선순위가 현제 토큰과 다른경우 멈춤
									break;
						
								if(operator->prev != NULL)//비어있지 않는 경우
									operator = operator->prev;//이동
								else
									break;//빈 경우 멈춤
							}

							if(strcmp(operator->name, tokens[*idx]) != 0)//같은 경우
								operator = operator->parent;//부모로 이동

							if(operator != NULL){//오퍼랜드 존재
								if(!strcmp(operator->name, tokens[*idx]))//토큰 같은 경우
									cur = operator;//현제 위치 조정
							}
						}
					}

					else//그 외의 경우
						cur = insert_node(operator, new);//노드 추가
				}

			}
			else
			{
				new = create_node(tokens[*idx], parentheses);//새로운 노드 생성

				if(cur == NULL)//현재 비어있는 경우
					cur = new;//추가

				else
				{//아닌경우
					operator = get_most_high_precedence_node(cur, new);//가장 높은 우선순위 찾음

					if(operator->parentheses > new->parentheses)//더 낮은 경우
						cur = insert_node(operator, new);//삽입

					else if(operator->parent == NULL && operator->prev ==  NULL){//가장 높은 경우
					
						if(get_precedence(operator->name) > get_precedence(new->name))//새로운 것이 작은 경우
						{
							if(operator->child_head != NULL){
	
								operator = get_last_child(operator);//가장 마지막 자식으로 이동
								cur = insert_node(operator, new);//삽입
							}
						}
					
						else	
							cur = insert_node(operator, new);//아닌경우 현재 위치에 추가
					}
	
					else//같은 경우
						cur = insert_node(operator, new);//현재 위치 추가
				}
			}
		}
		else 
		{
			new = create_node(tokens[*idx], parentheses);//새로운 노드 생성

			if(cur == NULL)//현재 위치 비어있는 경우 추가
				cur = new;//추가

			else if(cur->child_head == NULL){//자식 없는 경우
				cur->child_head = new;//연결
				new->parent = cur;//연결

				cur = new;//위치 이동
			}
			else{

				cur = get_last_child(cur);//마지막 자식 이동

				cur->next = new;//연결
				new->prev = cur;//연결

				cur = new;//현재 위치 이동
			}
		}

		*idx += 1;//개수 추가
	}

	return get_root(cur);//루트로 이동후 리턴
}

node *change_sibling(node *parent)//sibling 변경
{
	node *tmp;
	
	tmp = parent->child_head;//첫 자식 위치

	parent->child_head = parent->child_head->next;//이동
	parent->child_head->parent = parent;//변경
	parent->child_head->prev = NULL;//초기화

	parent->child_head->next = tmp;//스왑
	parent->child_head->next->prev = parent->child_head;//병경
	parent->child_head->next->next = NULL;//초기화
	parent->child_head->next->parent = NULL;//초기화

	return parent;
}

//연산자 노드 생성 함수
node *create_node(char *name, int parentheses)
{
	node *new;//새로운 노드

	new = (node *)malloc(sizeof(node));//할당
	new->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));//할당
	strcpy(new->name, name);//이름 입력

	new->parentheses = parentheses;//초기화
	new->parent = NULL;//초기화
	new->child_head = NULL;//초기화
	new->prev = NULL;//초기화
	new->next = NULL;//초기화

	return new;
}

//연산자 우선순위 반환 함수
int get_precedence(char *op)
{
	int i;

	for(i = 2; i < OPERATOR_CNT; i++){//연산자 탐색
		if(!strcmp(operators[i].operator, op))//같은 것 있으면 같은것 우선순위 리턴
			return operators[i].precedence;
	}
	//없으면 존재하지 않는 연산자
	return false;
}

int is_operator(char *op)
{
	int i;

	for(i = 0; i < OPERATOR_CNT; i++)
	{
		if(operators[i].operator == NULL)//연산자 존재하지 않는 경우
			break;
		if(!strcmp(operators[i].operator, op)){//같은 경우
			return true;
		}
	}
	//같지 않은 경우
	return false;
}

void print(node *cur)//노드 정보 출력 함수
{
	if(cur->child_head != NULL){//자식 존재하는 경우
		print(cur->child_head);//자식 정보 출력
		printf("\n");
	}

	if(cur->next != NULL){//다음 노드 존재하는 경우
		print(cur->next);//다음 노드 출력
		printf("\t");
	}
	printf("%s", cur->name);//이름 출력
}

node *get_operator(node *cur)//부모 노드 탐색 후 리턴
{
	if(cur == NULL)
		return cur;

	if(cur->prev != NULL)//가장 앞으로 이동
		while(cur->prev != NULL)//가장 앞으로 이동
			cur = cur->prev;//이동

	return cur->parent;//부모 리턴
}

node *get_root(node *cur) //root 노드 탐색
{
	if(cur == NULL)//본인이 루트인 경우
		return cur;

	while(cur->prev != NULL)//가장 앞으로 이동
		cur = cur->prev;

	if(cur->parent != NULL)//부모 존재시 부모의 루트 
		cur = get_root(cur->parent);

	return cur;
}

node *get_high_precedence_node(node *cur, node *new)// 가장 높은 우선순위 가지는 연산자 탐색 노드
{
	if(is_operator(cur->name))// 연산자가 존재하는 경우
		if(get_precedence(cur->name) < get_precedence(new->name))//우선순위 비교
			return cur;

	if(cur->prev != NULL){//현제 노드의 가장 아픙로 이동
		while(cur->prev != NULL){
			cur = cur->prev;
			
			return get_high_precedence_node(cur, new);//각 형재 노드와 비교 결과
		}


		if(cur->parent != NULL)//현재 노드의 부모 노드와 비교
			return get_high_precedence_node(cur->parent, new);
	}

	if(cur->parent == NULL)//현재 노드가 부모 노드인 경우
		return cur;

	return cur;
}

node *get_most_high_precedence_node(node *cur, node *new)//가장 높은 우선순위 가지는 연산자 
{
	node *operator = get_high_precedence_node(cur, new);//현재 가장 높은 우선순위 가지는 노드
	node *saved_operator = operator;//저장

	while(1)
	{
		if(saved_operator->parent == NULL)//부모인 경우
			break;

		if(saved_operator->prev != NULL)//부모가 아닌 경우 비교
			operator = get_high_precedence_node(saved_operator->prev, new);

		else if(saved_operator->parent != NULL)//부모와 비교
			operator = get_high_precedence_node(saved_operator->parent, new);

		saved_operator = operator;//가장 높은 우선순위 변경
	}
	
	return saved_operator;//출력
}

node *insert_node(node *old, node *new)//링크드리스트에 노드 추가
{
	if(old->prev != NULL){//기존 노드 뒤에 삽입
		new->prev = old->prev;
		old->prev->next = new;
		old->prev = NULL;
	}

	new->child_head = old;//서로 연결
	old->parent = new;//연결

	return new;
}

node *get_last_child(node *cur)//가장 뒤에 있는 자식 노드 탐색
{
	if(cur->child_head != NULL)//자식노드 존재하는 경우
		cur = cur->child_head;//현재 위치 이동

	while(cur->next != NULL)
		cur = cur->next;//가장 뒤로 이동

	return cur;//리턴
}
//형제 개수 획득
int get_sibling_cnt(node *cur)
{
	int i = 0;

	while(cur->prev != NULL)//시작위치 가장 앞설정
		cur = cur->prev;

	while(cur->next != NULL){//개수 파악
		cur = cur->next;//이동
		i++;//추가
	}

	return i;
}

void free_node(node *cur)//할당된 노드 메모리 해제 함수
{
	if(cur->child_head != NULL)//자식 노드 존재하는 경우
		free_node(cur->child_head);//자식 노드 해제

	if(cur->next != NULL)//자신이 마지막인 경우
		free_node(cur->next);//해제

	if(cur != NULL){//노듸 안의 메모리 해제
		cur->prev = NULL;//헤제
		cur->next = NULL;//헤제
		cur->parent = NULL;//헤제
		cur->child_head = NULL;//헤제
		free(cur);//헤제
	}
}


int is_character(char c)//char 형 판별 함수
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');//char 형 판별
}

//type을 나타내는 경우
int is_typeStatement(char *str)
{ 
	char *start;
	char str2[BUFLEN] = {0}; 
	char tmp[BUFLEN] = {0}; 
	char tmp2[BUFLEN] = {0}; 
	int i;	 
	
	start = str;//대입
	strncpy(str2,str,strlen(str));//변수 복사
	remove_space(str2);//삭제

	while(start[0] == ' ')//공백의 경우
		start += 1;//위치 이동

	if(strstr(str2, "gcc") != NULL)//gcc를 기준으로 자름
	{
		strncpy(tmp2, start, strlen("gcc"));//tmp2에 저장
		if(strcmp(tmp2,"gcc") != 0)//gcc 저장시 return 0
			return 0;
		else//아니면 return 2
			return 2;
	}
	
	for(i = 0; i < DATATYPE_SIZE; i++)//탐색
	{
		if(strstr(str2,datatype[i]) != NULL)//데이터 타입 존재하는 경우
		{	
			strncpy(tmp, str2, strlen(datatype[i]));//대이터 타입 복사
			strncpy(tmp2, start, strlen(datatype[i]));//데이터 타입 복사
			
			if(strcmp(tmp, datatype[i]) == 0){//일차하는 경우
				if(strcmp(tmp, tmp2) != 0)//비교시 일차하는 경우
					return 0;  
				else//일치하지 않는 경우
					return 2;
					
			}
		}

	}
	return 1;

}

int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]) //토큰 타입 확인 함수
{
	int i, j;

	for(i = 0; i < TOKEN_CNT; i++)//모든 토큰 순회
	{
		for(j = 0; j < DATATYPE_SIZE; j++)//모든 데이터 타입 순회
		{
			if(strstr(tokens[i], datatype[j]) != NULL && i > 0)//토큰과 데이터 타입중 하나와 일치
			{
				if(!strcmp(tokens[i - 1], "(") && !strcmp(tokens[i + 1], ")") //괄호의 경우 면서
						&& (tokens[i + 2][0] == '&' || tokens[i + 2][0] == '*' 
							|| tokens[i + 2][0] == ')' || tokens[i + 2][0] == '(' 
							|| tokens[i + 2][0] == '-' || tokens[i + 2][0] == '+' 
							|| is_character(tokens[i + 2][0])))//연산자인 경우 
					return i;//i 리턴
			}
		}
	}
	return -1;//일치하는 것이 없는 경우
}

int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]) 
{
    int i, j;

   
    for(i = 0; i < TOKEN_CNT; i++)//모든 토큰 순회
    {
        for(j = 0; j < DATATYPE_SIZE; j++)//모든 데이터 타비 순회
        {
            if(!strcmp(tokens[i], "struct") && (i+1) <= TOKEN_CNT && is_character(tokens[i + 1][strlen(tokens[i + 1]) - 1]))  //토큰이 구조체이며 다음 토큰의 마지막 문자가 문자인 경우
                    return i;//인덱스 반환
        }
    }
    return -1;
}

int all_star(char *str)//모든 문자열이 '*'로 이루어졌느지 판단하는 함수
{
	int i;
	int length= strlen(str);//입력 문자열 길이
	
 	if(length == 0)	//길이가 0인경우 0 리턴
		return 0;
	
	for(i = 0; i < length; i++)//모든 문자열 순회
		if(str[i] != '*')//문자가 '*'이 아닌 경우 0 반환
			return 0;
	return 1;

}

int all_character(char *str)//모든 문자열이 문자로 이루어졌느지 판단하는 함수(0~9,a~Z)
{
	int i;

	for(i = 0; i < strlen(str); i++)//모든 문자열 탐색
		if(is_character(str[i]))//문자 여부 판단
			return 1;//문자가 아닌 것이 있는 경우
	return 0;
	
}

int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]) 
{
	int i;
	int j = start - 1;
	int lcount = 0, rcount = 0;
	int sub_lcount = 0, sub_rcount = 0;

	if(start > -1){//토큰이 구조체인 경우 
		if(!strcmp(tokens[start], "struct")) {		
			strcat(tokens[start], " ");//토큰에 공백 추가
			strcat(tokens[start], tokens[start+1]);//토큰 병합	     

			for(i = start + 1; i < TOKEN_CNT - 1; i++){//모든 토큰 한칸씩 앞으로 이동
				strcpy(tokens[i], tokens[i + 1]);//복사
				memset(tokens[i + 1], 0, sizeof(tokens[0]));//이동
			}
		}

		else if(!strcmp(tokens[start], "unsigned") && strcmp(tokens[start+1], ")") != 0) {	//토큰이 unsinged이며 다음 토큰이우측 괄호가 아닌 경우	
			strcat(tokens[start], " ");//공백 추가
			strcat(tokens[start], tokens[start + 1]);//병합     
			strcat(tokens[start], tokens[start + 2]);//병합

			for(i = start + 1; i < TOKEN_CNT - 1; i++){//모든 토큰 앞으로 이동
				strcpy(tokens[i], tokens[i + 1]);//복사
				memset(tokens[i + 1], 0, sizeof(tokens[0]));//이동
			}
		}

		j = start + 1;//우측 괄호 찾아서 rcount 증가  
		while(!strcmp(tokens[j], ")")){//우측 괄호인 경우
				rcount ++;//증가
				if(j==TOKEN_CNT)//마지막인 경우
					break;
				j++;//위치 이동
		}
	
		j = start - 1;//좌측 괄호 개수 lcount 증가
		while(!strcmp(tokens[j], "(")){//좌측 괄호인 경우
        	        lcount ++;//증가
                	if(j == 0)//가장 앞인 경우
                        break;
               		j--;//앞으로 이동
		}
		if( (j!=0 && is_character(tokens[j][strlen(tokens[j])-1]) ) || j==0)//모두 문자인 경우
			lcount = rcount;//괄호 문제 없게 만듬

		if(lcount != rcount )//괄호 문제 있는 경우
			return false;//false 반환

		if( (start - lcount) >0 && !strcmp(tokens[start - lcount - 1], "sizeof")){//sizeof가 왼쪽 괄호 앞인경우
			return true; //true 반환
		}
		
		else if((!strcmp(tokens[start], "unsigned") || !strcmp(tokens[start], "struct")) && strcmp(tokens[start+1], ")")) {		//구조체거며 우측 괄호 없거나 unsinged 인 경우
			strcat(tokens[start - lcount], tokens[start]);//토큰 병합
			strcat(tokens[start - lcount], tokens[start + 1]);//토큰 병합
			strcpy(tokens[start - lcount + 1], tokens[start + rcount]);//토큰 병하
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount; i++) {//토큰 순회
				strcpy(tokens[i], tokens[i + lcount + rcount]);//토큰 이동
				memset(tokens[i + lcount + rcount], 0, sizeof(tokens[0]));//토큰 이동
			}


		}
 		else{
			if(tokens[start + 2][0] == '('){//시작지점 +2가 (인 경우
				j = start + 2;//왼쪽 괄혹 개수 
				while(!strcmp(tokens[j], "(")){//왼쪽 괄호인 경우
					sub_lcount++;//개수 증가
					j++;//이동
				} 	
				if(!strcmp(tokens[j + 1],")")){//우측 괄호 경우
					j = j + 1;//우측 괄호 개수 
					while(!strcmp(tokens[j], ")")){//우측 괄호
						sub_rcount++;//개수 증가
						j++;//증가
					}
				}
				else //우측 괄호 아닌 경우 false
					return false;

				if(sub_lcount != sub_rcount)//괄호 개수 잘못된 경우
					return false;//false 반환
				
				strcpy(tokens[start + 2], tokens[start + 2 + sub_lcount]);//토큰 변경
				for(int i = start + 3; i<TOKEN_CNT; i++)//토큰 순회
					memset(tokens[i], 0, sizeof(tokens[0]));//토큰 변경

			}
			strcat(tokens[start - lcount], tokens[start]);//토큰 병합
			strcat(tokens[start - lcount], tokens[start + 1]);//토큰 병합
			strcat(tokens[start - lcount], tokens[start + rcount + 1]);//토큰 병합
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount -1; i++) {//토큰 순회
				strcpy(tokens[i], tokens[i + lcount + rcount +1]);//토큰 복사
				memset(tokens[i + lcount + rcount + 1], 0, sizeof(tokens[0]));//토큰 이동

			}
		}
	}
	return true;
}

//토큰 메모리 세팅
void clear_tokens(char tokens[TOKEN_CNT][MINLEN])
{
	//반복문 변수
	int i;
	//반복하며 메모리 세팅
	for(i = 0; i < TOKEN_CNT; i++)
		//메모리 세팅
		memset(tokens[i], 0, sizeof(tokens[i]));
}

//오른쪽 공백 결과 제외 함수
char *rtrim(char *_str)
{
	//임시 저장 변수
	char tmp[BUFLEN];
	//가장 끝자리 변수
	char *end;

	//str 임시 저장
	strcpy(tmp, _str);
	//가장 끝으로 이동
	end = tmp + strlen(tmp) - 1;
	//공백인 경우 오른쪽 이동
	while(end != _str && isspace(*end))
		//이동
		--end;
	//가장 끝 널문자 추가
	*(end + 1) = '\0';
	//결과 복사
	_str = tmp;
	//결과 리턴
	return _str;
}

//문자열 시작 부분의 공백 제외 함수
char *ltrim(char *_str)
{
	//_str 검사 위한 포인터 뱐스
	char *start = _str;

	//공백인 경우 이동
	while(*start != '\0' && isspace(*start))
		//왼쪽으로 한칸 이동
		++start;
	//이동 결과 전달
	_str = start;
	//이동 결과 리턴
	return _str;
}

char* remove_extraspace(char *str) //주어진 문자열의 연속되는 여분의 공백을 제거하여 반환한다.
{
	int i;
	char *str2 = (char*)malloc(sizeof(char) * BUFLEN);
	char *start, *end;
	char temp[BUFLEN] = "";//임시 문자열
	int position;

	if(strstr(str,"include<")!=NULL){//include<가 있는 경우 공백 추가하여 include <로 변경
		start = str;//문자열 복사
		end = strpbrk(str, "<");//끝 삭제
		position = end - start;//위치 찾음
	
		strncat(temp, str, position);//해당 위치에 문자열 추가
		strcat(temp, " "); // 공백 추가
		strncat(temp, str + position, strlen(str) - position + 1);//괄호 모양 병합

		str = temp;// 문자열 초기화
	}
	
	for(i = 0; i < strlen(str); i++)// 주어진 문자열 탐색
	{
		if(str[i] ==' ')//공백이 ㄴ경우
		{
			if(i == 0 && str[0] ==' ')//연속 공백인 경우
				while(str[i + 1] == ' ')//공백 끝까지 이동
					i++;	
			else{//아닌 경우
				if(i > 0 && str[i - 1] != ' ')//뒤가 공백이 아닌 경우
					str2[strlen(str2)] = str[i];//문자열 길이 위치에 복사
				while(str[i + 1] == ' ')//공백 무시
					i++;
			} 
		}
		else//공백 존재하지 않는 경우
			str2[strlen(str2)] = str[i];//복사
	}

	return str2;//공백 제거 문자열 반환
}



void remove_space(char *str)//문자열의 모든 공백 삭제 함수
{
	char* i = str;
	char* j = str;
	
	while(*j != 0)//널을 만날 때 까지
	{
		*i = *j++;//i 변수 변경
		if(*i != ' ')//공백 무시
			i++;
	}
	*i = 0;//삭제
}

int check_brackets(char *str)//주어진 문자열의 괄호의 개수를 확인하고 개수 일치 여부 확인
{
	char *start = str;//문자열 검사 포인터
	int lcount = 0, rcount = 0;//좌우 괄호 개수 초기화
	
	while(1){
		if((start = strpbrk(start, "()")) != NULL){//괄호 찾을 때 까지 이동
			if(*(start) == '(')//좌측 괄호인경우
				lcount++;//좌측 괄호 개수 증가
			else//우측 괄호인 경우
				rcount++;//우측 괄호 개수 증가

			start += 1; //위치 이동
		}
		else//더이상 괄호 없는 경우
			break;
	}

	if(lcount != rcount)//괄호 일치하지 않음
		return 0;
	else //일치하는 경우
		return 1;
}

int get_token_cnt(char tokens[TOKEN_CNT][MINLEN])//주어진 토큰의 배열에서 빈 문자열 만나기 전까지의 모든 토큰 개수 반환
{
	int i;
	
	for(i = 0; i < TOKEN_CNT; i++)//토큰 순회
		if(!strcmp(tokens[i], ""))//빈 토큰 만나는 경우
			break;

	return i;//해당 인텍스 반환
}
