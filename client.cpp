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

struct clientArgs
{
    int flag;
    int clientSocket;
    struct sockaddr_in clientSockAddr;
};
void sig_handler(int fd)
{
    printf("Client is crushed or disconnected with server\n");
    sleep(1);
}

void* SendRequest(void* arg){
    clientArgs* args=(clientArgs*) arg;
    int* flag=&args->flag;
    int clientSocket=args->clientSocket;
    int count=0;
    char sndbuf[256];
    while(*flag==0){
        memset(sndbuf, 0, 256);
        int len= sprintf(sndbuf,"request %d\n", count);
        ssize_t sentcount=send(clientSocket, sndbuf, len, 0);
        if (sentcount==-1){
            perror("send error");
        }else{
            printf("sended to server: %s\n", sndbuf);
        }
        count++;
        sleep(1);
    }
    pthread_exit(nullptr);
}
void* GetAnswer(void* arg){
    clientArgs* args=(clientArgs*) arg;
    int* flag=&args->flag;
    int clientSocket=args->clientSocket;
    while (*flag==0){
        char rcvbuf[256];
        ssize_t reccount=recv(clientSocket, rcvbuf, 256, 0);
        if (reccount==-1){
            perror("recv error");
            sleep(1);
        }else if (reccount==0){
            printf("disconnection\n");
            sleep(1);
        }else{
            printf("client got from server: %s\n", rcvbuf);
        }
    }
    pthread_exit(nullptr);
}
void* Connect(void* arg){
    printf("connect func started\n");
    clientArgs* args=(clientArgs*) arg;
    int* flag=&args->flag;
    int clientSocket=args->clientSocket;
    struct sockaddr_in clientSockAddr =args->clientSockAddr;
    while (*flag==0){
        int result= connect(clientSocket, (struct sockaddr*)&clientSockAddr, sizeof(clientSockAddr));
        if (result==-1){
            perror("connect error");
            sleep(1);
        }else{
            printf("connection established\n");
            pthread_t sendR, getAn;
            pthread_create(&sendR, nullptr, SendRequest, arg);
            pthread_create(&getAn, nullptr, GetAnswer, arg);

            int res_send = pthread_join(sendR, nullptr);
            int res_get = pthread_join(getAn, nullptr);
        }
    }
    pthread_exit(nullptr);
}

int main(){
    printf("client started\n");

    signal(SIGPIPE, sig_handler);
    pthread_t con;
    clientArgs args;

    args.flag=0;
    args.clientSocket=socket(AF_INET, SOCK_STREAM,0);
    fcntl(args.clientSocket,F_SETFL,O_NONBLOCK);
    args.clientSockAddr.sin_family=AF_INET;
    args.clientSockAddr.sin_port= htons(7000);
    args.clientSockAddr.sin_addr.s_addr= inet_addr("127.0.0.1");
    pthread_create(&con, nullptr, Connect, (void*)&args);
    printf("waiting for key press\n");
    args.flag=getchar();
    printf("key was pressed\n");
    pthread_join(con, nullptr);
    shutdown(args.clientSocket, 2);
    close(args.clientSocket);
    printf("client finished\n");
    return 0;
}