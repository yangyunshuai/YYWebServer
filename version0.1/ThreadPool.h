
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <vector>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include <iostream>
#include <functional>

#include "Locker.h"

const int MAX_THREAD_SIZE = 1024;
const int MAX_QUEUE_SIZE = 10000;


// struct ThreadTask {
//     std::function<void(std::shared_ptr<void>)> process;     // 实际传入的是Server::do_request;
//     std::shared_ptr<void> arg;   // 实际应该是HttpData对象
// };

struct ThreadTask
{
// 	void (*process)(void *);			// 实际传入的是 HttpServer::do_process;
    
    std::function<void(void *)> process;    // 实际传入的是 HttpServer::do_process;
	void *arg;						// 实际应该是 HttpData* 
};



class ThreadPool {

public:

	// 参数thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量
	ThreadPool(int thread_number, int max_requests);
	~ThreadPool();

	// 往请求队列中添加任务
	// bool append(T *request);
	bool append(std::function<void(void *)> func, void* arg);



private:
	// 工作线程运行的函数，它不断从工作队列中取出任务并执行之
	static void* worker(void *arg);
	void run();


private:

	int thread_number_;
	int max_requests_;
	std::vector<pthread_t> threads_;	// 描述线程池的vector，其大小为 thread_number

	// std::list<T*> workqueue_;			// 请求队列
	std::list<ThreadTask> workqueue_;		// 请求队列


	MutexLocker mutex_;					// 保护请求队列的互斥锁
	Sem queuestat_;						// 信号量，标记当前请求队列的任务量，表示是否有任务需要处理，当没有
										// 任务需要处理且调用m_queuestat.wait()时，阻塞当前线程。
	bool stop_;						// 是否结束线程

};


#endif