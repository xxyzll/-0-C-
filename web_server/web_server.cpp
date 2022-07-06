#include "web_server.h"

int tiny_web_server::pipefd[2];
int tiny_web_server::listen_fd;

void tiny_web_server::sigalrm_handler(int sig){
    write(pipefd[1], &sig, sizeof(sig));
    // cout<< "定时到" << sig << endl;
}

tiny_web_server::tiny_web_server(){
    // 建立监听套接字
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 设置io复用
    int used = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &used, sizeof(used));

    // 绑定端口
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10000);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, listen_size);  

    //创建epoll任务,size被忽略
    http_connect::epoll_fd = epoll_create(128);
    //添加监听事件
    add_event(http_connect::epoll_fd, listen_fd, false);
    //连接请求初始化
    connect_list = new http_connect[MAX_FD_NUM];
    //线程池初始化
    work_pool = new th_pool<http_connect>(pthread_num);
    //定时链表
    time_connect = new timer_list(time_val_alarm);
 
    //信号捕获
    time_connect->addsig(SIGPIPE, SIG_IGN);
    time_connect->addsig(SIGALRM, sigalrm_handler, false);
    time_connect->addsig(SIGTERM, sigalrm_handler, false);

    //定时器
    set_timer();

    //开启管道
    socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd);
    //设置写端非阻塞
    setnoblocking(pipefd[1]);
    //监听读端
    add_event(http_connect::epoll_fd, pipefd[0], false);
}

tiny_web_server::~tiny_web_server(){
    close(listen_fd);
    close(http_connect::epoll_fd);
}

void tiny_web_server::set_timer(){
    alarm(time_val_alarm);
}

void tiny_web_server::run(){
    bool timeout = false;
    while(1){
        int event_count = epoll_wait(http_connect::epoll_fd, events, max_event_size, -1);
        // cout<< "event_count :" << event_count << endl;
        if ( ( event_count < 0 ) && ( errno != EINTR ) ) {
            printf( "epoll failure\n" );
            break;
        }
        for(int i=0; i< event_count; i++){
            int event_fd = events[i].data.fd;
            //新的客户端连接
            if(event_fd == listen_fd){
                // cout<< "检测到客户端" << endl;
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof( client_address );
                int connfd = accept( listen_fd, ( struct sockaddr* )&client_address, &client_addrlength );
                add_event(http_connect::epoll_fd, connfd, true);
                //初始化连
                connect_list[connfd].init_connect(connfd);
                time_connect->add_timer(connfd);
                // cout<< "客户端已经连接" << endl;
                http_connect::client_num += 1;
            } else if(event_fd == pipefd[0]){
                // 定时到了
                char signals[1024];
                recv(pipefd[0], signals, sizeof(signals), 0);
                timeout = true;
            }
            else{
                // 有数据可以读
                if(events[i].events & EPOLLIN ){
                    time_connect->update_node(event_fd);
                    if(connect_list[event_fd].read_all_data()){
                        work_pool->add_task(&connect_list[event_fd]);
                    }else{
                        perror("read::::::::");
                        //出错或者对面关闭了,关闭连接
                        connect_list[event_fd].close_connect();
                    }
                }
                else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) ) {
                    perror("error::::::::");
                    connect_list[event_fd].close_connect();
                }else if( events[i].events & EPOLLOUT ) {
                    time_connect->update_node(event_fd);
                    if( !connect_list[event_fd].write() ) {
                        connect_list[event_fd].close_connect();
                    }
                }
            }
        }    
        //检查超时
        if(timeout){
            chack_connect();
            timeout = false;
        }
    }
}

void tiny_web_server::chack_connect(){
    alarm(time_val_alarm);
    vector<int> over_time_fd = time_connect->scan();
    for(int i=0; i< over_time_fd.size(); i++){
        if(connect_list[over_time_fd[i]].connect_fd != -1){
            connect_list[over_time_fd[i]].close_connect();
            cout << "定时关闭连接" << endl;
        }
    }
}
