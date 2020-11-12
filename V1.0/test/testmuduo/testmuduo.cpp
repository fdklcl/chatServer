#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <string>
#include <functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

//基于muduo网络库开发服务器程序 回调服务器
class ChatServer
{
public:
    ChatServer(EventLoop* loop,       //#3 输出ChatServer的构造函数
            const InetAddress& listenAddr,
            const string& nameArg)
            :_server(loop,listenAddr,nameArg),
            _loop(loop)
        {
            //#4 给服务器注册用户连接和断开回调
            _server.setConnectionCallback(bind(&ChatServer::onConnection,this,_1));
            //#4 给服务器注册用户读写事件回调
            _server.setMessageCallback(bind(&ChatServer::onMessage,this,_1,_2,_3));

            //#5 设置线程池线程数量
            _server.setThreadNum(4);
        }
        //开启事件循环
        void start(){
            _server.start();
        }
private:
    //专们处理用户的连接和断开  用户自己设定
    void onConnection(const TcpConnectionPtr& conn){
        if(conn->connected()){
            cout << conn->peerAddress().toIpPort() << "->" << 
                conn->localAddress().toIpPort() << "state:online" << endl;
        }
        else{
            cout << conn->peerAddress().toIpPort() << "->" << 
                conn->localAddress().toIpPort() << "state:offline" << endl;
            conn->shutdown();
        }
    }
    //准们用于处理读写事件
    void onMessage(const TcpConnectionPtr& conn,Buffer* buffer,Timestamp time){
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << "time" << time.toString() << endl;
        conn->send(buf);
    }
    TcpServer _server;  //#1 组合TcpServer对象
    EventLoop *_loop;   //#2 创建EventLoop事件循环对象的指针
};

int main(){
    EventLoop loop;
    InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"ChatServer");

    server.start();
    loop.loop();  //epoll_wait()以阻塞方式等待新用户连接,已连接用户的读写事件等
    return 0;
}