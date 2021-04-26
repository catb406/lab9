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
#include <queue>

using namespace std;

queue<string> querySet;

int listenSocket=socket(AF_INET, SOCK_STREAM, 0);
int serverSocket= socket(AF_INET, SOCK_STREAM,0);
pthread_mutex_t mt;
int flagGetR=0, flagSendAn=0, flagWaitCon=0;

void* GetRequest(void* arg){
    const int RCV_SIZE = 1024;
    while(flagGetR==0){
        string rcv;
        vector<char> buf(RCV_SIZE);

        ssize_t reccount=recv(serverSocket, &buf[0], RCV_SIZE, 0);
        if (reccount==-1){
            perror("recv error");
            sleep(1);
        }else if(reccount==0){
            printf("disconnection\n");
            sleep(1);
        } else{
            rcv.append(buf.begin(), buf.end());
            cout << rcv << endl;

            pthread_mutex_lock(&mt);
            querySet.push(rcv);
            pthread_mutex_unlock(&mt);

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
    while (flagSendAn == 0) {
        pthread_mutex_lock(&mt);
        if (!querySet.empty()){
            string S=querySet.front();
            querySet.pop();
            pthread_mutex_unlock(&mt);
            printf("server got message from client: %s\n", S.c_str());
            string response=ProcRequest();
            vector<char>resp_c(response.begin(), response.end());
            ssize_t sentcount=send(serverSocket,&resp_c[0], resp_c.size(), 0);
            if (sentcount==-1){
                perror("send error");
            }else{
                printf("sended: %s", response.c_str());
            }
        }else{
            printf("queue is empty\n");
            pthread_mutex_unlock(&mt);
            sleep(1);
        }
    }
    pthread_exit(nullptr);
}

void* WaitConnection(void* arg){
    struct sockaddr_in serverSockAddr;
    socklen_t addrLen=(socklen_t) sizeof(serverSockAddr);
    while(flagWaitCon==0){
        serverSocket=accept(listenSocket, (struct sockaddr*)NULL, NULL);
        if (serverSocket==-1){
            perror("accept error");
            sleep(1);
        }else{
            printf("connection established\n");
            pthread_t sendAns, getR;
            pthread_create(&sendAns, nullptr, SendAnswer, nullptr);
            pthread_create(&getR, nullptr, GetRequest, nullptr);

            void* status_1;
            void* status_2;
            int res_send = pthread_join(sendAns, &status_1);
            int res_get = pthread_join(getR, &status_2);
        }
        shutdown(serverSocket, 2);
        close(serverSocket);
    }
    pthread_exit(nullptr);
}
int main() {
    printf("server started\n");
    pthread_mutex_init(&mt, NULL);
    pthread_t waitC;

    struct sockaddr_in listenSockAddr;
    listenSockAddr.sin_family=AF_INET;
    listenSockAddr.sin_port= htons(7000);
    listenSockAddr.sin_addr.s_addr= htonl(INADDR_ANY);
    fcntl(listenSocket, F_SETFL, O_NONBLOCK);
    if (-1 == bind(listenSocket,(struct sockaddr*)&listenSockAddr,sizeof(listenSockAddr))){ //устанавливаем соединение
        perror("bind socket error");
        close(listenSocket);
        exit(1);
    }else{
        printf("bind socket OK\n");
    }    int optval=1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    listen(listenSocket, SOMAXCONN);
    pthread_create(&waitC, nullptr, WaitConnection, nullptr);

    printf("waiting for key press\n");
    getchar();
    flagWaitCon=1;
    flagSendAn=1;
    flagGetR=1;
    printf("key was pressed\n");
    void* status;
    int res = pthread_join(waitC, &status);
    cout << "server finished\n" << endl;
    pthread_mutex_destroy(&mt);
    close(listenSocket);
    return 0;
}
