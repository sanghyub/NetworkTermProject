#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#define BUF_SIZE 20
#define PORT 10000
using namespace std;

typedef struct PACKET{
    char pubBuffer[BUF_SIZE];
    char subBuffer[BUF_SIZE];
}PACKET;

typedef struct NODEINFO{
    char pubTopic[BUF_SIZE];
    char subTopic[BUF_SIZE];
    int socketFD;
    struct sockaddr_in addr;
}NODEINFO;

NODEINFO nodeTable[5];  //table
int currTable;
int main()
{
    int brok_sock, node_sock;
    struct sockaddr_in brok_addr;
    struct sockaddr_in node_addr;
    socklen_t node_adr_sz;

    PACKET p;
    brok_sock=socket(PF_INET,SOCK_STREAM,0);  //socket creation

    if(brok_sock==-1){
        printf("broker_sock creatioin error()");
        return -1;
    }
    memset(&brok_addr, 0, sizeof(brok_addr));
    brok_addr.sin_family=AF_INET;
    brok_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    brok_addr.sin_port=htons(PORT);

    if(bind(brok_sock,(struct sockaddr*)&brok_addr, sizeof(brok_addr)) ==-1){  //bind
        printf("bind error()");
        return -1;
    }

    if(listen(brok_sock,5)==-1){  //listen
        printf("listen error()");
        return -1;
    }


    for(int k=0; k<5; k++){
        strcpy(nodeTable[k].pubTopic,"NULL");
        strcpy(nodeTable[k].subTopic,"NULL");
        nodeTable[k].socketFD=-1;
    }
     printf("wating for Node............\n");

    while(1){

        bool full =true;
        for(int k=0; k<5; k++){
            if(nodeTable[k].socketFD==-1){  //when table is empty ,store node info
                node_adr_sz=sizeof(node_addr);
                node_sock=accept(brok_sock, (struct sockaddr*)&node_addr, &node_adr_sz);
                while(read(node_sock, (void*)&p, BUF_SIZE*2)==0);
                strcpy(nodeTable[k].pubTopic,p.pubBuffer);
                strcpy(nodeTable[k].subTopic,p.subBuffer);
                nodeTable[k].addr= node_addr;
                nodeTable[k].socketFD=node_sock;
                currTable=k;
                full =false;
                break;
            }
        }

        if(full){   //when table is full, block
            printf("Table is Full!!!!!!\n\n");
            continue;
        }

        printf("---------------------Table Information---------------------\n"); //print table info
        for(int j=0; j<5; j++){
            if(nodeTable[j].socketFD==-1) continue;
            printf("\nPublish Topic: %s\n",nodeTable[j].pubTopic);
            printf("Subscribe Topic: %s\n",nodeTable[j].subTopic);
            printf("IP: %s\n",inet_ntoa(nodeTable[j].addr.sin_addr));
            printf("Port: %d\n",ntohs(nodeTable[j].addr.sin_port));
            printf("Socket Number: %d\n\n",nodeTable[j].socketFD);
        }
        printf("-----------------------------------------------------------\n\n\n");

        for(int j=0;j<5; j++){
            if(currTable!=j){
                if(!strcmp(nodeTable[currTable].subTopic, nodeTable[j].pubTopic)){
                    if(strcmp(nodeTable[j].pubTopic,"NULL")){
                    write(node_sock, (void*)&nodeTable[j].addr, sizeof(nodeTable[j].addr));  //send subscriber address
                    break;
                    }
                }
            }
        }
        for(int j=0; j<5; j++){
            if(currTable!=j){
                if(!strcmp(nodeTable[currTable].pubTopic, nodeTable[j].subTopic)){
                    if(strcmp(nodeTable[j].subTopic,"NULL")){
                    write(nodeTable[j].socketFD, (void*)&nodeTable[currTable].addr, sizeof(nodeTable[currTable].addr)); //send subscriber address
                    }
                }
            }
        }
    }

    return 0;
}
