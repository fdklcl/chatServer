#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client共有部分
*/

enum EnMsgTypeP{
    LOGIN_MSG = 1,  // 登录消息
    LOGIN_MSG_ACK,  //登录响应消息
    REG_MSG,        //注册消息
    REG_MSG_ACK,    //注册响应消息
    ONE_CHAT_MSG,   //聊天消息
    ADD_FRIEND_MSG, //添加好友消息
    REM_MSG,        //删除好友信息
    REM_MSG_ACK,    //删除好友回应消息
    MODPW_MSG,      //修改密码消息
    MODPW_MSG_ACK,  //修改密码回应消息

    CREATE_GROUP_MSG,//创建群组
    ADD_GROUP_MSG,   //加入群组
    GROUP_CHAT_MSG,  //群组聊天

    LOGINOUT_MSG,  //注销消息
};

#endif