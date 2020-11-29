#include <iostream>
#include "redis.hpp"
#include "json.hpp"
using namespace std;

//初始化
Redis::Redis()
    : _publish_context(nullptr), _subcribe_context(nullptr), _setGet_context(nullptr)
{
}
//释放资源
Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if (_subcribe_context != nullptr)
    {
        redisFree(_subcribe_context);
    }
    if (_setGet_context != nullptr)
    {
        redisFree(_setGet_context);
    }
}
//连接redis-server
bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _publish_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    _subcribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _subcribe_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    _setGet_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _setGet_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;

    return true;
}
//向redis中写入
bool Redis::setBaseInfo(int id, string str, int flag)
{
    if (flag == 1) //表示用户id
    {
        //组装key
        string key = "userBaseInfo:";
        string idStr = ::to_string(id);
        key += idStr;

        redisReply *reply = (redisReply *)redisCommand(_setGet_context, "set %s %s", key.c_str(), str.c_str());
        freeReplyObject(reply);
    }
    else //表示小组id
    {
        //组装key
        string key = "groupBaseInfo:";
        string idStr = ::to_string(id);
        key += idStr;

        redisReply *reply = (redisReply *)redisCommand(_setGet_context, "set %s %s", key.c_str(), str.c_str());
        freeReplyObject(reply);
    }
}
//删除redis中的基本信息
bool Redis::removeBaseInfo(int id,int flag)
{
    if (flag == 1) //表示用户id
    {
        //组装key
        string key = "userBaseInfo:";
        string idStr = ::to_string(id);
        key += idStr;

        redisReply *reply = (redisReply *)redisCommand(_setGet_context, "del %s", key.c_str());
        freeReplyObject(reply);
        return true;
    }
    else //表示小组id
    {
        //组装key
        string key = "groupBaseInfo:";
        string idStr = ::to_string(id);
        key += idStr;
        
        redisReply *reply = (redisReply *)redisCommand(_setGet_context, "del %s", key.c_str());
        freeReplyObject(reply);
    }
}
//从redis中获取值
User Redis::queryUserBaseInfo(int id,bool &flag)
{
    User user;
    //组装key
    string key = "userBaseInfo:";
    string idStr = ::to_string(id);
    key += idStr;

    redisReply *reply = (redisReply *)redisCommand(_setGet_context, "get %s", key.c_str());
    if (reply->type == 4)
    {
        flag = false;
        freeReplyObject(reply);
        return user;
    }
    else
    {
        string recvbuf = reply->str;
        json js = json::parse(recvbuf);

        user.setId(js["id"]);
        user.setName(js["name"]);
        user.setState(js["state"]);
        user.setPassword(js["password"]);
        flag = true;
        freeReplyObject(reply);
        return user;
    }
}
// 从redis中获取群组基本信息
Group Redis::queryGroupBaseInfo(int id)
{
    Group group;
    //组装key
    string key = "groupBaseInfo:";
    string idStr = ::to_string(id);
    key += idStr;

    redisReply *reply = (redisReply *)redisCommand(_setGet_context, "get %s", key.c_str());
    if (reply->type == 4)
    {
        freeReplyObject(reply);
        return group;
    }
    else
    {
        string recvbuf = reply->str;
        json js = json::parse(recvbuf);
        group.setId(js["id"]);
        group.setName(js["name"]);
        group.setDesc(js["desc"]);

        freeReplyObject(reply);
        return group;
    }
}

//向redis中写入其他信息
bool Redis::setInfo(int id, int Id, int flag)
{
    if (flag == 1) //表示用户的好友列表
    {
        string key = "userFriends:";
        string idStr = ::to_string(id);
        key += idStr;

        redisReply *reply = (redisReply *)redisCommand(_setGet_context, "rpush %s %d", key.c_str(), Id);
        freeReplyObject(reply);
    }
    else if (flag == 2) //表示好友的群组列表
    {
        string key = "userGroups:";
        string idStr = ::to_string(id);
        key += idStr;

        redisReply *reply = (redisReply *)redisCommand(_setGet_context, "rpush %s %d", key.c_str(), Id);
        freeReplyObject(reply);
    }
    else //群组的好友列表
    {
        string key = "groupFriends:";
        string idStr = ::to_string(id);
        key += idStr;

        redisReply *reply = (redisReply *)redisCommand(_setGet_context, "rpush %s %d", key.c_str(), Id);
        freeReplyObject(reply);
    }
}
//向redis中获取其他信息
vector<int> Redis::getInfo(int id, int flag)
{
    vector<int> res;
    if (flag == 1) //表示用户的好友列表
    {
        string key = "userFriends:";
        string idStr = ::to_string(id);
        key += idStr;

        redisReply *reply = (redisReply *)redisCommand(_setGet_context, "lrange %s 0 -1", key.c_str());
        int length = reply->elements;
        if (length == 0)
        {
            return res;
        }
        else
        {
            for (int i = 0; i < length; ++i)
            {
                string str = reply->element[i]->str;
                int id = atoi(str.c_str());
                res.push_back(id);
            }
        }
        freeReplyObject(reply);
        return res;
    }
    else if (flag == 2) //表示好友的群组列表
    {
        string key = "userGroups:";
        string idStr = ::to_string(id);
        key += idStr;

        redisReply *reply = (redisReply *)redisCommand(_setGet_context, "lrange %s 0 -1", key.c_str());
        int length = reply->elements;
        if (length == 0)
        {
            return res;
        }
        else
        {
            for (int i = 0; i < length; ++i)
            {
                string str = reply->element[i]->str;
                int id = atoi(str.c_str());
                res.push_back(id);
            }
        }
        freeReplyObject(reply);
        return res;
    }
    else //群组的好友列表
    {
        string key = "groupFriends:";
        string idStr = ::to_string(id);
        key += idStr;

        redisReply *reply = (redisReply *)redisCommand(_setGet_context, "lrange %s 0 -1", key.c_str());
        int length = reply->elements;
        if (length == 0)
        {
            return res;
        }
        else
        {
            for (int i = 0; i < length; ++i)
            {
                string str = reply->element[i]->str;
                int id = atoi(str.c_str());
                res.push_back(id);
            }
        }
        freeReplyObject(reply);
        return res;
    }
}
//根据用户号码查询用户消息

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}
// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    // redisGetReply

    return true;
}
// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}
// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->_subcribe_context, (void **)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}
//设置回调函数
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}