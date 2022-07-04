#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H

#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <vector>
#include <iostream>

template<typename T>
class th_pool{
public:
    static void* work(void *arg){
        th_pool* ptr = (th_pool*)arg;
        while(!ptr->kill_pool){
            sem_wait(&ptr->sem_var);
            pthread_mutex_lock(&ptr->pth_lock_var);
            T* t = ptr->tasks.front();
            ptr->tasks.pop();
            pthread_mutex_unlock(&ptr->pth_lock_var);
            t->process();
            // std::cout<< "一个线程处理" << std::endl;
        }
    }

    //创建线程，每个线程执行work函数
    th_pool(int size){
        //初始化信号量并设置为0
        sem_init(&sem_var, 0, 0);
        //初始化互斥量
        pthread_mutex_init(&pth_lock_var, NULL);
        kill_pool = false;

        this->size = size;
        pthread_ids.resize(size);

        for(int i=0; i< size; i++){
            pthread_create(&pthread_ids[i], NULL, work, this);
            pthread_detach(pthread_ids[i]);
        }
    }

    ~th_pool(){
        pthread_mutex_destroy(&pth_lock_var);
        sem_destroy(&sem_var);
    }

    // 向阻塞队列中添加任务, 注意传过来的参数必须是堆区的
    bool add_task(T* t){
        pthread_mutex_lock(&pth_lock_var);
        tasks.push(t);
        sem_post(&sem_var);
        pthread_mutex_unlock(&pth_lock_var);
        return true;
    }

private:
    std::vector<pthread_t> pthread_ids;
    std::queue<T*> tasks;

    //结束线程
    bool kill_pool;
    //互斥量
    pthread_mutex_t pth_lock_var;   
    //信号量
    sem_t sem_var;
    //大小
    int size;    
};
#endif