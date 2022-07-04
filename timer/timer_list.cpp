#include "timer_list.h"

timer_list::timer_list(unsigned int t){
    TIMESLOT = t;
    begin.next = &end;
    end.pre = &begin;
}

timer_list::~timer_list()
{
    // 删除begin-end之间所有节点
    node* work = begin.next;
    while(work!= &end){
        node* need_del = work;
        work = work->next;
        delete need_del;
    }
}

bool timer_list::add_timer(int fd, unsigned int num_time_slot){
    if(node_addr.find(fd) != node_addr.end()){
        return update_node(fd, num_time_slot);
    }
    node *n = new node(fd);
    n->expire_time = time(NULL) + num_time_slot*TIMESLOT;
    if(!insert_to_end(n))
        return false;
    node_addr[fd] = n;
    return true;
}

bool timer_list::update_node(int fd, unsigned int num_time_slot){
    // 没有这个节点
    if(node_addr.find(fd) == node_addr.end())
        return false;
    node* work = node_addr[fd];
    work->pre->next = work->next;
    work->next->pre = work->pre;
    work->expire_time = time(NULL) + num_time_slot * TIMESLOT;
    return insert_to_end(work);
}

bool timer_list::insert_to_end(node *n){
    node* pre_node = end.pre;
    n->next = &end;
    n->pre = pre_node;
    end.pre = n;
    pre_node->next = n;
    return true;
}

bool timer_list::del_node(node* n){
    if(!n)
        return false;
    n->pre->next = n->next;
    n->next->pre = n->pre;
    delete n;
    node_addr.erase(n->fd);
    return true;
}

vector<int> timer_list::scan(){
    vector<int> fds;
    node* work = begin.next;
    time_t cur_time = time(NULL);
    while(work!= &end && work->expire_time< cur_time){
        node* need_del = work;
        work = work->next;
        fds.push_back(need_del->fd);
        del_node(need_del);
    }
    return fds;
}
