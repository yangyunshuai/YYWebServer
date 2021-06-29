#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <memory>
#include <fcntl.h>
#include <sys/epoll.h>



#include "Socket.h"
#include "HttpParse.h"
#include "HttpResponse.h"
#include "HttpData.h"
#include "Timer.h"


#define BUF_SIZE 2048

class HttpServer {
public:
	HttpServer(int port = 80, const char *ip = "0.0.0.0");
// 	~HttpServer();

	void run(int thread_size, int max_queue_size = 10000);


public:

	int epollfd;


private:

	// 处理新连接，
	void handleNewConnection();
	// 关闭连接
	void closeConnection(HttpData*);


	// 线程调用的函数, 根据stage 执行不同函数
	// ProcessStageRequest： 从socket读数据到读缓冲区，解析HTTP请求，处理请求，往写缓冲区写数据
	// ProcessStageResponse： 从写缓冲区发送数据到 socket
	void do_process(void *arg); 

	// bool process_read();
	// bool process_write();


private:
	ServerSocket serverSocket;
    // 定时器，管理长连接
    TimerManager timerManager;


};















#endif