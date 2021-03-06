#include "usermodel.hpp"
#include "db.hpp"

#include <iostream>
using namespace std;

//User表的增加方法
bool UserModel::insert(User &user)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name,password,state) values('%s','%s','%s');",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.updata(sql))
        {
            //获取插入成功的用户数据生成的主键id     //有问题代解决   连接池如何进行添加
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

//根据用户号码查询用户消息
User UserModel::query(int id)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from User where userid=%d;", id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));  //userId
                user.setName(row[1]);      //userName
                user.setPassword(row[2]);  //userPassword
                user.setState(row[3]);     //userState

                mysql_free_result(res);
                return user;
            }
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

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.updata(sql))
        {
            return true;
        }
    }
    return false;
}

//重新用户的状态信息
bool UserModel::resetState()
{
     //组装sql语句
    char sql[1024] = "update User set state = 'offline' where state = 'online'";

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.updata(sql))
        {
            return true;
        }
    }
    return false;
}