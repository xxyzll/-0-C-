#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <sys/epoll.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>

#include "../thread_pool/thread_pool.h"
#include "../http/http_connect.h"
#include "../timer/timer_list.h"


//半连接队列大小
#define listen_size 12
//最大事件个数
#define max_event_size 10000
//最大文件描述符数量
#define MAX_FD_NUM 65535
#define time_val_alarm 3

class tiny_web_server{
public:
    #define pthread_num 24
    static int listen_fd;
    static int pipefd[2];           // 定时器管道0:读 1:写

    tiny_web_server();
    ~tiny_web_server();

    void run();

private:
    // 事件队列
    epoll_event events[max_event_size];
    // 工作线程
    th_pool<http_connect>* work_pool;
    // 连接队列
    http_connect* connect_list;
    // 时间链表
    timer_list* time_connect;
    // 定时器
    void set_timer();
    // 定时器回调函数
    static void sigalrm_handler(int sig);
    // 检查连接
    void chack_connect();
};
#endif