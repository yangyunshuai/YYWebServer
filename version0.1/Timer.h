#ifndef TIMER_H
#define TIMER_H

#include <deque>
#include <queue>
#include <sys/time.h>
#include <functional>

#include "Locker.h"
#include "HttpData.h"

class HttpData;

class TimerNode {
public:
    TimerNode(HttpData *httpData_, int timeout):httpData(httpData_), deleted(false) {
	    struct timeval now;
      	gettimeofday(&now, NULL);
 		expired_time = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
        
//         time_out_ = timeout;
//         init_time = ((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000);
    }
	// ~TimerNode();
    
	void setDeleted() {
		deleted = true;
        httpData = nullptr;
	}
	// 更新过期事件，当连接处理完客户发来的请求时 此函数被调用
	void updateExpiredTime(int timeout) {
	    struct timeval now;
      	gettimeofday(&now, NULL);
 		expired_time = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
	}

	size_t getExpTime() const { return expired_time; }
    
	bool isExpired() {
	    struct timeval now;
      	gettimeofday(&now, NULL);
		size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
		if (expired_time <= temp) {
            
//             std::cout<<"isExpired  current time : " << temp << std::endl;
//             std::cout<<"isExpired  expired_time : " << expired_time << std::endl;
            
//             std::cout<<"isExpired  init_time : " << init_time << std::endl;
//             std::cout<<"isExpired  time_out_ : " << time_out_ << std::endl;
            
			return true;
		} else {
// 			this->setDeleted();
			return false;
		}
	}

	bool isDeleted() const {
		return deleted;
	}
    
    HttpData *getHttpData() {
        return httpData;
    }
    
    int time_out_;
    size_t init_time;
    
private:
	HttpData *httpData;
	size_t expired_time;
	bool deleted;
};



struct TimerCmp {
	bool operator()(TimerNode *a,TimerNode *b) const {
		return a->getExpTime() > b->getExpTime();
	}
};

class TimerManager {
public:
    TimerManager();
    // ~TimerManager();
    void addTimer(HttpData *httpData, int timeout);
    void handleExpiredEvent(std::function<void(HttpData *)> closeConnection);

public:
// 		static const int DEFAULT_EXPIRED_TIME = 2 * 1000;    //ms 非长连接的过期时间 
// 		static const int DEFAULT_KEEP_ALIVE_TIME = 5 * 1000; //ms 长连接的过期时间
    static const int DEFAULT_EXPIRED_TIME = 20 * 1000;    //ms 非长连接的过期时间 
    static const int DEFAULT_KEEP_ALIVE_TIME = 60 * 1000; //ms 长连接的过期时间


private:
    // typedef std::shared_ptr<TimerNode> SPTimerNode;
    std::priority_queue<TimerNode*, std::deque<TimerNode* >, TimerCmp> timerNodeQueue;
// MutexLock lock;

    // 互斥锁，在操作 timerNodeQueue 时 必须加锁
    MutexLocker locker;
    
};

#endif