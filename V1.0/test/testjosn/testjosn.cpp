#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

//json序列化示例1
string func1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello,what are you doing now?";

    string sendBuf = js.dump();  //转化为字符串
    //cout << sendBuf.c_str() << endl;
    return sendBuf;
}

//json序列化示例2
string func2()
{
    json js;
    //添加数组
    js["id"] = {1,2,3,4,5};
    js["name"] = "zhang san";
    js["msg"]["zhang san"] = "hello world!";  //3
    js["msg"]["liu shuo"] = "hello china!";   //4
    //这条语句和上面的3和4语句是一样的,json数据对象和map一样不支持重复
    js["msg"] = {{"zhang san","hello world"},{"liu shuo","hello china!"}};
    
    string sendBuf = js.dump();
    return sendBuf;
}

//josn序列化示例3
string func3()
{
    json js;
    //直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;

    //直接序列化一个map容器
    std::map<int,std::string> map;
    map[1] = "黄山";
    map[2] = "华山";
    js["path"] = map;
    //cout << js << endl;
    string sendBuf = js.dump();
    return sendBuf;
}
int main()
{
    string recvBuf = func3();
    //数据的反序列化 json字符串 ==> 反序列化 ==> 数据对象
    json jsbuf = json::parse(recvBuf);
    // cout << jsbuf["msg_type"] << endl;
    // cout << jsbuf["from"] << endl;
    // cout << jsbuf["to"] << endl;
    // cout << jsbuf["msg"] << endl;

    // cout << jsbuf["id"] << endl;
    // auto vec = jsbuf["id"];
    // cout << vec[2] << endl;
    // cout << jsbuf["name"] << endl;

    auto msgjs = jsbuf["msg"];
    cout << msgjs["zhang san"] << endl;
    cout << msgjs["liu shuo"] << endl;
    
    vector<int> vec = jsbuf["list"];
    for(int val : vec){
        cout << val << " ";
    }
    cout << endl;
    map<int,string> map = jsbuf["path"];
    return 0;
}
