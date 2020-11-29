#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

// User表的数据操作类
class UserModel
{
public:
    //user表的增加方法
    bool insert(User &user);
    //根据userid删除好友
    bool remote(int id,int friendid);
    //根据用户号码查询用户消息
    User query(int id);
    //更新用户的状态信息
    bool updataState(User user);
    //更新用户的密码信息
    bool updataPassword(int id,string password);
    //重置用户的状态信息
    bool resetState();
};

#endif