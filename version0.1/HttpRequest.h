#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H


#include <iostream>
#include <string>
#include <unordered_map>
#include <sys/stat.h>
#include <errno.h>

#include "Util.h"
#include "Socket.h"
// #include "HttpParse.h"

class HttpRequest;

std::ostream &operator<<(std::ostream &, const HttpRequest &);


 enum LINE_STATE {
    LINE_STATE_OK = 0, LINE_STATE_BAD, LINE_STATE_OPEN
};

enum CHECK_STATE {
    CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_BODY
};

enum HTTP_CODE {
    NO_REQUEST = 0, BAD_REQUEST, GET_REQUEST, FILE_REQUEST, FORBIDDEN_REQUEST, 
    NO_RESOURCE, INTERNAL_ERROR, CLOSED_CONNECTION
};

enum METHOD {
    GET = 0, 
    POST,
    HEAD,
    PUT,
    DELETE,
    TRACE,
    OPTIONS,
    CONNECT,
    PATCH,
    OTHERMETHOD
};

enum VERSION {
    HTTP_1_0 = 0,
    HTTP_1_1,
    OTHERVERSION
};

enum HTTP_HEADER {
    Host = 0,
    User_Agent,
    Connection,
    Accept_Encoding,
    Accept_Language,
    Accept,
    Cache_Control,
    Upgrade_Insecure_Requests
};  



class HttpRequest {
public:

	friend std::ostream &operator<<(std::ostream &, const HttpRequest &);

	// HTTP请求方法，但我们仅支持GET

	// 写缓冲区的大小
	// static const int WRITE_BUFFER_SIZE = 1024;
	// 文件名的最大长度
	// static const int FILENAME_LEN = 200;


	// char write_buf[WRITE_BUFFER_SIZE];		// 写缓冲区


	VERSION version;			// HTTP 版本
	std::string url;				// URL
	METHOD method;			// 请求方法 GET/POST

	// bool linger;			// 是否持续连接
	// int content_length;		// HTTP请求的消息体的长度
	char *content;


	std::string file_path;		// Get方法请求的文件路径
	std::string file_type;		// Get方法请求的文件后缀名; 例如 .txt / .pdf / .word / .html / .png 


	struct stat file_stat;		// 目标文件的状态，通过它我们可以判断文件是否存在，是否为目录，是否可读，并获取文件大小等信息



    struct EnumClassHash {
        template<typename T>
        std::size_t operator()(T t) const {
            return static_cast<std::size_t>(t);
        }
    };

    std::unordered_map<HTTP_HEADER, std::string, EnumClassHash> headers;

    // 仅处理下面这些Header 
    static std::unordered_map<std::string, HTTP_HEADER> header_map;


public:
	HttpRequest();
	// ~HttpRequest();

	// 当新的Http数据报到来之前， 初始化数据, 当且仅当 Http为长连接时，此方法才被显式调用
	void init();

	// 从客户socket读数据到读缓冲区
	bool read_from_client(ClientSocket *clientSocket);

	// 处理 读缓冲区的数据。 		
	// 返回NO_REQUEST时： 当前Http数据报分析结果为未接收完毕，还需继续接收, 让HttpServer 重新注册 读事件；
	// 返回其他时；当前Http数据报接收完毕。
	HTTP_CODE process_read();

private:

	int read_index;						// 标识 读缓冲中已经读入的客户数据的最后一个字节的下一个位置
	int checked_index;					// 当前正在分析的字符在读缓冲区中的位置
	int start_line;						// 当前正在解析的行在读缓冲区中的起始位置
	CHECK_STATE check_state;			// 主状态机当前所处的状态

	static const int READ_BUFFER_SIZE = 2048;		// 读缓冲区的大小
	char read_buf[READ_BUFFER_SIZE];	// 读缓冲区

private:

	HTTP_CODE process_file_status();

	
};


#endif
