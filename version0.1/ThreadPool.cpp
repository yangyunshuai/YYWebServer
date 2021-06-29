
#include "ThreadPool.h"


ThreadPool::ThreadPool(int thread_number, int max_requests):thread_number_(thread_number), 
						max_requests_(max_requests), stop_(false) {

	if (thread_number_ <= 0 || thread_number_ > MAX_THREAD_SIZE)	{
		thread_number_ = 6;
	}

	if (max_requests_ <= 0 || max_requests_ > MAX_QUEUE_SIZE)	{
		max_requests_ = MAX_QUEUE_SIZE;
	}

	// 分配空间
	threads_.resize(thread_number_);

	int ret;
	for (int i = 0; i < thread_number_; ++i) {
		ret = pthread_create(&threads_[i], NULL, worker, this);
		if (ret != 0)
		{
            std::cout << "ThreadPool init error" << std::endl;
			throw std::exception();
		}

		ret = pthread_detach(threads_[i]);
		if (ret != 0)
		{
            std::cout << "ThreadPool init error" << std::endl;
			throw std::exception();
		}
	}

}

ThreadPool::~ThreadPool() {
	stop_ = true;
}


bool ThreadPool::append(std::function<void(void *)> func, void *arg) {

	{
		MutexLockerGuard guard(this->mutex_);
		
		if (workqueue_.size() >= max_requests_) {

	        std::cout << max_requests_;
	        std::cout << "ThreadPool too many requests" << std::endl;
			return false;
		}

		ThreadTask task;
		task.process = func;
		task.arg = arg;

		workqueue_.push_back(task);
	}

	queuestat_.post();

	return true;
}


void* ThreadPool::worker(void *arg) {

	ThreadPool *pool = (ThreadPool* )arg;
    if (pool == nullptr) {
    	return NULL;
    }

	pool->run();

	return pool;
}


void ThreadPool::run() {

	while (!stop_) {
		// 如果任务队列中没有任务，会阻塞当前线程，直到有任务
		queuestat_.wait();
		ThreadTask task;
		{
			MutexLockerGuard guard(this->mutex_);

			if (workqueue_.empty())
			{
				continue;
			}

			// FIFO
			task = workqueue_.front();
			workqueue_.pop_front();
		}
        
//         std::cout << "ThreadPool::run   有任务到来，处理任务" <<std::endl;
		task.process(task.arg);
	}
}

