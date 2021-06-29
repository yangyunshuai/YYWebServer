
#include "Epoll.h"
#include <cstdio>


std::unordered_map<int, HttpData*> Epoll::httpDataMap;
int Epoll::epoll_fd = -1;
const int Epoll::MAX_EVENTS = 10000;

epoll_event *Epoll::accept_events;

const int Epoll::EPOLL_WAIT_TIME = 10 * 1000;

// 可读 | ET模 | 保证一个socket连接在任一时刻只被一个线程处理
// const __uint32_t Epoll::DEFAULT_EVENTS =  (EPOLLIN | EPOLLET | EPOLLONESHOT);






int Epoll::init(int max_events) {
	Epoll::epoll_fd = ::epoll_create(max_events);
	if (epoll_fd < 0) {
		std::cout << "epoll create error" << std::endl;
		exit(-1);
	}

	Epoll::accept_events = new epoll_event[max_events];
	return Epoll::epoll_fd;
}


int Epoll::addfd(int epollfd, int fd, __uint32_t events, HttpData* httpData) {
    epoll_event event;
    event.events = events;
    event.data.fd = fd;

    // 增加httpDataMap
    if (httpData) {
	    httpDataMap[fd] = httpData;
    }

    int ret = ::epoll_ctl(Epoll::epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if (ret < 0) {
        std::cout << "epoll add error" << endl;

        // 这里要关闭连接
        delete httpData;
        httpDataMap.erase(fd);
        
        return -1;
    }
    return 0;
}


int Epoll::modfd(int epollfd, int fd, __uint32_t events, HttpData* httpData) {
    epoll_event event;
    event.events = events;
    event.data.fd = fd;

    // 每次更改的时候也更新 httpDataMap
    if (httpData) {
        httpDataMap[fd] = httpData;
    }
    int ret = ::epoll_ctl(Epoll::epoll_fd, EPOLL_CTL_MOD, fd, &event);
    if (ret < 0) {
        std::cout << "epoll mod error" << endl;

        // 释放httpData
        delete httpData;
        httpDataMap.erase(fd);
        return -1;
    }
    return 0;
}


int Epoll::delfd(int epollfd, int fd) {
    
    
    int ret = epoll_ctl(Epoll::epoll_fd, EPOLL_CTL_DEL, fd, 0);

    if (ret < 0) {
        std::cout << "epoll del error" << std::endl;
        std::cout << "Epoll::delfd  fd:   " << fd << std::endl;
        
        return -1;
    }
    
    // 移除httpDataMap 中的fd
    auto it = httpDataMap.find(fd);
    if (it != httpDataMap.end()) {
        httpDataMap.erase(it);
    }

    return 0;
}


// static int num_err = 0;

std::vector<std::pair<Epoll::ACTIVEEVENTTYPE, HttpData* > > Epoll::poll(const ServerSocket &serverSocket, int max_event, int timeout) {

	// 阻塞直到有注册事件发生 
	int event_num = epoll_wait(Epoll::epoll_fd, Epoll::accept_events, max_event, timeout);
    if (event_num < 0) {
        std::cout << "epoll_num=" << event_num << std::endl;
        std::cout << "epoll_wait error" << std::endl;
        std::cout << errno << std::endl;
        exit(-1);
    }


    std::vector<std::pair<Epoll::ACTIVEEVENTTYPE, HttpData* > > activeHttpDatas;


    	// ACTIVEEVENTCASE_NEWCLIENT = 0,		// 新客户连接事件
    	// ACTIVEEVENTCASE_IN,					// 客户数据到来事件
    	// ACTIVEEVENTCASE_OUT,				// 发往客户的数据准备发送就绪事件
    	// ACTIVEEVENTCASE_CLOSE				// 客户关闭连接事件


    for (int i = 0; i < event_num; ++i)
    {
    	int sockfd = Epoll::accept_events[i].data.fd;
    	// 新的连接到来
    	if ( (sockfd == serverSocket.listen_fd) && (Epoll::accept_events[i].events & EPOLLIN) ) {

    		pair<Epoll::ACTIVEEVENTTYPE, HttpData* > newclient_event(Epoll::ACTIVEEVENTCASE_NEWCLIENT, nullptr);
			activeHttpDatas.push_back(newclient_event);
    		// handleConnection(serverSocket);

    	} else if ( Epoll::accept_events[i].events & (EPOLLERR | EPOLLRDHUP |  EPOLLHUP) ) {
            
//             num_err ++;
            
//             std::cout << "Epoll::poll  (EPOLLERR | EPOLLRDHUP |  EPOLLHUP): "<< num_err << std::endl;
            
		    // 删除HttpDataMap中的 fd
			auto it = httpDataMap.find(sockfd);
    		if (it != httpDataMap.end()) {

    			// HttpData *temp_httpData = httpDataMap[fd];
    			// ClientSocket *temp_client = temp_httpData->clientSocket;

			    // // 注销 注册事件
			    // delfd(epoll_fd, temp_client->fd, 0);

	    		// // 关闭与客户的连接
    			// temp_client->close();
    			// // 释放资源
			    // delete temp_httpData;

	      //   	httpDataMap.erase(it);

    			HttpData *temp_httpData = httpDataMap[sockfd];
	    		pair<Epoll::ACTIVEEVENTTYPE, HttpData* > close_event(Epoll::ACTIVEEVENTCASE_CLOSE, temp_httpData);
				activeHttpDatas.push_back(close_event);

				httpDataMap.erase(it);
    		}
            
            if (it == httpDataMap.end()) {
                std::cout<< "Epoll::poll  httpData 被移除了" << std::endl;              
            }

    		continue;

    	} else if ( Epoll::accept_events[i].events & EPOLLIN ) {
    		// 客户发来数据
            
//             std::cout<< "Epoll::poll  有客户数据到来"<<std::endl;
            
    		// 取到HttpData* 
    		auto it = httpDataMap.find(sockfd);
    		if (it != httpDataMap.end()) {
                
    			HttpData *temp_httpData = httpDataMap[sockfd];
	    		pair<Epoll::ACTIVEEVENTTYPE, HttpData* > data_in_event(Epoll::ACTIVEEVENTCASE_IN, temp_httpData);
				activeHttpDatas.push_back(data_in_event);

    			// 添加完后就移除保存的连接
                httpDataMap.erase(it);
    		}

        
    	} else if ( Epoll::accept_events[i].events & EPOLLOUT ) {
    		// 数据发送准备就绪

    		// 取到HttpData* 
    		auto it = httpDataMap.find(sockfd);
    		if (it != httpDataMap.end()) {

    			HttpData *temp_httpData = httpDataMap[sockfd];
	    		pair<Epoll::ACTIVEEVENTTYPE, HttpData* > data_out_event(Epoll::ACTIVEEVENTCASE_OUT, temp_httpData);
				activeHttpDatas.push_back(data_out_event); 
    			// 添加完后就移除保存的连接
                httpDataMap.erase(it);
    		}

    	} else {

    		continue;
    	}

    }
    
    

    return activeHttpDatas;
}










