#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#define BUF_SIZE 20

int socketFD,pub_sock;
int clnt_socks[4] ,cnt=0;
pthread_mutex_t mutx= PTHREAD_MUTEX_INITIALIZER;  //mutex init
struct sockaddr_in finish_addr;
typedef struct PACKET{
    char pubBuffer[BUF_SIZE];
    char subBuffer[BUF_SIZE];
}PACKET;


void *clnt_handle(void* arg){    //serive to clients
    int clnt_sock=*((int*)arg);
    free(arg);
    pthread_detach(pthread_self());
    char pubMsg[8]="Publish";
    while(1){
    for(int i=0; i<4; i++){
        if(clnt_socks[i]==-1) break;  
        write(clnt_socks[i],(void*)&pubMsg, sizeof(pubMsg)); //when clnt_sock is connected , write msg
        printf("[SEND]Publish\n\n");
        }
    }
    close(clnt_sock);
}

void *publish(void* arg){
    pthread_t clnt_id[4];

    struct sockaddr_in peer_addr;
    socklen_t peer_addr_sz;
    peer_addr_sz=sizeof(peer_addr);
    getsockname(socketFD, (struct sockaddr*)&peer_addr, &peer_addr_sz); //get node address
    finish_addr=peer_addr;
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
    bind(pub_sock,(struct sockaddr*)&pub_addr, sizeof(pub_addr));  //publish socket bind

    listen(pub_sock,5);
    printf("waiting for subscriber\n");

    int* socketFDpointer;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_adr_sz;
    clnt_adr_sz=sizeof(clnt_addr);

    while(true){
        socketFDpointer=(int*)malloc(sizeof(int));  //socket pointer allocation
        *socketFDpointer=accept(pub_sock, (struct sockaddr*)&clnt_addr, &clnt_adr_sz);  //accept
        pthread_mutex_lock(&mutx);
        clnt_socks[cnt]=*socketFDpointer;
        pthread_create(&clnt_id[cnt], NULL, clnt_handle, (void*)socketFDpointer); //thread create and pass clnt_sock
        cnt++;
        printf("Connection Accept Success!!\n");
        pthread_mutex_unlock(&mutx);
    }

    close(pub_sock);
}

void *subscribe(void *arg){
    struct sockaddr_in sub_addr;
    int str_len=0;
    printf("waiting for publisher\n");
    while(str_len=read(socketFD,(void*)&sub_addr,sizeof(sub_addr))==0);
    printf("I got Address :%d\n", ntohs(sub_addr.sin_port));
    int sub_sock=socket(PF_INET,SOCK_STREAM,0);
    sub_addr.sin_family=AF_INET;
    inet_aton("127.0.0.1", (struct in_addr*) &sub_addr.sin_addr.s_addr);
    sub_addr.sin_port=sub_addr.sin_port/3;
	while (connect(sub_sock, (struct sockaddr*)&sub_addr, sizeof(sub_addr)) == -1); //until it's connected , block
    printf("connect success!!\n");

    char buffer[8];
    while(1){
        while(read(sub_sock,(void*)&buffer, sizeof(buffer))==0);
        printf("[INFO]%s\n\n", buffer);
    }
    close(sub_sock);
}

int main(int argc ,char* argv[]){

    if(argc!=3 && argc!=5){
        printf("error! publish or subscribe anything!\n");
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
    pthread_t sub;

    if(argc==3){  //when argument is either publish or subscribe
        if(!strcmp(argv[1],"-p")){
            strcpy(p.pubBuffer,argv[2]);
            strcpy(p.subBuffer,"NULL");
            write(socketFD,(void*)&p,sizeof(p));
        }
        else if(!strcmp(argv[1],"-s")){
            strcpy(p.subBuffer,argv[2]);
            strcpy(p.pubBuffer,"NULL");
            write(socketFD,(void*)&p,sizeof(p));
        }
         else{
            printf("wrong argument!");
            return -1;
        }
    }

    else if(argc==5){	//when arguments are both publish and subscribe
        if(!strcmp(argv[1],"-p")){
            strcpy(p.pubBuffer,argv[2]);
            strcpy(p.subBuffer,argv[4]);
            write(socketFD,(void*)&p,sizeof(p));
        }
        else if(!strcmp(argv[1],"-s")){
            strcpy(p.subBuffer,argv[2]);
            strcpy(p.pubBuffer,argv[4]);
            write(socketFD,(void*)&p,sizeof(p));
        }
        else{
            printf("wrong argument!");
            return -1;
        }

    }

    if(argc==3){
        if(!strcmp(argv[1],"-p")){
            pthread_create(&pub, NULL,publish, NULL );
            pthread_join(pub,NULL);
        }
        else if(!strcmp(argv[1],"-s")){
            pthread_create(&sub, NULL,subscribe,NULL );
            pthread_join(sub,NULL);
        }
    }
    else if(argc==5){
        pthread_create(&pub, NULL,publish, NULL );
        pthread_create(&sub, NULL,subscribe,NULL );

        pthread_join(pub,NULL);
        pthread_join(sub,NULL);
    }

    close(socketFD);

    return 0;
}
