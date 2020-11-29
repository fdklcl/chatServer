#include "offlinemessagemodel.hpp"
#include "connection.hpp"
#include "commenConnectionPool.hpp"

//存储用户的离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into OfflineMessage values(%d,'%s');",userid,msg.c_str());

    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();
    sp->update(sql);
}

//删除该用户所有的消息
void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from OfflineMessage where userid=%d",userid);
    
    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();
    sp->update(sql);
}

//查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"select message from OfflineMessage where userid=%d;",userid);

    vector<string> vec;
    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();
 
    MYSQL_RES *res = sp->query(sql);
    if(res != nullptr)
    {
         MYSQL_ROW row;
        while((row = mysql_fetch_row(res)) != nullptr)
        {
            vec.push_back(row[0]); 
        }
        mysql_free_result(res);
        return vec;
    }
    return vec;
}