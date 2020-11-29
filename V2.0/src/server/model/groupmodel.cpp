#include "groupmodel.hpp"
#include "group.hpp"
#include "commenConnectionPool.hpp"
#include "connection.hpp"

#include <iostream>
using namespace std;

//创建群组 name desc
bool GroupModel::createGroup(Group &group)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup(groupname,groupdesc) values('%s','%s');",
            group.getName().c_str(), group.getDesc().c_str());

    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();
    if (sp->update(sql))
    {
        //获取插入成功的用户数据生成的主键id  
        //mysql_insert_id() 返回给定的 connection 中上一步 INSERT
        // 查询中产生的 AUTO_INCREMENT 的 ID 号。
        group.setId(mysql_insert_id(sp->getConnection()));
        return true;
    }
    return false;
}
//加入群组
bool GroupModel::addGroup(int userid, int groupid, string role)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser(groupid,userid,grouprole) values(%d,%d,'%s');",
            groupid, userid, role.c_str());

    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();
    if (sp->update(sql))
    {
        sp->update(sql);
        return true;
    }
    return false;
}
//查询用户所在群组信息  用户的好友列表不过包括用户在群中的状态(creator or normal)
vector<Group> GroupModel::queryGroups(int userid)
{
    //组装sql语句
    char sql[1024] = {0};
    //查询用户id加入了那些组,返回组id,组的名称,组的描述
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from AllGroup a inner join \
                GroupUser b on a.id = b.groupid where b.userid = %d",
            userid);

    vector<Group> groupVec;
    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();

    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            Group group;
            group.setId(atoi(row[0])); //groupId
            group.setName(row[1]);     //groupName
            group.setDesc(row[2]);     //groupDesc
            groupVec.push_back(group);
        }
        mysql_free_result(res);
    }

    for (Group &group : groupVec)
    {
        //查询组里有哪些成员 userId userName userState groupRole
        sprintf(sql, "select a.userid,a.name,a.state,b.grouprole from User a inner join GroupUser \
            b on b.userid = a.userid where b.groupid = %d",group.getId());

        MYSQL_RES *res = sp->query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0])); //userId
                user.setName(row[1]);     //userName
                user.setState(row[2]);    //userState
                user.setRole(row[3]);     //groupRole
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}
//根据指定的groupid查询群组用户id列表,除userid自己,主要用户群聊业务给群组其他成员发消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select userid from GroupUser where groupid = %d and userid != %d", groupid, userid);

    vector<int> idVec;
    ConnectionPool *p = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> sp = p->getConnection();

    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            idVec.push_back(atoi(row[0]));
        }
        mysql_free_result(res);
    }
    return idVec;
}