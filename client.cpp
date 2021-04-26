#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <sys/sysinfo.h>
#include <csignal>
#include <arpa/inet.h>


using namespace std;

vector<string>msglist;
struct clientArgs
{
    int flag;
    int mySock;
    struct sockaddr_in saddr;
    socklen_t saddrlen;
    pthread_mutex_t mx;
};
void sig_handler(int fd)
{
    printf("Client is crushed or disconnected with server\n");
    sleep(1);
}

void* SendRequest(void* arg){
   clientArgs* args=(clientArgs*) arg;
    int* flag=&args->flag;
    int mySock=args->mySock;
    pthread_mutex_t mx=args->mx;
    struct sockaddr_in saddr=args->saddr;
    socklen_t saddrlen=args->saddrlen;
    int count=0;
    char sndbuf[256];
    while(*flag==0){
        int len = sprintf(sndbuf,"request %d",count);
        sleep(1);
        int sentcount = sendto(mySock,sndbuf,len,0,(struct sockaddr*)&saddr,saddrlen);
        if (sentcount == -1) {
            perror("send error");
        }else{
            printf("client send to server: %s\n", sndbuf);
        }
        count++;
    }
    pthread_exit(nullptr);
}
void* GetAnswer(void* arg){
    clientArgs* args=(clientArgs*) arg;
    int* flag=&args->flag;
    int mySock=args->mySock;
    pthread_mutex_t mx=args->mx;
    struct sockaddr_in saddr=args->saddr;
    socklen_t saddrlen=args->saddrlen;
    char rcvbuf[256];
    while (*flag==0){
        memset(rcvbuf,0,sizeof(rcvbuf));
        int recvcount = recvfrom(mySock,rcvbuf,sizeof(rcvbuf),0,(struct sockaddr*)&saddr,&saddrlen);
        if (recvcount == -1) {
            perror("recv error");
            sleep(1);
        }else{
            printf("client got from server: %s\n", rcvbuf);
        }
    }
    pthread_exit(nullptr);
}

int main(){
    printf("client started\n");

    signal(SIGPIPE, sig_handler);
    clientArgs args;
    pthread_t sendReq, getAns;
    args.mySock=socket(AF_INET,SOCK_DGRAM,0);

    fcntl(args.mySock, F_SETFL, O_NONBLOCK);

    args.flag=0;
    struct sockaddr_in bindaddr;
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(7000);
    bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(args.mySock,(struct sockaddr*)&bindaddr,sizeof(bindaddr));

    memset(&args.saddr, 0, sizeof(args.saddr));
    args.saddr.sin_family = AF_INET;
    args.saddr.sin_port = htons(8000);
    args.saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    args.saddrlen = sizeof(args.saddr);

    pthread_mutex_init(&args.mx, nullptr);
    pthread_create(&sendReq, nullptr, SendRequest, (void*)&args);
    pthread_create(&getAns, nullptr, GetAnswer, (void*)&args);
    printf("waiting for key press\n");
    args.flag=getchar();
    printf("key was pressed\n");
    pthread_join(sendReq, nullptr);
    pthread_join(getAns, nullptr);
    close(args.mySock);
    printf("client finished\n");
    return 0;
}