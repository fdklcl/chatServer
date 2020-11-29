#ifndef COMMENCONNECTIONPOOL_H
#define COMMENCONNECTIONPOOL_H
#include <iostream>
#include <string>
#include <stdio.h>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include <functional>
#include <condition_variable>
#include "connection.hpp"
using namespace std;
/*
线程安全的懒汉单例模式
*/
class ConnectionPool
{
public:
	//获取连接池对象实例
	static ConnectionPool* getConnectionPool()
	{
		//静态局部对象运行到此处的时候才产生对象 自动lock()和unlock()操作
		static ConnectionPool pool;  
		return &pool;
	}

	//从连接池中获取一个空闲连接
	shared_ptr<Connection> getConnection();
private:
	ConnectionPool();  //构造函数私有化

	//从配置文件中加载配置项
	bool loadConfigFile();

	//运行在独立地线程中,专门负责生产新连接
	void produceConnectionTask();

	//扫描超过maxIdleTime时间的空闲连接,进行多余的连接回收
	void scannerConnectionTask();

	string _ip;               //mysql的ip地址
	unsigned short _port;     //mysql的端口号,默认3306
	string _username;         //mysql登录用户名
	string _password;         //mysql登录密码
	string _dbname;           //mysql数据库名称
	int _initSize;            //连接池的初始连接量
	int _maxSize;             //连接池的最大连接量
	int _maxIdleTime;         //连接池的最大空闲时间
	int _connectionTimeout;   //连接池获取连接的最大超时时间

	queue<Connection*> _connectionQue;   //存储mysql连接的队列
	mutex _queueMutex;         //维护连接队列的线程安全的互斥锁,保证_connectionQueue多线程安全
	atomic_int _connectionCnt; //记录连接所创建的connection连接的总数量,支持线程安全
	condition_variable cv;     //设置条件变量,用于生产者线程和消费者线程之间的通信
};
#endif