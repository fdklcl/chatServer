#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

#include "user.hpp"
#include "group.hpp"
#include "json.hpp"
using json = nlohmann::json;

class Redis
{
public:
    Redis();
    ~Redis();
    // 连接redis服务器
    bool connect();
    //--------------------------------------------------------------
    //redis缓存模块
    //向redis中写入基本信息
    bool setBaseInfo(int id, string str, int flag);
    //从redis中获取用户基本信息
    User queryUserBaseInfo(int id,bool &flag);
    //从redis中获取群组基本信息
    Group queryGroupBaseInfo(int id);
    //向redis中写入其他信息
    bool setInfo(int id, int Id, int flag);
    //向redis中获取其他信息
    vector<int> getInfo(int id, int flag);
    //删除redis中的基本信息
    bool removeBaseInfo(int id,int flag);
    //----------------------------------------------------------------

    //----------------------------------------------------------------
    //redis 发布与订阅模块
    // 向redis指定的通道channel发布消息
    bool publish(int channel, string message);
    // 向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);
    // 向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);
    // 在独立线程中接收订阅通道中的消息
    void observer_channel_message();
    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int, string)> fn);
    //------------------------------------------------------------------
private:
    // hiredis同步上下文对象，负责publish消息
    redisContext *_publish_context;
    // hiredis同步上下文对象，负责subscribe消息
    redisContext *_subcribe_context;
    // hiredis同步上下文对象，负责上传与获取消息
    redisContext *_setGet_context;
    // 回调操作，收到订阅的消息，给service层上报
    function<void(int, string)> _notify_message_handler;
    //存储list中key的index
    unordered_map<string, int> _index_map;
};

#endif