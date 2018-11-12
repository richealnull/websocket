#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <stdio.h>  
#include <stdlib.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string>
#include "md5.h"
#include "websocket_endpoint.h"
#include "utility.h"

#define UNIX_DOMAIN "/tmp/UNIX.domain"
#define SHORT_LEN 2
#define NAME_HEAD_LEN 3
#define LONG_LEN 8
#define NAME_LEN 13

using namespace std;

char* _miyue = (char*)"ddeeff";
//大端转小端
unsigned long long ntohll(unsigned long long val)
{
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
        return (((unsigned long long )htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl(    (int)(val >> 32));
    }
    else if (__BYTE_ORDER == __BIG_ENDIAN)
    {
        return val;
    }
}
//小端转大端
unsigned long long htonll(unsigned long long val)
{
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
        return (((unsigned long long )htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl(    (int)(val >> 32));
    }
    else if (__BYTE_ORDER == __BIG_ENDIAN)
    {
        return val;
    }
}
//141封装
int buff(unsigned char* buffer,int size,unsigned char** _buff){
    int _length = 1 + 4 + 1 + size;
    unsigned char* _buffer = (unsigned char *)malloc(_length);
    _buffer[0]  = 0xaa;
    *((unsigned int *)(_buffer + 1))    = htonl(_length);
    _buffer[5]  = 0 - _buffer[0] - _buffer[1] - _buffer[2] - _buffer[3] - _buffer[4];
    memcpy(_buffer + 6, buffer, size);
    *_buff = _buffer;
    return _length;
}

//多播包封装
int fengzhuang(unsigned char** send,unsigned char* recv,int size,unsigned char* id,int idsize)
{
    unsigned char* buffer = (unsigned char*)malloc(size+idsize+9);
    memset(buffer,0,size+idsize);
    int len =0;
    if(buffer == NULL)
    {
        printf("malloc error\n");
    }
    buffer[0] = 0x02; //设置广播标识码
    len += 1;
    short ids =1; //id数量
    short IDS = htons(ids);
    memcpy(buffer+len,(unsigned char*)&IDS,sizeof(ids));
    len += sizeof(ids);
    unsigned short nlen = htons((unsigned short)idsize);
    memcpy(buffer+len,(unsigned char*)&nlen,sizeof(nlen));//id长度
    len += sizeof(nlen);
    memcpy(buffer+len,id,idsize);//id
    len += idsize;
    int rlen = htonl(size);
    memcpy(buffer+len,(unsigned char*)&rlen,sizeof(rlen)); //数据包长
    len += sizeof(rlen);
    memcpy(buffer+len,recv,size); //数据包内容
    len += size;
    *send = buffer;
    return len;
    printf("duobo size is %d\n",len);
}

class printf_client{
    private:
        int _fpEpoll;
        int tcpclient_fd;
        int unxclient_fd;
        int listen_fd;
        int hearttime;
        string _server;
        unsigned short _port;
        char* webname; //注册id
        kagula::websocket_endpoint endpoint;
    public:
        printf_client(){
            //初始化id
            webname = (char*)malloc(13);
            time_t  webtime = time(NULL);
            srand(webtime);
            int web= rand();
            char* heat =(char*)"WEB";
            unsigned long long lrand = web;
            lrand = htonll(lrand*12345);
            memcpy(webname,heat,3);
            memcpy(webname + 3,(char*)&lrand,LONG_LEN);

            //创建epoll
            _fpEpoll = -1;
            if(_fpEpoll < 0){
                _fpEpoll    = epoll_create(128);
            }
            //创建unix监听服务端
            socklen_t clt_addr_len;
            struct sockaddr_un clt_addr;  
            struct sockaddr_un srv_addr;  
            listen_fd=socket(PF_UNIX,SOCK_STREAM,0);
            if(listen_fd<0){  
                perror("cannot create communication socket");   
            }
            //监听加入epoll
            struct epoll_event eve = {0};
            eve.data.fd  = listen_fd;
            eve.events   = EPOLLIN | EPOLLET;
            epoll_ctl(_fpEpoll, EPOLL_CTL_ADD, listen_fd, &eve);
            //set server addr_param  
            srv_addr.sun_family=AF_UNIX;  
            strncpy(srv_addr.sun_path,UNIX_DOMAIN,sizeof(srv_addr.sun_path)-1);  
            unlink(UNIX_DOMAIN);  
            //bind sockfd & addr  
            int ret=::bind(listen_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));  
            if(ret==-1)  
            {  
                perror("cannot bind server socket");  
                close(listen_fd);  
                unlink(UNIX_DOMAIN);  
            }  
            //listen sockfd   
            ret=listen(listen_fd,1);  
            if(ret==-1)  
            {  
                perror("cannot listen the client connect request");  
                close(listen_fd);  
                unlink(UNIX_DOMAIN);  
            }
        }
        //epoll监听处理函数
        void slice(){
            static struct epoll_event events[10];
            int eventcount = epoll_wait(_fpEpoll,events,10,100);
            for (int i = 0; i < eventcount; i++){
                struct epoll_event *ev  = events + i;
                if(ev->data.fd<0)
                    continue;
                if(ev->data.fd == listen_fd){
                    //监听server 有新连接来，accept
                    struct sockaddr_un clt_addr;
                    int len = sizeof(clt_addr);
                    unxclient_fd = accept(listen_fd,(struct sockaddr *)&clt_addr,(socklen_t *)&len);
                    if(unxclient_fd<0)  {  
                        perror("cannot accept client connect request");  
                        close(listen_fd);  
                        unlink(UNIX_DOMAIN);  
                    }
                    else{
                        //改为非阻塞
                        int sock_opts   = fcntl(unxclient_fd, F_GETFL);
                        sock_opts       |= O_NONBLOCK;
                        fcntl(unxclient_fd, F_SETFL, sock_opts);
                        //加入监听队列
                        struct epoll_event ev = {0};
                        ev.data.fd  = unxclient_fd;
                        ev.events   = EPOLLIN | EPOLLET;
                        epoll_ctl(_fpEpoll, EPOLL_CTL_ADD, unxclient_fd, &ev);
                        printf("unxsock is success %d\n",unxclient_fd);
                    } 
                }
                else if(ev->data.fd == tcpclient_fd){
                    //接收tcp服务器数据再发送给websock线程
                    while(1){
                        unsigned char buffer[4096];
                        int size = recv(ev->data.fd, buffer, 4096, 0);
                        if(!size)
                        {
                            //close();
                            break;
                        }
                        if(size < 0)
                        {
                            if(EAGAIN != errno)
                                ::close(tcpclient_fd);
                                break;
                        }
                        //解析这个包发送出去
                        if(buffer[0] == 0xaa){
                            if(buffer[6] == 0x01){
                                hearttime = time(NULL);
                                printf("heart recv\n");
                            }
                            else{
                            unsigned int buflen = ntohl(*(unsigned int*)(buffer+1));
                            printf("buffer size is %d\n",buflen);
                            while(size<buflen)
                            {
                                unsigned char recvbuff2[buflen-size];
                                bzero(recvbuff2,sizeof(recvbuff2));
                                int re = recv(ev->data.fd,recvbuff2,buflen-size,0);
                                memcpy(buffer+size,recvbuff2,re);
                                size += re;
                            }
                                //跳过141开始解析转发
                                unsigned char str[buflen-4];
                                memset(str,0,buflen-4);
                                memcpy(str,buffer+4,buflen-4);
                                string s = (const char*)str;
                                printf("s is %s\n",s.c_str());
                                endpoint.send(s);
                            }
                        }
                    }
                }
                else if(ev->data.fd == unxclient_fd){
                    //接收web服务数据再发送给tcp服务器
                    while(1){
                        unsigned char buffer[4096];
                        int size = recv(ev->data.fd, buffer, 4096, 0);
                        if(!size)
                        {
                            // close();
                            break;
                        }
                        if(size < 0)
                        {
                            if(EAGAIN != errno)
                                printf("this conn is bad %d\n",unxclient_fd);
                            break;
                        }
                        for(int i =0;i<size;i++)
                                      {
                                               printf("%02x ",buffer[i]);
                                          }
                        int datalen = 0;
                        datalen = ntohl(*(unsigned int*)buffer);
                        printf("size is %d datalen is %d\n",size,datalen);
                        while(size<datalen)
                        {
                            unsigned char recvbuff2[datalen-size];
                            bzero(recvbuff2,sizeof(recvbuff2));
                            int re = recv(ev->data.fd,recvbuff2,datalen-size,0);
                            memcpy(buffer+size,recvbuff2,re);
                            size += re;
                        }
                        unsigned char sendbuf[datalen+13];
                        memcpy(sendbuf,webname,13);
                        memcpy(sendbuf+13,buffer,datalen);
                        //封装141包和多播包
                        char *args[3]= {0};
                        int argsize = strsplit((char*)buffer+4,"+",args,3);
                        //拿到要发送的网关id
                        printf("send to gwid is %s wxid is %s\n",args[1],args[2]);
                        int idlen = strlen(args[1]);
                        unsigned char* pack_content = NULL;   //多播包封装内容
                        unsigned char* send_pack = NULL;  //141封装内容
                        int duobo_pack_content_size = 0;  //封装后大小
                        //多播
                        duobo_pack_content_size = fengzhuang(&pack_content,sendbuf,datalen,(unsigned char*)args[0],idlen);
                        printf("duobopack size is %d\n",duobo_pack_content_size);
                        //141
                        int send_pack_size = buff(pack_content,duobo_pack_content_size,&send_pack); 
                        printf("141size%d\n",send_pack_size);
                        ::send(tcpclient_fd,send_pack,send_pack_size,0);
                    }
                }
            }
        }
        //tcp服务器注册函数
        void tcpserver_login(){
            unsigned char b[40]={0};
            b[0] = 0x01;
            time_t  time1 = time(NULL);
            srand(time1);
            int suijima= rand();
            suijima = htonl(suijima);
            memcpy(b+1,(char*)&suijima,sizeof(suijima));
            short nlen=13;
            nlen = htons(nlen);
            memcpy(b+1+sizeof(suijima),(char* )&nlen,SHORT_LEN);
            unsigned char* buffer=NULL;
            unsigned char* buffer2 =(unsigned char*)malloc(16);
            int len2 = create_md5((unsigned char*)webname,NAME_LEN,buffer2,_miyue);
            memcpy((char*)b + 1 + sizeof(suijima) + SHORT_LEN,webname,NAME_LEN);
            memcpy((char*)b + 1 + sizeof(suijima) + SHORT_LEN + NAME_LEN,buffer2,len2);
            int length = 1+sizeof(suijima) + SHORT_LEN + NAME_LEN + len2;
            int len = buff(b,length,&buffer);
            ::send(tcpclient_fd,(char*)buffer,len,0);
        }
        void websock_client(){
            endpoint.connect("ws://localhost:9002");
        }
        //client连接tcp服务器
        void tcp_conn(const char* server, unsigned short port)
        {
            _server = server;
            _port = port;
            tcpclient_fd     = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            struct sockaddr_in sa;
            sa.sin_family   = AF_INET;
            sa.sin_port     = htons(_port);
            struct hostent *ht = gethostbyname(_server.c_str());
            memcpy(&sa.sin_addr, ht->h_addr, ht->h_length);
            if(connect(tcpclient_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
            {
                printf("connect tcpserver error\n");
                close(tcpclient_fd);
                tcpclient_fd = 0;
            }
            else{
                int sock_opts   = fcntl(tcpclient_fd, F_GETFL);
                sock_opts       |= O_NONBLOCK;
                fcntl(tcpclient_fd, F_SETFL, sock_opts); 
                //加入epoll
                struct epoll_event ev = {0};
                ev.data.fd  = tcpclient_fd;
                ev.events   = EPOLLIN | EPOLLET;
                epoll_ctl(_fpEpoll, EPOLL_CTL_ADD,tcpclient_fd, &ev);
            }
        }
        //心跳封装
        void heart_send(){
                        if(tcpclient_fd == 0){
                            printf("reconn\n");
                            tcp_conn(_server.c_str(),_port);
                            tcpserver_login();
                        }
                        unsigned char b[1] ={0};
                        b[0] = 0x01; 
                        string _userId = string((char*)webname,3);
                        unsigned long long ll =ntohll(*(unsigned long long*)(webname+3));
                        char ltos2[16] ={0};
                        sprintf(ltos2,"%lld",ll);
                        _userId += string(ltos2);
                        printf("heart id %s\n",_userId.c_str());
                        unsigned char* pack_content = NULL;   //多播包封装内容
                        unsigned char* send_pack = NULL;  //141封装内容
                        int duobo_pack_content_size = 0;  //封装后大小
                        //多播
                        duobo_pack_content_size = fengzhuang(&pack_content,b,1,(unsigned char*)_userId.c_str(),_userId.size());
                        printf("duobopack size is %d\n",duobo_pack_content_size);
                        //141
                        int send_pack_size = buff(pack_content,duobo_pack_content_size,&send_pack); 
                        printf("141size%d\n",send_pack_size);
                        int re;
                        unsigned char res[10] = {0};
                        re = recv(tcpclient_fd,res,10,0);
                        if(!re){
                            close(tcpclient_fd);
                            tcpclient_fd = 0;
                        }
                        if(re < 0){
                            if(EAGAIN != errno){
                               close(tcpclient_fd);
                               tcpclient_fd = 0; 
                            }
                        }
                        ::send(tcpclient_fd,send_pack,send_pack_size,0);
        }
};
#endif
