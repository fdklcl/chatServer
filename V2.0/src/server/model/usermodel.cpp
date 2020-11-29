#include "usermodel.hpp"
#include "connection.hpp"
#include "commenConnectionPool.hpp"

#include <iostream>
using namespace std;

//User表的增加方法
bool UserModel::insert(User &user)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name,password,state) values('%s','%s','%s');",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());

    //MySQL mysql;
    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();

    if (sp->update(sql))
    {
        //获取插入成功的用户数据生成的主键id     //有问题代解决   连接池如何进行添加
        user.setId(mysql_insert_id(sp->getConnection()));
        return true;
    }
    return false;
}

//删除好友
bool UserModel::remote(int id,int friendid)
{
    char sql[1024] = {0};
    sprintf(sql,"delete from Friend where userid = %d and friendid = %d",id,friendid);
    
    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();

    if(sp->update(sql))
    {
        return true;
    }
    return false;
}

//根据用户号码查询用户消息
User UserModel::query(int id)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from User where userid = %d;", id);

    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();
    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr)
        {
            User user;
            user.setId(atoi(row[0])); //userId
            user.setName(row[1]);     //userName
            user.setPassword(row[2]); //userPassword
            user.setState(row[3]);    //userState

            mysql_free_result(res);
            return user;
        }
    }
    //默认userId = -1;
    return User();
}
//更新用户的状态信息
bool UserModel::updataState(User user)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update User set state = '%s' where userid = %d;", user.getState().c_str(), user.getId());

    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();
    
    if (sp->update(sql))
    {
        return true;
    }
    return false;
}
//更新用户的密码信息
bool UserModel::updataPassword(int id,string password)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update User set password = '%s' where userid = %d;", password.c_str(), id);

    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();
    
    if (sp->update(sql))
    {
        return true;
    }
    return false;
}
//重置用户的状态信息
bool UserModel::resetState()
{
    //组装sql语句
    char sql[1024] = "update User set state = 'offline' where state = 'online'";

    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();
    
    if (sp->update(sql))
    {
        return true;
    }
    return false;
}