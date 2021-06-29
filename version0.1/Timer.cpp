
#include "Timer.h"

TimerManager::TimerManager() {


}

void TimerManager::addTimer(HttpData *httpData, int timeout) {
	if (!httpData) {
		return;
	}
    
	TimerNode *node = new TimerNode(httpData, timeout);
    {
//         MutexLockerGuard guard(this->locker);
        timerNodeQueue.push(node);
        httpData->linkTimer(node);        
    }
    
}

// void TimerManager::closeTimer()



/* 处理逻辑是这样的~
因为 优先队列不支持随机访问，即使支持，随机删除某节点后破坏了堆的结构，需要重新更新堆结构。
所以对于关闭连接的客户连接时，其对应的时间节点会被置为deleted，被置为deleted的时间节点，会延迟到它
(1)超时 或
(2)它前面的节点都被删除时，它才会被删除。
一个点被置为deleted,它最迟会在TIMER_TIME_OUT时间后被删除。
这样做有两个好处：
(1) 第一个好处是不需要遍历优先队列，省时。
(2) 第二个好处是给超时时间一个容忍的时间，就是设定的超时时间是删除的下限(并不是一到超时时间就立即删除)，如果监听的请求在超时后的下一次请求中又一次出现了，
就不用再重新申请RequestData节点了，这样可以继续重复利用前面的RequestData，减少了一次delete和一次new的时间。
*/


void TimerManager::handleExpiredEvent(std::function<void(HttpData *)> closeConnection) {
    //  加锁访问 timerNodeQueue
//     MutexLockerGuard guard(this->locker);
    
	while (!timerNodeQueue.empty()) {
		TimerNode *node = timerNodeQueue.top();
		if (node->isDeleted()) {
			// 队列顶部的TimerNode节点是过期时间最短的节点，
			// 先判断是否已被删除（当客户关闭连接时，将相应的节点deleted置为true）
//             std::cout<< "TimerManager::handleExpiredEvent: isDeleted  此节点已被置为deleted" <<std::endl;
			timerNodeQueue.pop();
			delete node;

		} else if (node->isExpired()) {
			// 队列顶部的TimerNode节点是过期时间最短的节点，若其已过期，则删除它，并处理下一个TimerNode节点
			// 关闭与客户端的连接，此处实际调用的 HttpServer::closeConnection() 函数
//             std::cout<< "TimerManager::handleExpiredEvent: isExpired  此节点已过期 准备关闭连接" <<std::endl;
            
            HttpData *data = node->getHttpData();
            std::cout<<"TimerManager::handleExpiredEvent: isExpired  client fd: " << data->clientSocket->fd << std::endl;
            
            
			closeConnection(node->getHttpData());
//             std::cout<< "TimerManager::handleExpiredEvent: isExpired 关闭连接" <<std::endl;
			timerNodeQueue.pop();
			delete node;
			
		} else {
			// 队列顶部的TimerNode节点是过期时间最短的节点，当其未过期时，说明当前所有的节点均为过期
			break;
		} 
	}

}