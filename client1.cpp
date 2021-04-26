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

int clientSocket=socket(AF_INET, SOCK_STREAM, 0);
pthread_mutex_t mt;
vector<string>msglist;
int flagGetAn=0, flagSendR=0, flagCon=0;

void sig_handler(int fd)
{
    printf("Client is crushed or disconnect with server\n");
    sleep(1);
}

void* SendRequest(void* arg){
    int count=0;
    while(flagSendR==0){
        sleep(1);
        char sndbuf[256];
        int len= sprintf(sndbuf,"request %d", count);
        ssize_t sentcount=send(clientSocket, sndbuf, len, 0);
        if (sentcount==-1){
            perror("send error");
        }else{
            printf("sended to server: %s", sndbuf);
        }
        count++;
        sleep(1);
    }
    pthread_exit(nullptr);
}
void* GetAnswer(void* arg){
    char rcvbuf[256];
    while (flagGetAn==0){
        memset(rcvbuf, 0, 256);
        ssize_t reccount=recv(clientSocket, rcvbuf, 256, 0);
        if (reccount==-1){
            perror("recv error");
            sleep(1);
        }else if (reccount==0){
            printf("disconnection\n");
            sleep(1);
        }else{
            printf("client got from server: %s", rcvbuf);
        }
    }
    pthread_exit(nullptr);
}
void* Connect(void* arg){
    struct sockaddr_in clientSockAddr;
    while (flagCon==0){
        int result= connect(clientSocket, (struct sockaddr*)&clientSockAddr, sizeof(clientSockAddr));
        if (result==-1){
            perror("connect error");
            sleep(1);
        }else{
            printf("connection established\n");
            pthread_t sendR, getAn;
            pthread_create(&sendR, nullptr, SendRequest, nullptr);
            pthread_create(&getAn, nullptr, GetAnswer, nullptr);

            void* status_1;
            void* status_2;
            int res_send = pthread_join(sendR, &status_1);
            int res_get = pthread_join(getAn, &status_1);
            pthread_exit(nullptr);
        }
    }
    pthread_exit(nullptr);
}

int main(){
    printf("client started\n");
    signal(SIGPIPE, sig_handler);
    pthread_t con;

    clientSocket = socket(AF_INET, SOCK_STREAM,0);
    fcntl(clientSocket,F_SETFL,O_NONBLOCK);
    struct sockaddr_in listenSockAddr;
    listenSockAddr.sin_family=AF_INET;
    listenSockAddr.sin_port= htons(7000);
    listenSockAddr.sin_addr.s_addr= inet_addr("127.0.0.1");
    pthread_create(&con, nullptr, Connect, nullptr);
    printf("waiting for key press\n");
    getchar();
    flagCon=1;
    flagGetAn=1;
    flagSendR=1;
    printf("key was pressed\n");
    void* status;
    int res = pthread_join(con, &status);
    shutdown(clientSocket, 2);
    close(clientSocket);
    printf("client finished\n");
    return 0;
}