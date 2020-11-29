#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;
using json = nlohmann::json;

//记录当前系统登录的用户信息
User currentUser;
//记录当前用户的好友列表信息
vector<User> currentUserFriendList;
//记录当前登录用户的群组列表信息
vector<Group> currentUserGroupsList;
//控制聊天页面程序
static bool isMainMenuRuning = false;
//显示当前登录成功用户的基本信息
void showCurrentUserData();
//接收线程
void ReadTaskHandler(int cliendfd);
//获取系统时间(聊天信息需要添加时间信息)
string getCurrentTime();
//主聊天页面程序
void mainMenu(int clientfd);

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    //解析通过命令行参数传递的ip+port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    //创建client端的socket
    int clientfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    sockaddr_in server;
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    socklen_t len = sizeof(server);
    if (-1 == ::connect(clientfd, (struct sockaddr *)&server, len))
    {
        cerr << "connect server error" << endl;
        exit(-1);
    }

    //main线程用于接收用户输入,负责发送数据
    bool isDisplayFirstMune = true;
    for (;;)
    {
        int choice = 0;
        if (isDisplayFirstMune)
        {
            //显示首页面菜单 登录 注册 退出
            cout << "======================" << endl;
            cout << "1.login" << endl;
            cout << "2.register" << endl;
            cout << "3.quit" << endl;
            cout << "======================" << endl;
            cout << "choice:";
            cin >> choice;
            cin.get(); //读掉缓冲区残留的回车
        }

        switch (choice)
        {
        case 1:
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid : ";
            cin >> id;
            cin.get(); //读掉缓冲区残留的回车
            cout << "userpassword : ";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (-1 == len)
            {
                cerr << "send login msg error:" << request << endl;
            }
            else
            {
                char buff[1024] = {0};
                //状态信息 + 好友列表 + 离线消息
                len = recv(clientfd, buff, sizeof(buff), 0);
                if (-1 == len)
                {
                    cerr << "recv login msg error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buff);
                    if (0 != responsejs["error"].get<int>()) //登录失败
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else //登录成功
                    {
                        //记录当前用户的id和name
                        currentUser.setId(responsejs["id"].get<int>());
                        currentUser.setName(responsejs["name"]);

                        //记录当前用户的好友列表
                        if (responsejs.contains("friends"))
                        {

                            //初始化
                            currentUserFriendList.clear();
                            vector<string> vec = responsejs["friends"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                currentUserFriendList.push_back(user);
                            }
                        }

                        //记录当前的群组列表消息
                        if (responsejs.contains("groups"))
                        {
                            vector<string> vec1 = responsejs["groups"];
                            for (string &groupStr : vec1)
                            {
                                json groupjs = json::parse(groupStr);
                                Group group;
                                group.setId(groupjs["id"].get<int>());
                                group.setName(groupjs["groupname"]);
                                group.setDesc(groupjs["groupdesc"]);

                                //初始化
                                currentUserGroupsList.clear();
                                vector<string> vec2 = groupjs["users"];
                                for (string &userstr : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }
                                currentUserGroupsList.push_back(group);
                            }
                        }

                        //显示登陆用户的基本信息
                        showCurrentUserData();

                        //显示当前用户的离线消息
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                int msgType = js["msgid"].get<int>();
                                if (ONE_CHAT_MSG == msgType)
                                {
                                    cout << js["time"] << "[" << js["id"] << "]" << js["name"]
                                         << "said" << js["msg"] << endl;
                                }
                                else
                                {
                                    cout << "群消息[" << js["groupid"] << "]" << js["time"] << "["
                                         << js["id"] << "]" << js["name"] << "said" << js["msg"] << endl;
                                }
                            }
                        }

                        //登陆成功,启动接收线程负责接收数据 该线程只启动一次
                        static int threadNum = 0;
                        if (threadNum++ == 0)
                        {
                            std::thread readTask(ReadTaskHandler, clientfd);
                            readTask.detach();
                        }
                        isMainMenuRuning = true;
                        //进入主聊天页面
                        mainMenu(clientfd);
                    }
                }
            }
        }
        break;
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, sizeof(name));
            cout << "userpassword:";
            cin.getline(pwd, sizeof(pwd));

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (-1 == len)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else
            {
                char buff[1024] = {0};
                len = recv(clientfd, buff, sizeof(buff), 0);
                if (-1 == len)
                {
                    cerr << "recv reg msg error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buff);
                    if (0 != responsejs["error"].get<int>())
                    {
                        cerr << name << "is already exist,resister error!" << endl;
                    }
                    else
                    {
                        cout << name << "resgister success,userid id" << responsejs["id"]
                             << ",do not forget it!" << endl;
                    }
                }
            }
        }
        break;
        case 3:
        {
            isDisplayFirstMune = false;
            close(clientfd);
            exit(-1);
        }
        break;
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }
    return 0;
}

