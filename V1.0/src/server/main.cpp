#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
using namespace std;

void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "argv error" << endl;
        exit(-1);
    }

    //解析通过命令行参数传递的ip+port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT,resetHandler);

    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"charServer");

    server.start();
    loop.loop();
    
    return 0; 
}
