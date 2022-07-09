#include "mysql_conn.h"

mysql_connect* mysql_connect::get_instance(){
    static mysql_connect instance;
    return &instance;
}

mysql_connect::mysql_connect(){
    m_MaxConn = 0;
    m_CurConn = 0;
}

mysql_connect::~mysql_connect(){
    destory_pool();
}

void mysql_connect::init(string url, string user, string password, string data_base_name, int port, int max_conn){
    m_url = url;
    m_user = user; 
	m_port = port;		        
	m_passWord = password;	        
	m_databaseName = data_base_name;      
    m_MaxConn = max_conn;

    for(int i=0; i< m_MaxConn; i++){
        MYSQL* con = NULL;
		con = mysql_init(con);
        con = mysql_real_connect(con, url.c_str(), user.c_str(), password.c_str(), data_base_name.c_str(), port, NULL, 0);
        connList.push_back(con);
        ++m_FreeConn;
    }
    sem_var = sem(m_FreeConn);
    m_MaxConn = m_FreeConn;
}

int mysql_connect::get_free_conn(){
    return this->m_FreeConn;
}

void mysql_connect::destory_pool(){
    lock.lock();
	if (connList.size() > 0)
	{
		list<MYSQL *>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it)
		{
			MYSQL *con = *it;
			mysql_close(con);
		}
		m_CurConn = 0;
		m_FreeConn = 0;
		connList.clear();
	}
	lock.unlock();
}

MYSQL* mysql_connect::get_connection(){
    if (0 == connList.size())
        return NULL;
    sem_var.wait();
    lock.lock();

    MYSQL *con = connList.front();
    connList.pop_front();

    --m_FreeConn;
	++m_CurConn;

    lock.unlock();
    return con;
}

bool mysql_connect::release_connection(MYSQL* &conn){
    if (NULL == conn || connList.size() == m_MaxConn)
		return false;

    lock.lock();

	connList.push_back(conn);
	++m_FreeConn;
	--m_CurConn;
    conn = NULL;        //防止外部再一次调用
	lock.unlock();

	reserve.post();
	return true;
}

connectionRAII::connectionRAII(MYSQL **con, mysql_connect *connPool){
	*con = connPool->get_connection();

	conRAII = *con;
	poolRAII = connPool;
}

connectionRAII::~connectionRAII(){
	poolRAII->release_connection(conRAII);
}

