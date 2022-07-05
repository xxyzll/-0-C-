#ifndef TIMER_LIST_H
#define TIMER_LIST_H

#include <iostream>
#include <time.h>
#include <unordered_map>
#include <vector>
#include <signal.h>
#include <string.h>

using namespace std;
// 链表节点定义
class node{
public:
    node(): 
        pre(NULL), 
        next(NULL),
        expire_time(time(NULL)),
        fd(-1){};

    node(int fd): 
        pre(NULL), 
        next(NULL),
        expire_time(time(NULL)),
        fd(fd){};

    node* pre;
    node* next;
    int fd;
    time_t expire_time;                             //到期时间
};

// 到期后返回需要关闭的文件描述符
class timer_list
{
public:
    timer_list(unsigned int t);
    ~timer_list();
    void addsig(int sig, void(handler)(int), bool restart = true);
    bool add_timer(int fd, unsigned int num_time_slot= 1);         //添加一个定时器
    bool update_node(int fd, unsigned int num_time_slot= 1);       //更新一个节点
    vector<int> scan();                                            //扫描节点
private:
    node begin;                                     //创建开始和结束节点
    node end;
    unsigned int TIMESLOT;                          //单位时间
    unsigned int num_time_slot;                     //多少单位时间
    unordered_map<int, node*> node_addr;            //fd到地址的映射

    bool insert_to_end(node *n);                    //插入节点到末尾
    bool del_node(node* n);                         //删除一个节点
};

#endif