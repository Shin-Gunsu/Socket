
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h> 
#include<signal.h>
#define MAX_MSG_SIZE 4096

int sendflag = 0;
char my_fifo_name[30];  //내 fifo파일 명
char writebuf1[MAX_MSG_SIZE];   //출력 버퍼1
char writebuf2[MAX_MSG_SIZE];   //출력 버퍼2
char readbuf[MAX_MSG_SIZE];     //입력 버퍼1
char readbuf2[MAX_MSG_SIZE];    //입력 버퍼2
int fd;                 //파일디스크럽터
int fd_server;          //서버파일 디스크럽터

void *send_msg(void *arg) {     //송신 함수

    while (1) {
        strcpy(writebuf2, "1 ");    //어떤 메시지인지 판별하기 위한 flag변수 1
        strcat(writebuf2, my_fifo_name);    //내 fifo파일명 삽입
        strcat(writebuf2, " ");             //공백삽입
        fgets(writebuf1, sizeof(writebuf1), stdin); //메시지를 입력받아 writebuf1에 저장
        strcat(writebuf2, writebuf1);       //writebuf2에 writebuf1 삽입
        write(fd_server, writebuf2, strlen(writebuf2) + 1); //서버에 메시지 송신
    }
    return NULL;
}

void *receive_msg(void *arg) {      //수신함수

    char *tok;
    char send_fifo[30];

    while (1) {
        if ((fd = open(my_fifo_name, O_RDONLY)) == -1) {    //fifo 읽어온다.
            perror("open: client");
            exit(1);
        }   
        read(fd, readbuf, sizeof(readbuf));     //읽어온 문자열을 readbuf에 저장
        //printf("받은 메시지 %s",readbuf);
        tok=strtok(readbuf," ");                //송신자, 메시지를 잘름
        strcpy(send_fifo,tok);                  //송신자를 send_fifo에 저장
        strcpy(readbuf2,strtok(NULL,""));       //메시지를 readbuf2에 저장
        if(strcmp(send_fifo,my_fifo_name)!=0){  //송신자가 자신이라면 메시지를 출력하지 않음
            printf("\n%s: %s",send_fifo,readbuf2);  //송신자와 메시지 출력
        }
        close(fd);

    }
    return NULL;
}

void sigintHandler(int sig_num) {   //인터럽트로 인한 종료시 서버에게 끊어짐 메시지 송신

    char str[100];
    strcpy(str,"-1");       //flag=-1 
    strcat(str," ");
    strcat(str,my_fifo_name);   
    //printf("\n%s\n",str);
    if ((fd_server = open("server_fifo", O_WRONLY)) == -1) { //서버 fifo열기
        perror("open: server fifo");
    }else{
        write(fd_server, str, strlen(str) + 1);     //서버에 메시지 송신
    }
    exit(1);
}

int main() {
    signal(SIGINT, sigintHandler);  //시그널 핸들러 등록
    pthread_t send_thread, receive_thread;  //스레드를 사용하여 메시지를 입력과 출력을 병렬로 수행

    sprintf(my_fifo_name, "client_fifo_%d", getpid());  //my_fifo_name에 클라이언트fifo파일 입력
    
    mkfifo(my_fifo_name, 0666);     //fifo파일 생성
    printf("%s 접속...\n", my_fifo_name);
    strcpy(writebuf2, "0 ");        //최초 연결 flag 설정
    strcat(writebuf2, my_fifo_name);    //fifo파일 명 삽입
    if ((fd_server = open("server_fifo", O_WRONLY)) == -1) {    //서버 연결
        perror("open: server fifo");
        exit(1);
    }
    write(fd_server, writebuf2, strlen(writebuf2) + 1);     //서버에 연결 요청 메시지 송신


    // 스레드 생성
    pthread_create(&send_thread, NULL, send_msg, NULL);
    pthread_create(&receive_thread, NULL, receive_msg, NULL);

    // 스레드가 종료될 때까지 대기
    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);
    return 0;
}
