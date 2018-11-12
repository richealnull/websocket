#include <iostream>
#include <map>
#include <exception>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <pthread.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/un.h>
#include <arpa/inet.h>
#include "utility.h"
#define UNIX_DOMAIN "/tmp/UNIX.domain"

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

struct connection_data {
    int sessionid;
    std::string name;
};

class print_server {
    public:
       // print_server() : m_next_sessionid(1) {
        print_server(){
            m_server.init_asio();
            m_server.set_open_handler(boost::bind(&print_server::on_open,this,::_1));
            m_server.set_close_handler(boost::bind(&print_server::on_close,this,::_1));
            m_server.set_message_handler(boost::bind(&print_server::on_message,this,::_1,::_2));
        }
        //连接unx服务端
        void unxconnect(){ 
            struct sockaddr_un srv_addr;
            unxclient_fd=socket(PF_UNIX,SOCK_STREAM,0);  
            if(unxclient_fd<0)  
            {  
                perror("cannot create communication socket");  
            }     
            srv_addr.sun_family=AF_UNIX;  
            strcpy(srv_addr.sun_path,UNIX_DOMAIN);  
            //connect server  
            int ret=connect(unxclient_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));  
            if(ret==-1)  
            {  
                perror("cannot connect to the server");  
                close(unxclient_fd);    
            }
            struct epoll_event eve = {0};
            eve.data.fd  = unxclient_fd;
            eve.events   = EPOLLIN | EPOLLET;
            epoll_ctl(_fpEpoll, EPOLL_CTL_ADD,unxclient_fd, &eve);
        }
        void on_open(connection_hdl hdl) {
            //connection_data data;
            std::string data;
            //data.sessionid = m_next_sessionid++;
            //data.name = "";
            data = "";
            m_connections[hdl] = data;
        }
        void on_close(connection_hdl hdl) {
            //connection_data& data = get_data_from_hdl(hdl);
            std::string data = get_data_from_hdl(hdl);
            //std::cout << "Closing connection " << data.name 
              //  << " with sessionid " << data.sessionid << std::endl;
            std::cout<<"closing connecting "<<data<<std::endl;
            m_connections.erase(hdl);
            m_connections2.erase(data);
        }
        // 重写on_message接收的数据类型  c
        void on_message(connection_hdl hdl, server::message_ptr msg) {
            std::string data = get_data_from_hdl(hdl);
            std::string str = msg->get_payload();
            int len = str.size();
            std::cout<<"data is"<<str<<"datalen is"<<len<<std::endl;
            unsigned char buf[len];
            unsigned char sendbuf[len+4];
            memset(buf,0,len);
            memset(sendbuf,0,len);
            memcpy(sendbuf+4,str.c_str(),len);
            memcpy(buf,str.c_str(),len);
            printf("buf is %s\n",buf);
            char *args[2]   = {0};
            int size = strsplit((char*)buf,"+", args,20);
            printf("buf is %s\n",buf);
            printf("first str is %s second str is %s args size is %d\n",args[0],args[1],size);
            string head = string((char*)args[0],2);
            if(strcmp(args[0],"LOGIN") == 0){
                if(data == ""){
                    data = args[1];
                    m_connections[hdl] = data;
                    m_connections2[data] = hdl;
                    std::cout<<"user"<<data<<"was login"<<std::endl;
                }
                else{
                    data = args[1];
                     m_connections[hdl] = data;
                     m_connections2[data] = hdl;
                     std::cout<<"user"<<data<<"was login"<<std::endl;
                }
            }
            //else if(strcmp(args[0],"send") == 0){
            else if(head.compare("GW")== 0){
                int sendlen = htonl(len+4);
                memcpy(sendbuf,(char*)&sendlen,4);
                ::send(unxclient_fd,sendbuf,len+4,0);
            }
            //else if(strcmp(args[0],"towx")){
            else if((head.compare("MN")) == 0){
            con_list2::iterator it = m_connections2.find(args[0]);
            if (it != m_connections2.end()) {
                //转发给微信小程序
                m_server.send(it->second, str, websocketpp::frame::opcode::text);
            }
            }
        }
        std::string get_data_from_hdl(connection_hdl hdl){
            con_list::iterator it = m_connections.find(hdl);
            if (it == m_connections.end()) {
                throw std::invalid_argument("No data avaliable for session");
            }
            return it->second;
        }
        void run(uint16_t port) {
            m_server.listen(port);
            m_server.start_accept();
            m_server.run();
        }
    private:
        //typedef std::map<connection_hdl,connection_data> con_list;
        typedef std::map<connection_hdl,std::string> con_list;
        typedef std::map<std::string,connection_hdl> con_list2;

        //int m_next_sessionid;
        server m_server;
        con_list m_connections;
        con_list2 m_connections2;
        int _fpEpoll;
        int unxclient_fd;
};
/*int main() {
  print_server server;
  server.run(9002);
  }*/

