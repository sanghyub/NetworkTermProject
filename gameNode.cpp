#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#define BUF_SIZE 20

int socketFD,pub_sock,x=0,y=0;
pthread_mutex_t mutx= PTHREAD_MUTEX_INITIALIZER;

typedef struct PACKET{
    char pubBuffer[BUF_SIZE];
    char subBuffer[BUF_SIZE];
}PACKET;


void MoveCursor(int X, int Y){   //move cursor to x,y
 printf("\033[%d;%dH", Y+1, X+1);
}


void alarmHandler(int signal){  //when signal is alarm,  x-1;
    pthread_mutex_lock(&mutx);
    MoveCursor(x, y);
    printf(" ");
    x--;
    if(x<0) x=80; //when 'o' meets left border
    MoveCursor(x, y);
    printf("o\n");
    alarm(1);  //signal alarm
    pthread_mutex_unlock(&mutx);

}

void *subscribe(void *arg){
    char direction;
    struct sockaddr_in sub_addr;
    int str_len=0;
    printf("Waiting for Publisher\n");

    while(str_len=read(socketFD,(void*)&sub_addr,sizeof(sub_addr))==0);
    printf("I got Address :%d\n", ntohs(sub_addr.sin_port));

    int sub_sock=socket(PF_INET,SOCK_STREAM,0);
    sub_addr.sin_family=AF_INET;
    inet_aton("127.0.0.1", (struct in_addr*) &sub_addr.sin_addr.s_addr);
    sub_addr.sin_port=sub_addr.sin_port/3;

    while(connect(sub_sock,(struct sockaddr*)&sub_addr,sizeof(sub_addr))==-1);
    printf("connect success!!\n");
    fprintf(stdout, "\033[2J");  //clear terminal window
    fflush(stdout);

    while(1){

    while(read(sub_sock,(void*)&direction, sizeof(direction))==0);

    pthread_mutex_lock(&mutx);
    MoveCursor(x, y);
    pthread_mutex_unlock(&mutx);
    alarm(1);
    if(direction=='l'){   //when 'o' go left
        pthread_mutex_lock(&mutx);
        printf(" ");
        x--;
        if(x<0) x=80; //when 'o' meets left border
        MoveCursor(x, y);
        printf("o\n");
        pthread_mutex_unlock(&mutx);
    }

    else if(direction=='u'){  //when 'u' go up
        pthread_mutex_lock(&mutx);
        printf(" ");
        y--;
        if(y<0) y=22; //when 'o' meets top border
        MoveCursor(x, y);
        printf("o\n");
        pthread_mutex_unlock(&mutx);
    }

    else if(direction=='d'){  //when 'o' go down
        pthread_mutex_lock(&mutx);
        printf(" ");
        y++;
        if(y>22) y=0;  //when 'o' bottom border
        MoveCursor(x, y);
        printf("o\n");
        pthread_mutex_unlock(&mutx);
    }

    else if(direction=='r'){  //when 'o' go right
        pthread_mutex_lock(&mutx);
        printf(" ");
        x++;
        if(x>80) x=0; //when 'o' meets right border
        MoveCursor(x, y);
        printf("o\n");
        pthread_mutex_unlock(&mutx);
    }
    }
    close(sub_sock);
}

int main(int argc ,char* argv[]){

    if(argc!=3){
        printf("This Node Can Only sublish!!!\n");
        return -1;
    }

    socketFD=socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sock_addr;

    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family=AF_INET;
    inet_aton("127.0.0.1", (struct in_addr*) &sock_addr.sin_addr.s_addr);
    sock_addr.sin_port=htons(10000);

    if(connect(socketFD, (struct sockaddr*)&sock_addr, sizeof(sock_addr))==-1){
        printf("connect error()");
        return-1;
    }

    PACKET p;
    pthread_t sub;

    signal(SIGALRM, alarmHandler);

    if(argc==3){
        if(!strcmp(argv[1],"-s")){
            strcpy(p.subBuffer,argv[2]);
            strcpy(p.pubBuffer,"NULL");
            write(socketFD,(void*)&p,sizeof(p));
        }
         else{
            printf("This Node Can Only sublish!!!");
            return -1;
        }
    }

    pthread_create(&sub, NULL,subscribe,NULL );
    pthread_join(sub,NULL);

    close(socketFD);

    return 0;
}
