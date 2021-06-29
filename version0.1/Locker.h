#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>
#include "noncopyable.h"

// 封装 信号量 的类
class Sem : public noncopyable {

public:
	// 创建并初始化 信号量
	Sem()
	{
		if (sem_init(&m_sem, 0, 0) != 0 )
		{
			throw std::exception();
		}
	}
	// 销毁 信号量
	~Sem()
	{
		sem_destroy(&m_sem);
	}

	// 等待信号量
	bool wait()
	{
		return sem_wait(&m_sem) == 0;
	}
	// 增加信号量
	bool post()
	{
		return sem_post(&m_sem) == 0;
	}

private:
	sem_t m_sem;
};


// 封装 互斥锁 的类
class MutexLocker : public noncopyable {

public:
	// 创建并初始化 互斥锁
	MutexLocker()
	{
		if (pthread_mutex_init(&m_mutex, NULL) != 0)
		{
			throw std::exception();
		}
	}
	// 销毁 互斥锁
	~MutexLocker()
	{
		pthread_mutex_destroy(&m_mutex);
	}

	// 获取 互斥锁
	bool lock()
	{
		return pthread_mutex_lock(&m_mutex) == 0;
	}
	// 释放 互斥锁
	bool unlock()
	{
		return pthread_mutex_unlock(&m_mutex) == 0;
	}


private:
	pthread_mutex_t m_mutex;
};


class MutexLockerGuard : public noncopyable {

public:
	MutexLockerGuard(MutexLocker &locker):locker_(locker) {
		locker_.lock();
	}
	~MutexLockerGuard() {
		locker_.unlock();
	}

private:
	MutexLocker &locker_; 
};






// 封装 条件变量 的类
class Cond : public noncopyable {

public:
	// 创建并初始化 条件变量
	Cond()
	{
		if ( pthread_mutex_init(&m_mutex, NULL) != 0 )
		{
			throw std::exception();
		}
		if (pthread_cond_init(&m_cond, NULL) != 0)
		{
			// 构造函数一旦出现问题， 就应该立即释放已经成功分配了的资源
			pthread_mutex_destroy(&m_mutex);
			throw std::exception();
		}
	}
	// 销毁 条件变量
	~Cond()
	{
		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_cond);
	}
	// 等待条件变量
	bool wait()
	{
		int ret = 0;
		pthread_mutex_lock(&m_mutex);
		ret = pthread_cond_wait(&m_cond, &m_mutex);
		pthread_mutex_unlock(&m_mutex);
		return ret == 0;
	}
	// 唤醒等待条件变量的线程
	bool signal()
	{
		return pthread_cond_signal(&m_cond) == 0;
	}


private:
	pthread_mutex_t m_mutex;
	pthread_cond_t m_cond;
};

#endif