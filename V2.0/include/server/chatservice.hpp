#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "usermodel.hpp"
#include "friendmodel.hpp"
#include "offlinemessagemodel.hpp"
#include "groupmodel.hpp"

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "redis.hpp"
#include "json.hpp"
using json = nlohmann::json;

//业务模块代码
//存贮回调函数
using MsgHandler = std::function<void(const TcpConnectionPtr &conn,json &js,Timestamp &time)>;

//聊天服务器业务类
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService* getInstance();
    
    //处理登录业务
    void login(const TcpConnectionPtr &conn,json &js,Timestamp);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn,json &js,Timestamp);
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn,json &js,Timestamp);
    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn,json &js,Timestamp);
    //删除好友业务
    void remFriend(const TcpConnectionPtr &conn,json &js,Timestamp);
    //修改用户密码业务
    void modifypassword(const TcpConnectionPtr &conn,json &js,Timestamp);
    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn,json &js,Timestamp);
    //加入群组业务
    void addGroup(const TcpConnectionPtr &conn,json &js,Timestamp);
    //群聊业务
    void groupChat(const TcpConnectionPtr &conn,json &js,Timestamp);
    //处理注销业务
    void loginout(const TcpConnectionPtr &conn,json &js,Timestamp);
    //服务器异常,业务重置方法
    void reset();
    //处理客户端异常退出
    void clientCloseException(TcpConnectionPtr conn);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);  
    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);
private:
    //注册消息以及对应的业务处理方法
    ChatService();
    //存消息id和其对应的业务方法 key:msgid value:function
    unordered_map<int,MsgHandler> _msgHandlerMap;  
    //存贮在线的用户连接
    unordered_map<int,TcpConnectionPtr> _userConnectionMap;
    //定义互斥锁,保证_userConnectionMap的线程安全
    mutex _connMutex;
    //数据操作类对象
    UserModel _userModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
    OfflineMsgModel _offlineMsgModel;
    //rsdis操作对象
    Redis _redis;
};

#endif