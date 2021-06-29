

#include "HttpServer.h"
#include "Epoll.h"
#include "ThreadPool.h"


HttpServer::HttpServer(int port, const char *ip):serverSocket(port, ip), timerManager(){
	if (serverSocket.bind() != 0) {
		exit(0);
	}
	if (serverSocket.listen() != 0) {
		exit(0);
	}

};

    	// ACTIVEEVENTCASE_NEWCLIENT = 0,		// 新客户连接事件
    	// ACTIVEEVENTCASE_IN,					// 客户数据到来事件
    	// ACTIVEEVENTCASE_OUT,				// 发往客户的数据准备发送就绪事件
    	// ACTIVEEVENTCASE_CLOSE				// 客户关闭连接事件

void HttpServer::run(int thread_size, int max_queue_size) {
	
	ThreadPool threadPool(thread_size, max_queue_size);
    
	int epoll_fd = Epoll::init(1024);

    __uint32_t main_events =  EPOLLIN | EPOLLET;
	Epoll::addfd(epoll_fd, serverSocket.listen_fd, main_events, nullptr);
    
    std::cout<<"等待新连接..."<<std::endl;
    
	while (true) {
        
        //  epoll_wait 最多等待的时间，设置此值是为了 处理定时时间（Http长连接的保活检测，若某个连接60秒内无数据发送，则关闭此连接）
	    std::vector<std::pair<Epoll::ACTIVEEVENTTYPE, HttpData*> > activeEvents = Epoll::poll(serverSocket, 1024, Epoll::EPOLL_WAIT_TIME);

//         const int EPOLL_WAIT_TIME = 1 * 1000; // ms
//         std::vector<std::pair<Epoll::ACTIVEEVENTTYPE, HttpData*> > activeEvents = Epoll::poll(serverSocket, 1024, EPOLL_WAIT_TIME);

        
		for (auto& a_event : activeEvents) {
			switch (a_event.first) {
				case Epoll::ACTIVEEVENTCASE_NEWCLIENT: {
					// 主线程 接收客户新连接
					handleNewConnection();
                    
                    std::cout << "HttpServer::run   有新连接到来，处理新连接" <<std::endl;

					break;
				}
				case Epoll::ACTIVEEVENTCASE_IN:
				case Epoll::ACTIVEEVENTCASE_OUT: {

					// 子线程处理客户数据请求
// 					threadPool.append(&do_process, a_event.second);	
                    
                    // 连接的定时器 暂停计时 直到此次请求返回后再重新计时
                    HttpData *httpData = a_event.second;
//                     httpData->timeNode.updateExpiredTime();
                    //  有新的请求到来时，关闭计时器，等待处理完请求后再 重新设置计时器
                    if (a_event.first == Epoll::ACTIVEEVENTCASE_IN) {    
                        httpData->closeTimer();

//                         std::cout << "HttpServer::run  Epoll::ACTIVEEVENTCASE_IN     "<<std::endl;
//                         std::cout << "有新的请求到来时，关闭计时器，等待处理完请求后再 重新设置计时器" <<std::endl;
                    }
                    // 将任务加入线程池
                    std::cout << "HttpServer::run   将任务加入线程池" <<std::endl;
					threadPool.append(std::bind(&HttpServer::do_process, this, std::placeholders::_1), a_event.second);	
                    
					break;
				}
				case Epoll::ACTIVEEVENTCASE_CLOSE: {
					// 客户端关闭连接
                    
                    std::cout << "HttpServer::run  Epoll::ACTIVEEVENTCASE_CLOSE   客户端 关闭 连接 " <<std::endl;
                        
//                     std::cout << "HttpServer::run   Epoll::ACTIVEEVENTCASE_CLOSE   客户端 关闭 连接" <<std::endl;
					closeConnection(a_event.second);
                    
					break;
				}
				default: {
					break;
				}
			}
		}

		// 处理定时事件:  其中HttpServer::closeConnection传到TimerManager里，当长连接的TimerNode过期时，被调用以关闭连接
        timerManager.handleExpiredEvent(std::bind(&HttpServer::closeConnection, this, std::placeholders::_1));
        
	}
}


