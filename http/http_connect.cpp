#include "http_connect.h"

// 定义HTTP响应的一些状态信息
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";

int http_connect::epoll_fd;
int http_connect::client_num = 0;
//设置非阻塞
void setnoblocking(int fd){
    int old_opt = fcntl(fd, F_GETFL);
    old_opt |= O_NONBLOCK;
    fcntl(fd, F_SETFL, old_opt);
}

//添加一个事件
void add_event(int epollfd, int fd, bool oneshort, bool ET_FIG){
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN| EPOLLRDHUP;
    if(oneshort){
        event.events |= EPOLLONESHOT;
    }
    if(ET_FIG){
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // setnoblocking(fd);
}

void mod_event(int epollfd, int fd, int eve, bool ET_FIG){
    struct epoll_event event;
    event.data.fd = fd;
    event.events = eve | EPOLLONESHOT | EPOLLRDHUP;
    if(ET_FIG)
        event.events |= EPOLLET;
    epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}

void remove_event( int epollfd, int fd ) {
    epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
    http_connect::client_num -= 1;
    close(fd);
}

// 返回false表示对面关闭连接或者读取出错
bool http_connect::read_all_data(){
    // while(true){
    //     if(readed_idx>= read_buff_size){
    //         cout<< "缓冲区满" << endl;
    //         return false;
    //     }
            
    //     int ret = recv(connect_fd, read_buff+readed_idx, read_buff_size-readed_idx, 0);
    //     if(ret == -1){
    //         // 在读取完的时候会返回
    //         if(errno == EAGAIN || errno == EWOULDBLOCK)
    //             break;
    //         cout<< "读取错误" << endl;
    //         return false;
    //     }else if(ret == 0){
    //         cout<< "对方关闭连接" << endl;
    //         return false;
    //     } 
    //     readed_idx += ret;
    // }
    int ret = recv(connect_fd, read_buff+readed_idx, read_buff_size-readed_idx, 0);
    if (ret <= 0)
    {
        cout << "error: num "<< error << endl;
        return false;
    }
    readed_idx += ret;
    
    return true;
}

void http_connect::init_connect(int connect_fd){
    //重置已经接收到的缓冲区
    readed_idx = 0;
    //重置已经接收到的字节数
    prase_idx = 0;
    //保存fd
    this->connect_fd = connect_fd;
    //请求体的长度
    content_length = 0;
    master_state = CHECK_STATE_REQUESTLINE;
    writed_idx = 0;
    keep_connect = false;
    file_address = 0;
    memset(read_buff, 0, read_buff_size);
    memset(write_buff, 0, write_buf_size);
}

http_connect::http_connect(int connect_fd){
    this->connect_fd = connect_fd;
    readed_idx = 0;
    prase_idx = 0;
    content_length = 0;
    master_state = CHECK_STATE_REQUESTLINE;
    writed_idx = 0;
    keep_connect = false;
    memset(read_buff, 0, read_buff_size);
    memset(write_buff, 0, write_buf_size);
}

// 解析一行数据, 解析完的数据以\0方式结尾
http_connect::LINE_STATUS http_connect::prase_line(){
    for(; prase_idx<= readed_idx; prase_idx++){
        if(read_buff[prase_idx] == '\r'){
            // 阅读到了'\r' 如果已经是结尾了,说明接受不完全
            if(prase_idx+1 == readed_idx){
                return LINE_OPEN;
            }else if(read_buff[prase_idx+1] == '\n'){
                // 解析完全
                read_buff[prase_idx] = '\0';
                read_buff[prase_idx+1] = '\0';
                // 直接跳过\r\n
                prase_idx += 2;
                return LINE_OK;
            }else
                return LINE_BAD;
        }else if(read_buff[prase_idx] == '\n'){
            if (prase_idx > 1 && read_buff[prase_idx - 1] == '\r')
            {
                read_buff[prase_idx - 1] = '\0';
                read_buff[prase_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

// 解析数据
http_connect::HTTP_CODE http_connect::process_read(){
    LINE_STATUS line_state = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    int line_start = 0;
    while((master_state == CHECK_STATE_CONTENT && line_state == LINE_OK)|| 
            (line_state = prase_line()) == LINE_OK){
        char* text = line_start + read_buff;
        line_start = prase_idx;
        switch(master_state){
            // 解析请求行
            case CHECK_STATE_REQUESTLINE:{
                ret = prase_request(text);
                if(ret == BAD_REQUEST)
                    return BAD_REQUEST;
            } break;
            case CHECK_STATE_HEADER:{
                ret = prase_head(text);
                if(ret == BAD_REQUEST)
                    return BAD_REQUEST;
                if(ret == GET_REQUEST)
                    return do_request();
            } break;
            // 下一步实现
            case CHECK_STATE_CONTENT:{
                cout<<"error not code the method"<< endl;
            }
            default:{
                cout<< "意外的情况" <<endl;
            }
        }
    }
    return NO_REQUEST;
}

http_connect::HTTP_CODE http_connect::prase_request(char *text){
    // 获取方法
    if(!get_method(text)){
        cout<< "method error" << endl;
        return BAD_REQUEST;
    }
    // 获取请求网址
    if(!get_url(text)){
        cout<< "url error" << endl;
        return BAD_REQUEST;
    }
    // 获取版本号
    if(!get_version(text)){
        cout<< "version error" << endl;
        return BAD_REQUEST;
    }   
    master_state = CHECK_STATE_HEADER;
}

void http_connect::process(){
    cout<< read_buff << endl;
    // 解析请求
    HTTP_CODE ret = process_read();
    if(ret == NO_REQUEST){
        cout<< "NO_REQUEST" << endl;
        mod_event(epoll_fd, connect_fd, EPOLLIN);
        return;
    }
    // 生成响应
    bool write_ret = process_write(ret);
    // 写入出错关闭连接
    if(!write_ret){
        close_connect();
        return;
    }
    mod_event( epoll_fd, connect_fd, EPOLLOUT);
}

bool http_connect::process_write(HTTP_CODE state){
    switch(state){
        case INTERNAL_ERROR:
            add_statue_line(500, error_500_form);
            add_header(strlen(error_500_form));
            if(!add_content(error_500_form))
                return false;
            break;
        case BAD_REQUEST:
            add_statue_line(400, error_400_form);
            add_header(strlen(error_400_form));
            if(!add_content(error_400_form))
                return false;
            break;
        case NO_REQUEST:
            add_statue_line(404, error_404_form);
            add_header(strlen(error_404_form));
            if(!add_content(error_404_form))
                return false;
            break;
        case FORBIDDEN_REQUEST:
            add_statue_line(403, error_403_form);
            add_header(strlen(error_403_form));
            if(!add_content(error_403_form))
                return false;
            break;
        case FILE_REQUEST:
            add_statue_line(200, ok_200_title);
            add_header(file_stat.st_size);
            m_iv[ 0 ].iov_base = write_buff;
            m_iv[ 0 ].iov_len = writed_idx;
            m_iv[ 1 ].iov_base = file_address;
            m_iv[ 1 ].iov_len = file_stat.st_size;
            m_iv_count = 2;
            return true;
            break;
        default:
            return false;
    }
    m_iv[ 0 ].iov_base = write_buff;
    m_iv[ 0 ].iov_len = writed_idx;
    m_iv_count = 1;
    return true;
}

bool http_connect::get_method(char*& text){
    char *method_str = text;
    text = strpbrk(method_str, " \t");
    if (! text) { 
        return false;
    }
    *text++ = '\0';
    for(auto m: methods){
        if ( strcasecmp(method_str, m.first) == 0 ) { // 忽略大小写比较
            req_method = m.second;
            return true;
        }
    }
    return false;
}

bool http_connect::get_url(char*& text){
    url = text;
    text = strpbrk(url, " \t");
    if (!text) { 
        return false;
    }
    *text++ = '\0';
    return true;
}

bool http_connect::get_version(char* text){
    version = text;
    return true;
}

bool http_connect::gethost(void* arg, char* text){
    http_connect* ptr = (http_connect*) arg;
    text = strpbrk(text, ":\t");
    if (! text) { 
        return false;
    }
    ptr->port = text+1;
    return true;
}

http_connect::HTTP_CODE http_connect::prase_head(char* text){
    // 进入下一个状态。如果为get请求直接do
    if(text[0] == '\0'){
        if(content_length == 0)
            return GET_REQUEST;
        master_state = CHECK_STATE_CONTENT;
    }

    char *key = text;
    text = strpbrk(text, ":\t");
    if (! text) { 
        return BAD_REQUEST;
    }
    // 跳过冒号和空格
    *text++ = '\0'; text++; 
    string key_str = key;
    // 这里用string比较，否则比较的是地址的值
    if(deal_head_funs.find(key_str)!= deal_head_funs.end() && !deal_head_funs[key_str]((void*)this, text)){
        return BAD_REQUEST;
    }
    // 继续接受key-value
    return NO_REQUEST;
}

http_connect::HTTP_CODE http_connect::do_request(){
    // 拼接文件名
    strcpy(real_file, doc_root);
    int len = strlen( doc_root );
    strncpy( real_file + len, url, max_file_len-len-1 );

    // 获取real_file文件的相关的状态信息，-1失败，0成功
    if ( stat( real_file, &file_stat ) < 0 ) {
        return NO_RESOURCE;
    }

    // 判断访问权限
    if(!(file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;
    
    // 判断是否是目录
    if ( S_ISDIR( file_stat.st_mode ) ) 
        return BAD_REQUEST;
    
    int fd = open( real_file, O_RDONLY );
    file_address = (char*)mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    // cout<< real_file << endl;
    return FILE_REQUEST;
}

bool http_connect::write_in_buff(const char* format, ...){
    if( writed_idx >= write_buf_size ) {
        return false;
    }
    va_list arg_list;
    va_start( arg_list, format );
    int len = vsnprintf( write_buff + writed_idx, write_buf_size-1-writed_idx, format, arg_list );
    if( len >= ( write_buf_size - 1 - writed_idx ) ) {
        return false;
    }
    writed_idx += len;
    va_end( arg_list );
    return true;
}

bool http_connect::add_statue_line(int status, const char* title ){
    return write_in_buff("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_connect::add_header(int content_len){
    // 长度
    write_in_buff("Content-Length: %d\r\n", content_len);
    // 类型
    write_in_buff("Content-Type: %s\r\n", "text/html");
    // 是否保持连接
    write_in_buff("Connection: %s\r\n", ( keep_connect == true ) ? "keep-alive" : "close");
    // 回车
    write_in_buff("%s", "\r\n");
}

bool http_connect::add_content(const char* content){
    return write_in_buff("%s", content);
}

void http_connect::close_connect(){
    if(connect_fd != -1){
        // cout<< "断开连接" << connect_fd << endl;
        remove_event(epoll_fd, connect_fd);
        // cout<< "client num: " << http_connect::client_num << endl;
        connect_fd = -1;
        unmap();
    }
}

bool http_connect::write(){
    int temp = 0;
    // int have_send = 0;              //已经发送的字节
    int need_to_send = get_unsend_len();  //需要发送的字节
    // cout<< "write buff: "<< write_buff << endl;
    if(need_to_send == 0){
        mod_event(epoll_fd, connect_fd, EPOLLIN);
        init_connect(connect_fd);
        return true;
    }

    while(1){
        temp = writev(connect_fd, m_iv, m_iv_count);
        if (temp < 0)
        {
            if (errno == EAGAIN)
            {
                mod_event( epoll_fd, connect_fd, EPOLLOUT );
                return true;
            }
            unmap();
            return false;
        }

        need_to_send -= temp;

        if(temp>= m_iv[0].iov_len){
            m_iv[1].iov_base = (char*)m_iv[1].iov_base + (temp-m_iv[0].iov_len);
            m_iv[1].iov_len = need_to_send;
            m_iv[0].iov_len = 0;
        }else{
            m_iv[0].iov_base = (char*)m_iv[0].iov_base+ temp;
            m_iv[0].iov_len -= temp;
        }
           
        if(need_to_send<= 0){
            unmap();
            if(keep_connect){
                init_connect(connect_fd);
                mod_event(epoll_fd, connect_fd, EPOLLIN);
                return true;
            }else{
                cout<< "close" << endl;
                mod_event(epoll_fd, connect_fd, EPOLLIN);
                return false;
            }
        }
    }
    return true;
}

int http_connect::get_unsend_len(){
    int ret = 0;
    for(int i=0; i< m_iv_count; i++){
        ret += m_iv[i].iov_len;
    }
    return ret;
}

void http_connect::unmap(){
    if(file_address){
        munmap(file_address, file_stat.st_size);
        file_address = 0;
    }
}
