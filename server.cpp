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
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace std;

vector<string>msglist;
struct serverArgs
{
    int flag;
    int mySock;
    struct sockaddr_in saddr;
    socklen_t saddrlen;
    pthread_mutex_t mx;
};

void* GetRequest(void* arg){
    serverArgs* args=(serverArgs*) arg;
    int* flag=&args->flag;
    int mySock=args->mySock;
    pthread_mutex_t mx=args->mx;
    struct sockaddr_in saddr=args->saddr;
    socklen_t saddrlen=args->saddrlen;
    char rcvbuf[256];
    while(*flag==0){
        memset(rcvbuf,0,sizeof(rcvbuf));
        int recvcount = recvfrom(mySock,rcvbuf,sizeof(rcvbuf),0,(struct sockaddr*)&saddr,&saddrlen);
        if (recvcount==-1){
            perror("recv error");
            sleep(1);
        }else{
            pthread_mutex_lock(&mx);
            printf("server got message from client: %s\n", rcvbuf);
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
    int mySock=args->mySock;
    pthread_mutex_t mx=args->mx;
    struct sockaddr_in saddr=args->saddr;
    socklen_t saddrlen=args->saddrlen;
    char sndbuf[256];
    while (*flag == 0) {
        pthread_mutex_lock(&mx);
        if (!msglist.empty()){
            string S=msglist.back();
            msglist.pop_back();
            pthread_mutex_unlock(&mx);
            string msg=S+": "+ProcRequest();
            memset(sndbuf,0,sizeof(sndbuf));
            strcpy(sndbuf, msg.c_str());
            int sentcount = sendto(mySock,sndbuf,sizeof(msg),0,(struct sockaddr*)&saddr,saddrlen);
            if (sentcount == -1) {
                perror("send error");
            }else{
                printf("server send to client: %s\n", sndbuf);
            }
        }else{
              pthread_mutex_unlock(&mx);
              printf("queue is empty\n");
              sleep(1);
          }
    }
    pthread_exit(nullptr);

}

int main() {
    printf("server started\n");

    serverArgs args;
    pthread_t getReq, sendAns;

    int optval=1;

    args.mySock=socket(AF_INET,SOCK_DGRAM,0);
    fcntl(args.mySock, F_SETFL, O_NONBLOCK);
    setsockopt(args.mySock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));


    args.flag=0;
    struct sockaddr_in bindaddr;
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(8000);
    bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(args.mySock,(struct sockaddr*)&bindaddr,sizeof(bindaddr));

    memset(&args.saddr, 0, sizeof(args.saddr));
    args.saddr.sin_family = AF_INET;
    args.saddr.sin_port = htons(7000);
    args.saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    args.saddrlen = sizeof(args.saddr);

    pthread_mutex_init(&args.mx, nullptr);
    pthread_create(&getReq, nullptr, GetRequest, (void*)&args);
    pthread_create(&sendAns, nullptr, SendAnswer, (void*)&args);

    printf("waiting for key press\n");
    args.flag=getchar();
    printf("key was pressed\n");

    pthread_join(getReq, nullptr);
    pthread_join(sendAns, nullptr);

    cout << "server finished\n" << endl;
    pthread_mutex_destroy(&args.mx);
    close(args.mySock);
    return 0;
}
