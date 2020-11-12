#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"

#include <functional>
#include <string>

using namespace std;
using json = nlohmann::json;
//网络模块
ChatServer::ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg)
            :_server(loop,listenAddr,nameArg)
            ,_loop(loop)
        {
            //注册连接回调
            _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
            //注册消息回调
            _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
            //设置线程数量
            _server.setThreadNum(4);
        }
//开启事件循环
void ChatServer::start(){
    _server.start();
}

//处理客户连接和断开
void ChatServer::onConnection(const TcpConnectionPtr& conn){
    //客户端断开连接
    if(!conn->connected()){
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

//处理读写消息事件
void ChatServer::onMessage(const TcpConnectionPtr& conn,Buffer* buffer,Timestamp time){
    string buf = buffer->retrieveAllAsString();
    //数据的反序列化
    json js = json::parse(buf);
    //回调消息绑定好的事件处理器,并执行相应的业务逻辑
    ChatService::instance()->getHandler(js["msgid"].get<int>())(conn,js,time);
}