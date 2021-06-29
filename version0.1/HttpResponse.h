
#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H


#include <stdarg.h>
#include <sys/mman.h>
#include<sys/uio.h>

#include "HttpRequest.h"
#include "Socket.h"



class HttpResponse {

public:
    enum HttpStatusCode {
        OK200 = 200,
        BadRequest400 = 400,
        Forbiden403 = 403,
        NotFound404 = 404,
        InternalError500 = 500,
        Unknow = 0
    };

    // write_to_client 函数返回的 标记，使HttpServer::根据标记采取不同动作
    enum HttpWriteStatus {
        HttpWriteStatus_CLOSE = 0,  // 关闭连接
        HttpWriteStatus_KEEPALIVE,	// 保持连接
        HttpWriteStatus_WAITWRITE	// 当前TCP写缓冲已经没有空间，等待下一次可写事件，
    };



	HttpResponse();
	// ~HttpResponse();

	void init();
    
    // return   true： 写入写缓冲区成功； false： 写入写缓存失败，关闭连接( 后续可改为给客户端发送 500服务器错误信息 )
	bool process_write(HTTP_CODE code);

	HttpWriteStatus write_to_client(ClientSocket *clientSocket);


public:
    
    VERSION version;
    bool keep_alive;

    std::string file_path;
    std::string mime;
    int contentLength;

private:

	bool add_response(const char* format, ...);
	bool add_status_line(const VERSION version, const HttpStatusCode status, const char *msg);
	bool add_headers(int content_len, bool keep_alive_, std::string mime);
	bool add_content(const char *content);
	void unmap();


private:
    HttpStatusCode statusCode;
    std::string statusMsg;
    
    // std::unordered_map<std::string, std::string> headers;
    const char *body;

	// 客户请求的目标文件被 mmap 到内存中的起始位置
    char *file_address;



	// 写缓冲区的大小
	static const int WRITE_BUFFER_SIZE = 2048;
	// 写缓冲区
    char write_buf[WRITE_BUFFER_SIZE];
	// 往写缓冲区中的字节数
	int write_index;

	// 我们将采用 wirtev 来执行写操作，所以定义下面两个成员，其中 iv_count 表示被写内存块的数量
	struct iovec iv[2];
	int iv_count;
    
    // 总的要发送的字节数： 写缓冲区中的字节数 + 请求的文件的字节数
    int sum_write_bytes;
    // 总的发送的下一个位置（总的已发送字节数）
    int write_send_index; 
    
};







#endif
