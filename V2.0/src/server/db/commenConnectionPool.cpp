#include "commenConnectionPool.hpp"
#include <muduo/base/Logging.h>
#include <string.h>

//从配置文件中加载配置项
bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("/home/sweet/github/chatServer/V1.0/src/server/db/mysql.cnf", "r");
	if (nullptr == pf)
	{
		LOG_ERROR << "mysql.cnf file is not exist!";
		return false;
	}
	//cout << "文件打开成功" << endl;
	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find('=', 0);
		if (idx == -1) //无效配置项
		continue;

		int endidx = str.find('\n', idx);
		string key = str.substr(0, idx);  
		string value = str.substr((idx + 1), (endidx - idx - 1));

		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port = atoi(value.c_str());
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
		else if (key == "dbname")
		{
			_dbname = value;
		}
		else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeout")
		{
			_connectionTimeout = atoi(value.c_str());
		}
	}
	return true;
}

//连接池的构造
ConnectionPool::ConnectionPool()
{
	//加载配置项
	if (!loadConfigFile())
	{
		return;
	}
	//创建初始数量的连接
	for (int i = 0; i < _initSize; ++i)
	{
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->refreshAliveTime();    //刷新一下开始空闲的起始时间
		_connectionQue.push(p);
		_connectionCnt++;
	}

	//创建一个新线程,作为连接的生产者
	/*
	线程函数都是C接口,produceConnectionTask是普通的成员方法,该
	成员方法的调用需要依赖对象,因此需要使用绑定器
	*/
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));  
	produce.detach();

	//启动一个新的定时线程,扫描超过maxIdleTime时间的空闲连接,进行多余的连接回收
	thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}

//运行在独立地线程中,专门负责生产新连接
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty())
		{
			//等待队列 => 阻塞队列 =>获取互斥锁才能继续进行
			cv.wait(lock);   //#1等待状态 #2释放互斥锁
		}

		//若队列为空,连接数量没有到达上限,继续创建新的连接
		if (_initSize < _maxSize)
		{
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAliveTime();    //刷新一下开始空闲的起始时间
			_connectionQue.push(p);
			_connectionCnt++;
		}

		//通知消费者线程可以连接了 等待状态=>阻塞状态
		cv.notify_all();
	}
}

//给外部提供一个接口.从连接池中获取一个空闲连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty())
	{
		//wait_for()函数和sleep()函数的区别
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
		{
			if (_connectionQue.empty())
			{
				LOG_ERROR << "获取连接超时...获取连接失败";
				return nullptr;
			}
		}
	}
	/*
	shared_ptr智能指针析构时,会把connection资源直接delete掉,相当于调
	用connection的析构函数,关闭连接,但我们实际上不希望真正的关闭连接,
	而是在使用完后,将连接归还到连接池中,因此需要自定义智能指针的删除器
	*/
	shared_ptr<Connection> sp(_connectionQue.front()
		, [&](Connection* pcon) 
	{
		unique_lock<mutex> lock(_queueMutex);
		pcon->refreshAliveTime();    //刷新一下开始空闲的起始时间
		_connectionQue.push(pcon);
	});
	_connectionQue.pop();
	cv.notify_all();  //消费完连接以后,通知生产者线程检查队列,若为空,接着生产
	
	return sp;
}

//扫描超过maxIdleTime时间的空闲连接, 进行多余的连接回收
void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		//通过sleep_for模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

		//扫描整个队列,释放对于的连接
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() >= (_maxIdleTime * 1000))
			{
				_connectionQue.pop();
				_connectionCnt--;
				delete p;    //调用~Connection()释放连接
			}
			else
			{
				break;       //队头的连接没有超过_maxIdleTime,其他的连接肯定没有超过
			}
		}
	}
}