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

using namespace std;

vector<string>msglist;
struct serverArgs
{
    int flag;
    int listenSocket;
    int serverSocket;
    pthread_mutex_t mx;
};

void* GetRequest(void* arg){
    serverArgs* args=(serverArgs*) arg;
    int* flag=&args->flag;
    int serverSocket=args->serverSocket;
    pthread_mutex_t mx=args->mx;
    char rcvbuf[256];
    while(*flag==0){
        ssize_t reccount=recv(serverSocket, rcvbuf, 256, 0);
        if (reccount==-1){
            perror("recv error");
            sleep(1);
        }else if(reccount==0){
            printf("disconnection\n");
            sleep(1);
        } else{
            pthread_mutex_lock(&mx);
            msglist.push_back(rcvbuf);
            pthread_mutex_unlock(&mx);

        }
    }
    pthread_exit(nullptr);
}

string ProcRequest(){
    struct sysinfo sysinfo1;
    sysinfo(&sysinfo1);
    string msg="system uptime "+ to_string(sysinfo1.uptime);
    return msg;
}

void* SendAnswer(void* arg) {
    serverArgs* args=(serverArgs*) arg;
    int* flag=&args->flag;
    int serverSocket=args->serverSocket;
    pthread_mutex_t mx=args->mx;
    while (*flag == 0) {

        pthread_mutex_lock(&mx);
        if (!msglist.empty()){
            string S=msglist.back();
            msglist.pop_back();
            pthread_mutex_unlock(&mx);

            printf("server got message from client: %s\n", S.c_str());
            string msg=ProcRequest();
            char* sndbuf=new char;
            strcpy(sndbuf, msg.c_str());
            ssize_t sentcount=send(serverSocket,sndbuf, sizeof(msg), 0);
            if (sentcount==-1){
                perror("send error");
            }else{
                printf("sended: %s\n", sndbuf);
            }
        }else{
            pthread_mutex_unlock(&mx);
            printf("queue is empty\n");
            sleep(1);
        }
    }
    pthread_exit(nullptr);
}

void* WaitConnection(void* arg){
    serverArgs* args=(serverArgs*) arg;
    int* flag=&args->flag;
    int* serverSocket=&args->serverSocket;
    int listenSocket=args->listenSocket;

    while(*flag==0){
        *serverSocket=accept(listenSocket, (struct sockaddr*)NULL, NULL);
        if (*serverSocket==-1){
            perror("accept error");
            sleep(1);
        }else{
            printf("connection with client established\n");
            pthread_t sendAns, getR;
            pthread_create(&sendAns, nullptr, SendAnswer, arg);
            pthread_create(&getR, nullptr, GetRequest, arg);

            int res_send = pthread_join(sendAns, nullptr);
            int res_get = pthread_join(getR, nullptr);
        }
        shutdown(*serverSocket, 2);
        close(*serverSocket);
    }
    pthread_exit(nullptr);
}
int main() {
    printf("server started\n");

    serverArgs args;
    pthread_t waitC;

    struct sockaddr_in listenSockAddr;
    int optval=1;

    args.flag=0;
    args.listenSocket= socket(AF_INET, SOCK_STREAM, 0);
    fcntl(args.listenSocket, F_SETFL, O_NONBLOCK);

    listenSockAddr.sin_family=AF_INET;
    listenSockAddr.sin_port= htons(7000);
    listenSockAddr.sin_addr.s_addr= htonl(INADDR_ANY);

    if (-1 == bind(args.listenSocket,(struct sockaddr*)&listenSockAddr,sizeof(listenSockAddr))){
        perror("bind socket error");
        close(args.listenSocket);
        exit(1);
    }else{
        printf("bind socket OK\n");
    }
    setsockopt(args.listenSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    listen(args.listenSocket, SOMAXCONN);
    pthread_mutex_init(&args.mx, nullptr);
    pthread_create(&waitC, nullptr, WaitConnection, (void*)&args);

    printf("waiting for key press\n");
    args.flag=getchar();
    printf("key was pressed\n");
    int res = pthread_join(waitC, nullptr);
    cout << "server finished\n" << endl;
    pthread_mutex_destroy(&args.mx);
    close(args.listenSocket);
    return 0;
}
