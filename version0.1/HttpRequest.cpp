

#include "HttpRequest.h"
#include "HttpParse.h"


// 重载 HttpRequest <<
std::ostream &operator<<(std::ostream &os, const HttpRequest &request) {
    os << "method:" << request.method << std::endl;
    os << "url:" << request.url << std::endl;
    os << "version:" << request.version << std::endl;
    
    os << "read_index:" << request.read_index << std::endl;
    os << "checked_index:" << request.checked_index << std::endl;
    os << "start_line:" << request.start_line << std::endl;

        
    //os << "content:" << request.mContent << std::endl;
    for (auto it = request.headers.begin(); it != request.headers.end(); ++it) {
        os << it->first << ":" << it->second << std::endl;
    }
    return os;
}



HttpRequest::HttpRequest() {
	init();
};



void HttpRequest::init() {

	version = HTTP_1_0;
	url = "";
	method = GET;
	// linger = false;
	file_path = "";
	file_type = "";
	// content_length = -1;
	content = nullptr;

	read_index = 0;
	checked_index = 0;
	start_line = 0;
	check_state = CHECK_STATE_REQUESTLINE;

	headers = std::unordered_map<HTTP_HEADER, std::string, EnumClassHash>();

	memset(read_buf, '\0', READ_BUFFER_SIZE);
}


bool HttpRequest::read_from_client(ClientSocket *clientSocket) {

	if (!clientSocket) {	
		std::cout<<"HttpRequest::read_from_client:  clientSocket == NULL"<< std::endl;
		return false;
	}


	if (read_index >= HttpRequest::READ_BUFFER_SIZE) {
		std::cout<< "HttpRequest::read_from_client: read_buf is full"<< std::endl;
		return false;
	}

    int recv_data;
    while (true) {
	    // 这里也是同样的，由于是非阻塞IO，所以返回-1 也不一定是错误，还需判断error
	    recv_data = recv(clientSocket->fd, read_buf + read_index, HttpRequest::READ_BUFFER_SIZE - read_index, 0);
	    if (recv_data == -1) {
	        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
	            return true;  
	        }
	        // 出错，关闭与该客户的连接
	        std::cout << "HttpRequest::read_from_client:   reading faild" << std::endl;
	        return false;
	     }
	     else if (recv_data == 0) {
	     	// 客户已关闭连接，服务端也应关闭连接
            std::cout << "HttpRequest::read_from_client: recv_data == 0  客户已关闭连接，服务端也应关闭连接" << std::endl;
	     	return false;
	     }

	     read_index += recv_data;
    }

    return true;
}


HTTP_CODE HttpRequest::process_read() {

	HTTP_CODE read_ret = HttpParse::parse_content(read_buf, checked_index, read_index, start_line, check_state, this);

	if (read_ret == NO_REQUEST) {
		// 当前Http数据报未接收完毕，还需继续接收,返回NO_REQUEST, 让HttpServer 重新注册 读事件
        
        std::cout << "HttpRequest::process_read:  当前Http数据报未接收完毕，还需继续接收,返回NO_REQUEST" << std::endl;
        std::cout<< "HttpParse::process_read  buffer: " << read_buf << std::endl;
		return read_ret;
	}


	// 当前Http数据报接收完毕，进行下一步操作
	return process_file_status();
}


HTTP_CODE HttpRequest::process_file_status() {

	std::string main_path = "static_doc";
    
	// strcpy(file_path, );
	// file_path += main_path;
	// int len = strlen(doc_root);
	// strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);

	// 直接去掉URL中？后的参数，等待后续开发
	size_t pos = url.rfind("?");
    if (pos != std::string::npos) {
		url.erase(pos);
    }

	// 补充后缀名
	pos = url.rfind(".");
	if (pos != std::string::npos) {
		file_type = url.substr(pos);
	}

	// 寻找要求的文件
	file_path =  file_path + main_path + url;

	const char *real_file_path = file_path.c_str();

	if (stat(real_file_path, &file_stat) < 0)
	{
		return NO_RESOURCE;		//404
	}

	// 判断有权限访问该文件
	if ( !(file_stat.st_mode & S_IROTH) )
	{
		return FORBIDDEN_REQUEST;	//403
	}

	// 判断是否是文件夹
	if (S_ISDIR(file_stat.st_mode))
	{
		return BAD_REQUEST;		
	}


	return FILE_REQUEST;
}



