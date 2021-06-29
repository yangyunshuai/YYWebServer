#ifndef EPOLL_H
#define EPOLL_H 

#include <sys/epoll.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>

#include "HttpData.h"
#include "Socket.h"




class Epoll
{
public:
	// Epoll();
	// ~Epoll();
    enum ACTIVEEVENTTYPE
    {
    	ACTIVEEVENTCASE_NEWCLIENT = 0,		// 新客户连接事件
    	ACTIVEEVENTCASE_IN,					// 客户数据到来事件
    	ACTIVEEVENTCASE_OUT,				// 发往客户的数据准备发送就绪事件
    	ACTIVEEVENTCASE_CLOSE				// 客户关闭连接事件
    };

	

	static int init(int max_events);

	static int addfd(int epollfd, int fd, __uint32_t events, HttpData* httpData);

	static int modfd(int epollfd, int fd, __uint32_t events, HttpData* httpData);

	static int delfd(int epollfd, int fd);

	static std::vector<std::pair<Epoll::ACTIVEEVENTTYPE, HttpData* > >
	poll(const ServerSocket &serverSocket, int max_events, int timeout);

	// static void handleConnection(const ServerSocket &serverSocket);

public:

	static int epoll_fd;
    static std::unordered_map<int, HttpData*> httpDataMap;
    static const int MAX_EVENTS;
    
    // 有事件到来时，使用 accept_events 接收之     
    static epoll_event *accept_events;
    
    //  epoll_wait 最多等待的时间，设置此值是为了 处理定时时间（Http长连接的保活检测，若某个连接60秒内无数据发送，则关闭此连接）
    static const int EPOLL_WAIT_TIME; // ms
     
    
    // static TimerManager timerManager;

//     const static __uint32_t DEFAULT_EVENTS;

	
};







#endif