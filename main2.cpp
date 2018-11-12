#include <iostream>
#include <string>
#include <sstream>
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

#include "websocket_endpoint.h"

int main(int argc, char **argv)
{
    bool done = false;
    std::string input;
    kagula::websocket_endpoint endpoint;

    endpoint.connect("ws://localhost:9002");

    while (!done) {
        std::cout << "Enter Command: ";
        std::getline(std::cin, input);

        if (input == "quit") {
            //done = true;
                char b[12]={0};
                //unsigned char* buffer = NULL;
                char* name = (char*)"MNP9999";
                b[0] = 0x01;
                int idlen = 7;
                idlen = htonl(idlen);
                memcpy(b+1,(char*)&idlen,4);
                memcpy(b+5,name,7);
                std::string str =b;
                int sizes = str.size();
                printf("data len is %d\n",sizes);
                printf("data is %s\n",str.c_str());
                endpoint.send(str);
        }
        else if (input.substr(0, 4) == "send") {
            std::stringstream ss(input);

            std::string cmd;
            std::string message;

            ss >> cmd;
            std::getline(ss, message);
            std::string lo = "login";
            if(message.compare(lo) == 0){
                printf("??????????????\n");
                 char b[12]={0};
                //unsigned char* buffer = NULL;
                 char* name = (char*)"MNP9999";
                b[0] = 0x01;
                int idlen = 7;
                idlen = htonl(idlen);
                memcpy(b+1,(char*)&idlen,4);
                memcpy(b+5,name,7);
                std::string str = (char*)b;
                endpoint.send(str);
            }
            else if(message.compare("duobo") == 0){
                char c[30] = {0};
                char* buffer = NULL;
                char* wgname = (char*)"GWM3777";
                char* name = (char*)"MNP9999";
                c[0] = 0x02;
                int wgidlen =7;
                wgidlen = htonl(wgidlen);
                memcpy(c+1,(char*)&wgidlen,4);
                memcpy(c+5,wgname,7);
                memcpy(c+12,(char*)&wgidlen,4);
                memcpy(c+16,name,7);
                std::string str =(char*)c;
                endpoint.send(str);

            }
            //加个包头？

            //endpoint.send(message);
        }
        else if (input.substr(0, 4) == "show") {
            endpoint.show();
        }
        else {
            std::cout << "> Unrecognized Command" << std::endl;
        }
    }

    endpoint.close();

    return 0;
}

