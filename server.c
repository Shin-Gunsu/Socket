
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include<signal.h>
#define MAX_MSG_SIZE 4100

#define MAX_LENGTH 50
char readbuf[MAX_MSG_SIZE]; //읽어들인 문자열 버퍼
char writebuf[MAX_MSG_SIZE];   //보낼 문자열 버퍼

// 연결 리스트 노드 구조체
typedef struct Node {
    char data[50]; // 데이터
    int fd; // 파일 디스크립터
    struct Node* next; // 다음 노드를 가리키는 포인터
} Node;

// 원형 연결리스트 구조체 정의
typedef struct CL {
    Node* head; // 헤드 노드를 가리키는 포인터
    int size; // 원형 연결리스트의 크기
} CList;

// 원형 연결리스트 생성 함수
CList* create_CList() {
    CList* cl = (CList*)malloc(sizeof(CList)); // 원형 연결리스트 메모리 할당
    cl->head = NULL; // 헤드 노드 초기화
    cl->size = 0; // 크기 초기화
    return cl; // 원형 연결리스트 반환
}

// 원형 연결리스트 삽입 함수
void insert_CList(CList* cl, char* data, int fd) {
    Node* new_Node = (Node*)malloc(sizeof(Node)); // 새로운 노드 메모리 할당
    strcpy(new_Node->data, data); // 데이터 복사
    new_Node->fd = fd; // 파일 디스크립터 복사
    if (cl->head == NULL) { // 원형 연결리스트가 비어있는 경우
        new_Node->next = new_Node; // 새로운 노드가 자기 자신을 가리키도록 함
        cl->head = new_Node; // 헤드 노드를 새로운 노드로 설정
    }
    else { // 원형 연결리스트가 비어있지 않은 경우
        Node* last = cl->head; // 마지막 노드를 찾기 위한 임시 노드
        while (last->next != cl->head) { // 마지막 노드까지 반복
            last = last->next; // 다음 노드로 이동
        }
        new_Node->next = cl->head; // 새로운 노드의 다음 노드를 헤드 노드로 설정
        last->next = new_Node; // 마지막 노드의 다음 노드를 새로운 노드로 설정
        cl->head = new_Node; // 헤드 노드를 새로운 노드로 설정
    }
    cl->size++; // 원형 연결리스트의 크기 증가
}

// 원형 연결리스트 삭제 함수
void delete_CList(CList* cl, char* data) {
    if (cl->head == NULL) { // 원형 연결리스트가 비어있는 경우
        printf("접속한 클라이언트 없음.\n");
        return;
    }
    Node* curr = cl->head; // 현재 노드를 헤드 노드로 설정
    Node* prev = NULL; // 이전 노드를 NULL로 설정
    do {
        if (strcmp(curr->data, data) == 0) { // 현재 노드의 데이터가 삭제할 데이터와 일치하는 경우
            if (prev == NULL) { // 이전 노드가 없는 경우
                if (curr->next == curr) { // 현재 노드가 유일한 노드인 경우
                    cl->head = NULL; // 헤드 노드를 NULL로 설정
                }
                else { // 현재 노드가 유일한 노드가 아닌 경우
                    Node* last = cl->head; // 마지막 노드를 찾기 위한 임시 노드
                    while (last->next != cl->head) { // 마지막 노드까지 반복
                        last = last->next; // 다음 노드로 이동
                    }
                    cl->head = curr->next; // 헤드 노드를 현재 노드의 다음 노드로 설정
                    last->next = cl->head; // 마지막 노드의 다음 노드를 헤드 노드로 설정
                }
            }
            else { // 이전 노드가 있는 경우
                prev->next = curr->next; // 이전 노드의 다음 노드를 현재 노드의 다음 노드로 설정
            }
            free(curr); // 현재 노드 메모리 해제
            cl->size--; // 원형 연결리스트의 크기 감소
            printf("%s 클라이언트를 삭제.\n", data);
            return;
        }
        prev = curr; // 이전 노드를 현재 노드로 설정
        curr = curr->next; // 현재 노드를 다음 노드로 설정
    } while (curr != cl->head); // 헤드 노드까지 반복
    printf("%s 클라이언트를 찾을 수 없음.\n", data); // 삭제할 데이터가 없는 경우
}

// 원형 연결리스트 검색 함수
Node* search_CList(CList* cl, char* data) {
    if (cl->head == NULL) { // 원형 연결리스트가 비어있는 경우
        printf("원형 연결리스트가 비어있습니다.\n");
        return NULL;
    }
    Node* curr = cl->head; // 현재 노드를 헤드 노드로 설정
    do {
        if (strcmp(curr->data, data) == 0) { // 현재 노드의 데이터가 검색할 데이터와 일치하는 경우
            printf("%s 데이터를 찾았습니다.\n", data);
            return curr; // 현재 노드 반환
        }
        curr = curr->next; // 현재 노드를 다음 노드로 설정
    } while (curr != cl->head); // 헤드 노드까지 반복
    printf("%s 데이터를 찾을 수 없습니다.\n", data); // 검색할 데이터가 없는 경우
    return NULL;
}