// 处理新连接，
void HttpServer::handleNewConnection() {
    
	ClientSocket *client = new ClientSocket();
	while (serverSocket.accept(*client) > 0) {

        // 设置非阻塞
        int ret = setnonblocking(client->fd);
        if (ret < 0) {
            std::cout << "setnonblocking error" << std::endl;
            client->close();
            continue;
        }

        // 构造HttpData 
        HttpData *httpData = new HttpData();
        // httpData->request = new HttpRequest();	// 由HttpData构造函数构造
        // httpData->response = new HttpResponse();
        httpData->clientSocket = new ClientSocket(*client);
        
        __uint32_t init_events =  EPOLLIN | EPOLLET | EPOLLRDHUP |EPOLLONESHOT;
        Epoll::addfd(Epoll::epoll_fd, httpData->clientSocket->fd, init_events, httpData);
        
        // 测试 添加定时器 默认超时时间 20 秒
        timerManager.addTimer(httpData, TimerManager::DEFAULT_EXPIRED_TIME);   
	}

	if (client->fd < 0) {
		delete client;
	}
}


// 关闭连接
void HttpServer::closeConnection(HttpData* httpData) {

	if (!httpData) {
		std::cout<<"HttpServer::closeConnection:  httpdata == NULL"<<std::endl;
		return;
	}

	ClientSocket *client = httpData->clientSocket;
	if (!client) {
		std::cout<<"HttpServer::closeConnection:  httpdata->clientSocket == NULL"<<std::endl;
		return;
	}
    
    std::cout<< "HttpServer::closeConnection   closeConnection "<< std::endl;
    
    if (client->fd > 10000 || client->fd <= 0) {
//         std::cout<< "httpData->request" << *(httpData->request) << std::endl;
        std::cout<<  "client fd" <<client->fd  << std::endl;
        
        std::cout<<  "client address: " <<client << std::endl;
        std::cout<<  "client  fd  address: " << &(client->fd) << std::endl;
    }
    
    
    
	Epoll::delfd(Epoll::epoll_fd, client->fd);
	client->close();
    // 通过主动调用closeTimer 使得 httpData的TimerNode置为deleted，使得删除该节点
//     std::cout << "HttpServer::closeConnection closeTimer()"<<std::endl;
    httpData->closeTimer();
    
	delete httpData;
    
}