//显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "======================login user============================" << endl;
    cout << "current login user => id" << currentUser.getId() << "name:" << currentUser.getName() << endl;
    cout << "----------------------friend list---------------------------" << endl;
    if (!currentUserFriendList.empty())
    {
        for (User &user : currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------------" << endl;
    if (!currentUserGroupsList.empty())
    {
        for (Group &group : currentUserGroupsList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "============================================================" << endl;
}
//接收线程
void ReadTaskHandler(int clientfd)
{
    for (;;)
    {
        char buff[1024] = {0};
        int len = recv(clientfd, buff, sizeof(buff), 0);
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }
        json js = json::parse(buff);
        int msgType = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgType)
        {
            cout << js["time"] << "[" << js["id"] << "]" << js["name"]
                 << "said" << js["msg"] << endl;
            continue;
        }
        if (GROUP_CHAT_MSG == msgType)
        {
            cout << "群消息[" << js["groupid"] << "]" << js["time"] << "[" << js["id"] << "]" << js["name"] << "said" << js["msg"] << endl;
            continue;
        }
    }
}
//获取系统时间(聊天信息需要添加时间信息)
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

void help(int clientfd = 0, string str = " ");
void chat(int, string);
void addfriend(int, string);
void remfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);
void modifypassword(int,string);

unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令,格式help"},
    {"chat", "一对一聊天,格式chat:friendid:message"},
    {"addfriend", "添加好友,格式addfriend:friendid"},
    {"remfriend","删除好友,格式:remfriend:friendid"},
    {"modifypassword","修改密码,格式modifypassword:password"},
    {"creategroup", "创建群组,格式creategroup:groupname:groupdesc"},
    {"addgroup", "添加群组,格式addgroup:groupid"},
    {"groupchat", "群聊,格式groupchat:groupid:message"},
    {"loginout", "注销,格式loginout"}};
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"remfriend",remfriend},
    {"modifypassword",modifypassword},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};
    
//主聊天页面程序
void mainMenu(int clientfd)
{
    help();

    char buff[1024] = {0};
    while (isMainMenuRuning)
    {
        cin.getline(buff, 1024);
        string commandbuf(buff);
        string command;
        int index = commandbuf.find(":");
        if (-1 == index)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, index);
        }

        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }
        it->second(clientfd, commandbuf.substr(index + 1, commandbuf.size() - index));
    }
}
void help(int clientfd, string str)
{
    cout << "show command list" << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << ":" << p.second << endl;
    }
    cout << endl;
}
//聊天
void chat(int clientfd, string str)
{
    int index = str.find(":");
    if (-1 == index)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, index).c_str());
    string message = str.substr(index + 1, str.size() - index);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = currentUser.getId();
    js["name"] = currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string request = js.dump();
    int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send one chat msg error ->" << request << endl;
    }
}
//添加好友
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = currentUser.getId();
    js["friendid"] = friendid;
    string request = js.dump();

    int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg error ->" << request << endl;
    }
}
//删除好友
void remfriend(int clientfd,string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = REM_MSG;
    js["id"] = currentUser.getId();
    js["friendid"] = friendid;
    string request = js.dump();

    int len = send(clientfd,request.c_str(),strlen(request.c_str()) + 1,0);
    if(-1 == len)
    {
        cerr << "send remfriend msg error ->" << request << endl;
    }
}
//修改好友密码
void modifypassword(int client,string str)
{
    json js;
    js["msgid"] = MODPW_MSG;
    js["id"] = currentUser.getId();
    js["password"] = str.c_str();
    string request = js.dump();
    
    int len = send(client,request.c_str(),strlen(request.c_str()) + 1,0);
    if(-1 == len)
    {
        cerr << "send modifypassword msg error ->" << request << endl;
    }
}
//创建群组
void creategroup(int clientfd, string str)
{
    int index = str.find(":");
    if (-1 == index)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }

    string groupName = str.substr(0, index);
    string groupDesc = str.substr(index + 1, str.size() - index);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = currentUser.getId();
    js["groupname"] = groupName;
    js["groupdesc"] = groupDesc;
    string request = js.dump();

    int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg error ->" << request << endl;
    }
}
//添加群组
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = currentUser.getId();
    js["groupid"] = groupid;
    string request = js.dump();

    int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroup msg error ->" << request << endl;
    }
}
//群聊
void groupchat(int clientfd, string str)
{
    int index = str.find(":");
    if (-1 == index)
    {
        cerr << "groupchat command invalid!" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, index).c_str());
    string message = str.substr(index + 1, str.size() - index);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = currentUser.getId();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string request = js.dump();

    int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send group chat msg error ->" << request << endl;
    }
}
//客户端退出
void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = currentUser.getId();
    string request = js.dump();

    int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send loginout msg error ->" << request << endl;
    }
    else
    {
        isMainMenuRuning = false;
    }
}
