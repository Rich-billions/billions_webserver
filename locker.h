#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

/*封装信号量*/
class sem {
public:
    /*初始化未名信号量*/
    sem() {
        if(sem_init(&m_sem, 0, 0) != 0) {
            throw std::exception();
        }
    }
    /*销毁信号量*/
    ~sem() {
        sem_destroy(&m_sem);
    }
    /*p操作，等待信号量*/
    bool wait() {
        return sem_wait(&m_sem) == 0;
    }
    /*v操作，增加信号量*/
    bool post() {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

/*封装互斥锁的类*/
class locker {
public:
    /*创建并初始化互斥锁*/
    locker() {
        if(pthread_mutex_init(&m_mutex, NULL) != 0) {
            throw std::exception();
        }
    }
    /*销毁互斥锁*/
    ~locker() {
        pthread_mutex_destroy(&m_mutex);
    }
    /*获取互斥锁*/
    bool lock() {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    /*释放互斥锁*/
    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

private:
    pthread_mutex_t m_mutex;
};

/*封装条件变量的类*/
class cond {
public:
    /*创建并初始化条件变量*/
    cond() {
        if(pthread_mutex_init(&m_mutex, NULL) != 0) {
            throw std::exception();
        }
        if(pthread_cond_init(&m_cond, NULL) != 0) {
            /*若构建条件变量出现问题，及时释放锁*/
            pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    /*销毁条件变量*/
    ~cond() {
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }
    /*等待条件变量*/
    bool wait() {
        int ret = 0;
        pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, &m_mutex);
        pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    /*唤醒等待条件变量的线程*/
    bool signal() {
        return pthread_cond_signal(&m_cond) == 0;
    }

private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

#endif