void HttpServer::do_process(void *arg) {
	
	HttpData *httpData = (HttpData* )arg;
    
	if (!httpData) {
		std::cout<<"HttpServer::do_process:  httpdata == NULL"<<std::endl;
		return;
	}

	if ((!httpData->request) || (!httpData->response) || (!httpData->clientSocket) ) {
		std::cout<< "HttpServer: request ,response or clientSocket is NULL"<<std::endl;
		return;
	}

    std::cout << "HttpServer::do_process   即将处理任务" <<std::endl;

	if (httpData->stage == HttpData::ProcessStageRequest) {
		if (httpData->request->read_from_client(httpData->clientSocket)) {
			// 处理读到的数据

//             std::cout << "HttpServer::do_process   读完数据，准备分析数据" <<std::endl;
            
			// static HTTP_CODE parse_content(char *buffer, int &checked_index, int &read_index, 
			// int &start_line, CHECK_STATE& check_state);
            HTTP_CODE code = httpData->request->process_read();
            
            std::cout << "HttpServer::do_process   已分析完数据" <<std::endl;
//             std::cout << *(httpData->request) <<std::endl;
//             std::cout << "HttpServer::do_process HTTP_CODE: "<<code <<std::endl;
            
			if (code != NO_REQUEST) {
				// 当前Http数据包接收完毕
				httpData->transformHttpResponseFromHttpRequest();
                // 将返回的信息写入 写缓冲区
                
                if (httpData->response->process_write(code)) {
                    // 写入写缓存成功，切换到发送阶段
                    httpData->stage = HttpData::ProcessStageResponse;
                    // 注册 可写事件
                    __uint32_t init_events =  EPOLLOUT | EPOLLET | EPOLLRDHUP |EPOLLONESHOT;
                    Epoll::modfd(Epoll::epoll_fd, httpData->clientSocket->fd, init_events, httpData);
                    
//                     std::cout << "HttpServer::do_process   写入写缓存成功，切换到发送阶段，注册 可写事件" <<std::endl;
                }
                else {
                    // 写入写缓存失败，立即关闭连接( 后续可改为给客户端发送 500服务器错误信息 )
                    closeConnection(httpData);
//                     std::cout << "HttpServer::do_process   写入写缓存失败，关闭连接" <<std::endl;
                }
                
			} else {
				// 当前Http数据报未接收完毕，还需继续接收, 重新注册 读事件
				__uint32_t init_events =  EPOLLIN | EPOLLET | EPOLLRDHUP |EPOLLONESHOT;
        		Epoll::modfd(Epoll::epoll_fd, httpData->clientSocket->fd, init_events, httpData);
                
                timerManager.addTimer(httpData, TimerManager::DEFAULT_EXPIRED_TIME);
                
                
                std::cout<< "HttpServer::do_process   当前Http数据报未接收完毕，还需继续接收, 重新注册 读事件  httpData->clientSocket->fd: " << httpData->clientSocket->fd <<std::endl;
                
                std::cout<<  "HttpServer::do_process  client address: " << httpData->clientSocket << std::endl;
                std::cout<<  "HttpServer::do_process  client  fd  address: " << &(httpData->clientSocket->fd) << std::endl;    
                
                ////////////////
                // 问题出在从这以后 到 TimerNode过期 之前
                
//                 std::cout<< "HttpServer::do_process   当前Http数据报未接收完毕，还需继续接收, 重新注册 读事件" <<std::endl;
                
                
				return;
			}

		} else {
			// 立即关闭连接
            
            std::cout<< " HttpServer::do_process  另外一种关闭方式 " << std::endl;
			closeConnection(httpData);
		}

	} else {
        
//         std::cout << "HttpServer::do_process   写阶段" <<std::endl;
        
		HttpResponse::HttpWriteStatus writeStatus = httpData->response->write_to_client(httpData->clientSocket);
		switch (writeStatus) {
			case HttpResponse::HttpWriteStatus_KEEPALIVE: {	// 保持连接
                
                std::cout << "HttpServer::do_process   HttpResponse::HttpWriteStatus_KEEPALIVE 保持长连接 设置长连接的定时器 " <<std::endl;
                
				// 初始化HttpData，为下一次连接做准备
				httpData->init();
            
				__uint32_t init_events =  EPOLLIN | EPOLLET | EPOLLRDHUP |EPOLLONESHOT;				
        		Epoll::modfd(Epoll::epoll_fd, httpData->clientSocket->fd, init_events, httpData);
                
                // 设置长连接的定时器                 
                timerManager.addTimer(httpData, TimerManager::DEFAULT_KEEP_ALIVE_TIME);
                
				break;
			}
			case HttpResponse::HttpWriteStatus_CLOSE: {	// 关闭连接
                
				closeConnection(httpData);
//                 压力测试时，应注释下面 几行， 打开上面一行
                
                
//                 std::cout << "HttpServer::do_process   HttpResponse::HttpWriteStatus_CLOSE 延迟到 默认过期时间 后无连接时关闭 httpData->clientSocket->fd: " << httpData->clientSocket->fd <<std::endl;
                    // 延迟关闭，延迟到 默认过期时间 后无连接时关闭
//                 httpData->init();
//                 __uint32_t init_events =  EPOLLIN | EPOLLET | EPOLLRDHUP |EPOLLONESHOT;				
//                 Epoll::modfd(Epoll::epoll_fd, httpData->clientSocket->fd, init_events, httpData);
//                 timerManager.addTimer(httpData, TimerManager::DEFAULT_EXPIRED_TIME); 
                
				break;
			}

			case HttpResponse::HttpWriteStatus_WAITWRITE: {	// 当前TCP写缓冲已经没有空间，等待下一次可写事件


				__uint32_t init_events =  EPOLLOUT | EPOLLET | EPOLLRDHUP |EPOLLONESHOT;				
        		Epoll::modfd(Epoll::epoll_fd, httpData->clientSocket->fd, init_events, httpData);

				break;
			}
			default: {
				break;
			}

		}

	}

}













