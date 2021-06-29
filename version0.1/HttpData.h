#ifndef HTTPDATA_H
#define HTTPDATA_H 

#include <memory>

#include "HttpParse.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Socket.h"
#include "Timer.h"

class TimerNode;

class HttpData
{
public:
	enum ProcessStage
	{
		ProcessStageRequest = 0,
		ProcessStageResponse
	};


	HttpData():stage(ProcessStageRequest) {
    	request = new HttpRequest();
        response = new HttpResponse();
        clientSocket = nullptr;			// 由HttpServer在 与客户建立连接时 赋给本类
	}
	// ~HttpData();
	~HttpData() {
		delete request;
		delete response;
		delete clientSocket;
	}

	// 将request的一些属性 直接（或处理后） 赋给 response，为了解耦
	void transformHttpResponseFromHttpRequest();
    
    
    void init() {
        if (!request || !response) {
            // 
            std::cout<<"HttpData::init  request == NULL || response == NULL"<<std::endl;
            return;
        }
        
        request->init();
        response->init();
        stage = ProcessStageRequest;
    }
    
    
    void linkTimer(TimerNode *node) {
        timeNode = node;
    }
    //  关闭计时器，将计时器置为 deleted，等待时间节点队列删除    
    void closeTimer();
    
    //    更新定时器的时间  （当长连接 结束一次请求时 重设）  
//     void updateExpiredTime() {
//         if (timeNode) {
//             timeNode->setDeleted();
//             timeNode = nullptr;
//         }
//     }
    

public:


	HttpRequest *request;
	HttpResponse *response;
	ClientSocket *clientSocket;
    
	ProcessStage stage;


private:
    static std::unordered_map<std::string, std::string> Mime_map;
    TimerNode *timeNode;    // 时间节点

};










#endif