int server_fd;      //서버 파일디스크럽터
CList *fifo_list= NULL; //접속한 클라언트들의 정보를 저장할 원형 연결리스트
void sigintHandler(int sig_num) {   //서버 종료시 리스트를 돌며 클라이언트 fifo파일들 삭제
    Node* tmp;
    tmp=fifo_list->head;
    if(tmp!=NULL){
        do{
            unlink(tmp->data);
            tmp = tmp->next;
        } while (tmp != fifo_list->head);

        if(close (server_fd) == -1){
            perror ("서버 디스크럽터");
        }
    }
    exit(1);
}

int main() {
    signal(SIGINT, sigintHandler);  //시그널 핸들러
    fifo_list=create_CList();       //원형연결리스트 생성
    int fd;                         //클라이언트 파일 디스크럽터
    char fifo_name[30];             //fifo파일 명
    char flag[2];                   //읽어온 메시지의 상태를 확인하는 플래그, fifo연결을 위한 메시지는 0, 일반 메시지는 1, 클라이언크 종료시 -1
    char send_fifo[30];             //메시지를 보낸 클라이언트 명
    char *tok_str;                  
    Node* tmp=NULL;                 //Node임시 변수

    mkfifo("server_fifo", 0666);    // FIFO 생성


    printf("서버 실행 중...\n");
    server_fd = open("server_fifo", O_RDONLY);  //server_fifo열기
    while (1) {
        if(read(server_fd, readbuf, sizeof(readbuf))<=0){   //server_fifo를 읽어와 readbuf에 저장
            perror("server");
        }
        //printf("전체 메시지 : %s\n" ,readbuf);
        tok_str=strtok(readbuf," ");	        //flag추출
        strcpy(flag,tok_str);

        if(strcmp(flag,"0")==0){                //flag = 0 ,클라이언트와 연결
            tok_str=strtok(NULL," ");           //메시지를 보낸 클라이언트 이름 저장
            strcpy(fifo_name,tok_str);          //클라이언트 이름 저장
            printf("%s님이 접속하셨습니다.\n",fifo_name);  //접속 확인 메시지
            insert_CList(fifo_list,fifo_name,-1);   //클라이언트 리스트에 삽입
            //printf("리스트에 삽입 %s\n",fifo_list->head->data);
        }else if(strcmp(flag,"1")==0){          //flag=1 , 메시지를 받고 리스트에 있는 클라이언트 들에게 출력
            tok_str=strtok(NULL," ");           //메시지를 보낸 클라이언트 이름 저장
            strcpy(send_fifo,tok_str);
            //printf("fifo : %s\n",send_fifo);
            strcpy(readbuf,strtok(NULL,""));    //readbuf에 플래그와 클라이언트명을 자르고 남은 메시지 저장
            //printf("받은 메시지 : %s\n",readbuf);
            strcpy(writebuf,send_fifo);         //writebuf에 파일명 복사
            strcat(writebuf," ");               //공백 삽입
            strcat(writebuf,readbuf);           //메시지 삽입
            //printf("다른 클라이언트에게 보낼 메시지:%s\n",writebuf);    
            tmp=fifo_list->head;                //클라이언트 리스트의 시작부분
            if(tmp!=NULL){                      //접속한 클라이언트가 있을시 리스트를 돌면서 메시지 보냄
                do {
                    //printf("%s에게 메시지 보냄\n",tmp->data);   
                    if(tmp->fd=open(tmp->data,O_WRONLY)==-1){   //클라이언트 리스트에 저장된 클라이언트의 fifo파일 열기
                        perror("메시지보내기 오류\n");
                        break;
                    }
                    if(write(tmp->fd,writebuf,strlen (writebuf)+1)<=0){ //리스트를 돌며 해당fifo파일에 메시지 송신, write시 -1이나 0이 나온다면 클라이언트와 접속종료
                        printf("%s와 연결이 끊어졌습니다.\n",tmp->data );
                        close(tmp->fd);
                        unlink(tmp->data);
                        delete_CList(fifo_list,tmp->data);
                    }else{
                        close(tmp->fd);
                        tmp = tmp->next;
                    }
                    
                } while (tmp != fifo_list->head && tmp!=NULL);
            }
        }else{      //flag = -1클라이언트가 입터럽트로 인해 종료시
            tok_str=strtok(NULL," ");
            strcpy(send_fifo,tok_str);
            printf("끊김 메시지 %s\n",send_fifo);
            tmp=search_CList(fifo_list,send_fifo);  //보낸 클라이언트 추출
            if(tmp!=NULL){                          //종료된 클라이언트를 리스트에서 삭제
                printf("%s 종료\n",tmp->data);
                close(tmp->fd);
                delete_CList(fifo_list,tmp->data);
            }
        }
    }
    return 0;
}
