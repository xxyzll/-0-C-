#ifndef MYSQL_CONN_H
#define MYSQL_CONN_H

#include <iostream>
#include <string>
#include <list>
#include "../locker/locker.h"
#include <mysql/mysql.h>

using namespace std;

class mysql_connect{
public:
    static mysql_connect* get_instance();
    void init(string url, string user, string password, string data_base_name, int port, int max_conn);
	int get_free_conn();        //获取连接
	void destory_pool();		//销毁所有连接
	MYSQL* get_connection();	//获取一个连接
	bool release_connection(MYSQL* &conn); 			//释放连接

private:
	string m_url;			    //主机地址
	int m_port;		        	//数据库端口号
	string m_user;		        //登陆数据库用户名
	string m_passWord;	        //登陆数据库密码
	string m_databaseName;      //使用数据库名

    int m_MaxConn;  			//最大连接数
	int m_CurConn;  			//当前已使用的连接数
	int m_FreeConn; 			//当前空闲的连接数

	locker lock;
    sem sem_var;
	list<MYSQL *> connList; 	//连接池
	sem reserve;

    mysql_connect();
    ~mysql_connect();
};

class connectionRAII{

public:
	connectionRAII(MYSQL **con, mysql_connect *connPool);
	~connectionRAII();
	
private:
	MYSQL *conRAII;
	mysql_connect *poolRAII;
};


#endif
