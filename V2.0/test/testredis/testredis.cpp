#include <string>
#include <stdio.h>
#include <hiredis/hiredis.h>
#include <iostream>
#include <stdlib.h>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

int main()
{
    redisContext *conn = redisConnect("127.0.0.1", 6379);
    if (conn != NULL && conn->err)
    {
        printf("connection error: %s\n", conn->errstr);
        return 0;
    }
    // json user;
    // user["id"] = 2;
    // user["name"] = "fandikang";
    // user["state"] = "offline";
    // cout << user << endl;
    //string sendbuf = user.dump();
    redisReply *reply = (redisReply *)redisCommand(conn, "set user %s","hello");
    //redisReply *reply = (redisReply *)redisCommand(conn, "rpush user 1 2 3");
    cout << reply->type << endl;
    freeReplyObject(reply);

    reply = (redisReply *)redisCommand(conn, "del user1");
    //reply = (redisReply *)redisCommand(conn, "lrange user 0 -1");

    //int length = reply->elements;
    //cout << length << endl;
    //for (int i = 0; i < length; ++i)
    //
        //string str = reply->element[i]->str;
        //int id = atoi(str.c_str());
        //cout << id << " ";
    //}
    //cout << endl;
    cout << reply->type << endl;
    cout << "--" << reply->integer << " --" << endl;
    //cout << reply->str << endl;
    //string recvbuf = reply->str;
    // json jsbuf = json::parse(recvbuf);
    //cout << jsbuf["name"] << endl;
    //cout << reply->type << endl;
    freeReplyObject(reply);

    redisFree(conn);
    return 0;
}
