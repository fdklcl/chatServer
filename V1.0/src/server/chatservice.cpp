#include "chatservice.hpp"
#include "public.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <iostream>
#include <memory>
#include <vector>
//获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}
//注册消息以及对应的业务处理方法
ChatService::ChatService()
{
    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    //连接redis服务器
    if (_redis.connect())
    {
        //设置上报消息的回调
        _redis.init_notify_handler(std::bind(&::ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}
//服务器异常,业务重置方法
void ChatService::reset()
{
    //把online转态的用户
    _userModel.resetState();
}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    //msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())//说明没找到
    { 
        //返回一个默认的处理器,空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << "msgid:" << msgid << "can not find handler!";
        };
    }
    return _msgHandlerMap[msgid];
}
//处理登录业务 id passward
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    //获取用户id和密码
    int id = js["id"].get<int>();
    string password = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPassword() == password)
    {
        if (user.getState() == "online")  //如果该用户存在且处于上线状态
        {
            //用户已经登录,不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["error"] = 2;
            response["errmsg"] = "id already exist,please repeat cin!";
            //数据序列化
            conn->send(response.dump());
        }
        else
        {
            //登录成功,记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnectionMap.insert({id, conn});
            }

            //id用户登录成功后,向redis订阅channel(id)
            //用自己的id注册一个channel
            _redis.subscribe(id);

            //登录成功,更改用户状态信息
            user.setState("online");
            _userModel.updataState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK; //msgid
            response["error"] = 0;             //error
            response["id"] = user.getId();     //userId
            response["name"] = user.getName(); //userName
            //查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                //读取该用户的离线消息后,把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }
            //查询用户的好友信息并且返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json is;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }
            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }
            //数据序列化
            conn->send(response.dump());
        }
    }
    else
    {
        //登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["error"] = 1;
        response["errmsg"] = "id or password error,please repeat it!";
        //数据序列化
        conn->send(response.dump());
    }
}
//处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);
    bool state = _userModel.insert(user);
    if (state)
    { //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["error"] = 0;
        response["id"] = user.getId();
        //数据序列化
        conn->send(response.dump());
    }
    else
    { //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["error"] = 1;
        //数据序列化
        conn->send(response.dump());
    }
}
//处理客户端异常退出
void ChatService::clientCloseException(TcpConnectionPtr conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnectionMap.begin();
        for (; it != _userConnectionMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                //从_userConnectionMap中删除用户的连接信息
                _userConnectionMap.erase(it);
                break;
            }
        }
    }

    //用户注销,相当于下线,在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    //更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updataState(user);
    }
}
//处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnectionMap.find(userid);
        if (it != _userConnectionMap.end())
        {
            _userConnectionMap.erase(it);
        }
    }
    //用户注销,相当于下线,在redis中取消订阅通道
    _redis.unsubscribe(userid);

    //更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updataState(user);
}
//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnectionMap.find(toid);
        if (it != _userConnectionMap.end())
        {
            //toid在线,转发消息   服务主动转发消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }
    //查询toid是否在线
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }
    //toid不在线,存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

//添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    //存储好友信息
    _friendModel.insert(userid, friendid);
    _friendModel.insert(friendid, userid);
}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        //存储群组创建人消息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}
//群聊业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    //查询出该群组有哪些用户,当然需要排除自己
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnectionMap.find(id);
        if (it != _userConnectionMap.end())
        {
            //转发群消息
            it->second->send(js.dump());
        }
        else
        {
            //查询toid是否在线
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
                return;
            }
            else
            {
                //存储离线消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnectionMap.find(userid);
    if (it != _userConnectionMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}