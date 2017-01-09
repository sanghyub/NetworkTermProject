#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <termios.h>
#include <stdlib.h>
#define BUF_SIZE 20

int socketFD,pub_sock,key;
int clnt_socks[4] ,cnt=0;
pthread_mutex_t mutx= PTHREAD_MUTEX_INITIALIZER; //mutex init

typedef struct PACKET{
    char pubBuffer[BUF_SIZE];
    char subBuffer[BUF_SIZE];
}PACKET;

int getch(void) 
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}

void *input_key(void* arg){  // key thread
    pthread_detach(pthread_self());
    printf("Input Direction Key!!\n");
    while(1){
        key=getch();
        if(key==26 ||key==3) return 0;
        if(key==27){
            if(key==91) {
            key=getch();
            }
        }
    }
}

void *clnt_handle(void* arg){  //handle client threads
    int clnt_sock=*((int*)arg);
    free(arg);
    pthread_t key_id;
    pthread_detach(pthread_self());
    pthread_create(&key_id,NULL,input_key,NULL);  //create key thread

    char left='l';
    char right='r';
    char up='u';
    char down='d';

    while(1){
            if(key==68){   //when key input is left
                printf("[Publish]LEFT!\n");
                pthread_mutex_lock(&mutx);
                for(int i=0; i<4; i++){
                    if(clnt_socks[i]!=-1)  write(clnt_socks[i],(void*)&left,sizeof(left));
                }
                key=0;
                pthread_mutex_unlock(&mutx);
            }
            else if(key==65){  //when key input is up
                printf("[Publish]UP!\n");
                pthread_mutex_lock(&mutx);
                for(int i=0; i<4; i++){
                    if(clnt_socks[i]!=-1) write(clnt_socks[i],(void*)&up,sizeof(up));
                }
                key=0;
                pthread_mutex_unlock(&mutx);
            }

            else if(key==67){  //when key input is right
                printf("[Publish]RIGHT!\n");
                pthread_mutex_lock(&mutx);
                for(int i=0; i<4; i++){
                    if(clnt_socks[i]!=-1) write(clnt_socks[i],(void*)&right,sizeof(right));
                }
                key=0;
                pthread_mutex_unlock(&mutx);
            }

            else if(key==66){   //when key input is down
                printf("[Publish]DOWN!\n");
                pthread_mutex_lock(&mutx);
                for(int i=0; i<4; i++){
                    if(clnt_socks[i]!=-1) write(clnt_socks[i],(void*)&down,sizeof(down));
                }
                key=0;
                pthread_mutex_unlock(&mutx);
            }
 }
 close(clnt_sock);

}

void *publish(void* arg){
    pthread_t clnt_id[4];

    struct sockaddr_in peer_addr;
    socklen_t peer_addr_sz;
    peer_addr_sz=sizeof(peer_addr);
    getsockname(socketFD, (struct sockaddr*)&peer_addr, &peer_addr_sz);
    peer_addr.sin_port/=3;

    int clnt_sock,i;
    struct sockaddr_in pub_addr;
    socklen_t pub_adr_sz;
    pub_adr_sz=sizeof(pub_addr);

    pub_sock=socket(PF_INET,SOCK_STREAM,0);

    memset(&pub_addr,0,sizeof(pub_addr));
    pub_addr.sin_family=AF_INET;
    pub_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    pub_addr.sin_port=peer_addr.sin_port;
    bind(pub_sock,(struct sockaddr*)&pub_addr, sizeof(pub_addr));

    listen(pub_sock,5);
    printf("waiting for subscribe\n");

    int* socketFDpointer;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_adr_sz;
    clnt_adr_sz=sizeof(clnt_addr);

    while(true){
		socketFDpointer = (int*)malloc(sizeof(int)); // allcocate client_socket
        *socketFDpointer=accept(pub_sock, (struct sockaddr*)&clnt_addr, &clnt_adr_sz);
        pthread_mutex_lock(&mutx);
        clnt_socks[cnt]=*socketFDpointer;
        pthread_create(&clnt_id[cnt], NULL, clnt_handle, (void*)socketFDpointer);
        cnt++;
        printf("Connection Accept Success!!\n");
        pthread_mutex_unlock(&mutx);
    }
    close(pub_sock);
}


int main(int argc ,char* argv[]){

    if(argc!=3){
        printf("This Node Can Only Publish!!\n");
        return -1;
    }
    memset(&clnt_socks,-1,sizeof(clnt_socks));
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
    pthread_t pub;

    if(argc==3){
        if(!strcmp(argv[1],"-p")){
            strcpy(p.pubBuffer,argv[2]);
            strcpy(p.subBuffer,"NULL");
            write(socketFD,(void*)&p,sizeof(p));
        }
         else{
            printf("This Node Can Only Publish!!");
            return -1;
        }
    }

    pthread_create(&pub, NULL,publish, NULL);
    pthread_join(pub,NULL);

    close(socketFD);

    return 0;
}

