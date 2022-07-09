#ifndef LOCK_H
#define LOCK_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

// 锁封装
class locker{
public:

    locker(){
        pthread_mutex_init(&mutex, NULL);
    }
    
    ~locker(){
        pthread_mutex_destroy(&mutex);
    }

    bool lock(){
        return pthread_mutex_lock(&mutex) == 0;
    }
    bool unlock(){
        return pthread_mutex_unlock(&mutex) == 0;
    }

    pthread_mutex_t *get(){
        return &mutex;
    }


private:
    pthread_mutex_t mutex;
};

// 信号量封装
class sem{
public:
    sem(){
        sem_init(&sem_var, 0, 0);
    }
    sem(int val){
        sem_init(&sem_var, 0, val);
    }
    ~sem(){
        sem_destroy(&sem_var);
    }
    bool wait(){
        return sem_wait(&sem_var) == 0;
    }
    bool post(){
        return sem_post(&sem_var) == 0;
    }

    private:
    sem_t sem_var;
};


#endif