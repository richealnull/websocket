#include "client.hpp"
#include "conn.hpp"
#include <unistd.h>
#include <stdio.h>

void* pthread_func(void* ptr){
    const char* ip = "120.79.235.156";
    unsigned short port = 10001;
    printf_client client;
    client.tcp_conn(ip,port);
    client.tcpserver_login();
    sleep(4);
    client.websock_client();
    int num = 0;
    while(1){
        num++;
        //300大概25秒可以用来做心跳发送
        if(num == 300){
            client.heart_send();
            //printf("reconn is %d\n",reconn);
            //if(reconn <= 0){
              //  client.tcp_conn(ip,port);
                //client.tcpserver_login();
            
            num = 0;
        }
        client.slice();
    }
}

int main() {
    pthread_t tid;
    print_server server;
    printf("pthread---------star\n");
    int i =  pthread_create(&tid,NULL,pthread_func,NULL);
    printf("pthread--------create%d\n",i); 
    sleep(2);
    server.unxconnect();
    server.run(9002);
    pthread_exit( NULL);
}

