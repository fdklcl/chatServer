#include "groupmodel.hpp"
#include "group.hpp"
#include "db.hpp"

#include <iostream>
using namespace std;


//创建群组 name desc
bool GroupModel::createGroup(Group &group)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup(groupname,groupdesc) values('%s','%s');",
            group.getName().c_str(),group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.updata(sql))
        {
            //获取插入成功的用户数据生成的主键id     //有问题代解决   连接池如何进行添加
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
//加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser(groupid,userid,grouprole) values(%d,%d,'%s');",
            groupid,userid,role.c_str());
    
    MySQL mysql;
    if (mysql.connect())
    {
       mysql.updata(sql);
    }
}
//查询用户所在群组信息  用户的好友列表不过包括用户在群中的状态(creator or normal)
vector<Group> GroupModel::queryGroups(int userid)
{
    //组装sql语句
    char sql[1024] = {0};
    //查询用户id加入了那些组,返回组id,组的名称,组的描述
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from AllGroup a inner join \
                GroupUser b on a.id = b.groupid where b.userid = %d",userid);
        
    vector<Group> groupVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));  //groupId
                group.setName(row[1]);    //groupName
                group.setDesc(row[2]);    //groupDesc
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    for(Group &group : groupVec)
    {
        //查询组里有哪些成员 userId userName userState groupRole 
        sprintf(sql,"select a.userid,a.name,a.state,b.grouprole from User a inner join GroupUser \
            b on b.userid = a.userid where b.groupid = %d",group.getId());
        
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));   //userId
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
    sprintf(sql,"select userid from GroupUser where groupid = %d and userid != %d",groupid,userid);
        
    vector<int> idVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}