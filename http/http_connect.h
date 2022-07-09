#ifndef HTTP_CONNECT_H
#define HTTP_CONNECT_H

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <error.h>
#include <string.h>
#include <vector>
#include <utility>
#include <unordered_map>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/uio.h>
#include "../database/mysql_conn.h"

using namespace std;

void setnoblocking(int fd);
void add_event(int epollfd, int fd, bool oneshort, bool ET_FIG=false);
void mod_event(int epollfd, int fd, int eve, bool ET_FIG=false);
void remove_event( int epollfd, int fd );

struct login_par{
    char* user;
    char* password;
};
typedef struct login_par login_par;

class http_connect{
public:
    #define read_buff_size  1024
    #define max_file_len    200
    #define write_buf_size  1024
    // 网站的根目录
    const char* doc_root = "/home/xx/c++code/resource";
    static int epoll_fd;
    static int client_num;
    int connect_fd;                         
    // HTTP请求方法，这里只支持GET
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};
    /*
        解析客户端请求时，主状态机的状态
        CHECK_STATE_REQUESTLINE:当前正在分析请求行
        CHECK_STATE_HEADER:当前正在分析头部字段
        CHECK_STATE_CONTENT:当前正在解析请求体
    */
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    /*
        服务器处理HTTP请求的可能结果，报文解析的结果
        NO_REQUEST          :   请求不完整，需要继续读取客户数据
        GET_REQUEST         :   表示获得了一个完成的客户请求
        BAD_REQUEST         :   表示客户请求语法错误
        NO_RESOURCE         :   表示服务器没有资源
        FORBIDDEN_REQUEST   :   表示客户对资源没有足够的访问权限
        FILE_REQUEST        :   文件请求,获取文件成功
        INTERNAL_ERROR      :   表示服务器内部错误
        CLOSED_CONNECTION   :   表示客户端已经关闭连接了
    */
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    // 从状态机的三种可能状态，即行的读取状态，分别表示
    // 1.读取到一个完整的行 2.行出错 3.行数据尚且不完整
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

    // 连接的fd
    http_connect(int connect_fd);
    // 默认fd
    http_connect():connect_fd(-1){};
    // 处理请求
    void process();
    // 读完所有数据
    bool read_all_data();
    //初始化连接
    void init_connect(int connect_fd);

    void close_connect();
    bool write();
private:
    // 读取数据相关
    int readed_idx;                         // 当前已经读到缓冲区索引
    char read_buff[read_buff_size];         // 读取缓冲区
    char write_buff[write_buf_size];        // 写缓冲区
    int writed_idx;                         // 当前应该写入的位置
    METHOD req_method;                      // 请求的方法
    char *version;                          // 请求的版本
    char *url;                              // 请求的路径
    char *port;                             // 端口
    char real_file[max_file_len];           // 真实文件的位置
    bool keep_connect;                      // 是否保持连接
    char *file_address;                     // 共享内存的地址
    struct stat file_stat;
    struct iovec m_iv[2];                   // 分散写               
    int m_iv_count;
    void unmap();
    char* referer;                          // 请求来源
    login_par user;                         // 登录参数

    int content_length;                     // 请求体的长度，默认为0
    int prase_idx;                          // 当前解析的idx
    CHECK_STATE master_state;               // 主状态机状态
    LINE_STATUS prase_line();               // 解析一行数据
    HTTP_CODE process_read();               // 解析数据
    HTTP_CODE prase_request(char* text);    // 解析请求行
    HTTP_CODE prase_head(char* text);       // 解析请求头
    HTTP_CODE prase_body(char* text);       // 解析请求体

    bool process_write(HTTP_CODE state);    // 写回响应
    bool write_in_buff(const char* format, ...);                            // 写入缓冲
    bool add_statue_line(int status, const char* title );                   // 写入状态行
    bool add_header(int content_len);       // 写入头标识
    bool add_content(const char* content);  // 写入内容
    int get_unsend_len();

    vector<pair<char*, METHOD>> methods = {{"GET", GET}, {"POST", POST}};   // 支持的请求方法
    vector<char*> versions = {"http/1.1", "http/2.1"};                      // 支持的版本号
    bool get_method(char*& text);           // 获取请求求方法
    bool get_version(char *text);           // 获取版本
    bool get_url(char*& text);              // 获取目录

    // 注册即可，无需动解析部分
    static bool gethost(void* arg, char*text);                               // 获取端口
    static bool getreferer(void* arg, char*text);                            // 获取请求的来源
    static bool getContent_length(void* arg, char*text);                     // 获取正文长度
    // 头处理函数集合
    unordered_map<string, bool (*)(void*, char*)> deal_head_funs = {{"Host", gethost}, 
                                                                    {"Referer", getreferer},
                                                                    {"Content-Length", getContent_length}};

    // 响应函数处理函数集合 
    unordered_map<METHOD, http_connect::HTTP_CODE (*)(void*)> recall_function = {{GET, deal_with_get}, 
                                                                                 {POST, deal_with_post}};
    static http_connect::HTTP_CODE deal_with_get(void*);                    // 处理get请求
    static http_connect::HTTP_CODE deal_with_post(void*);                   // 处理post请求

    static bool get_user_name(void* arg, char* text);                       // 得到user名
    static bool get_user_password(void* arg, char* text);                   // 得到user密码

    // 请求体处理函数集合
    unordered_map<string, bool (*)(void*, char*)> deal_body_funs = {{"user", get_user_name},
                                                                    {"password", get_user_password}};

    static http_connect::HTTP_CODE log_action(void* arg);
    static http_connect::HTTP_CODE welcome_action(void* arg);
    // post响应
    unordered_map<string, HTTP_CODE (*)(void*)> post_actions = {{"/user_input", log_action},
                                                                {"/picture.html", welcome_action},
                                                                {"/video.html", welcome_action},
                                                                {"/fans.html", welcome_action}};
};
#endif
