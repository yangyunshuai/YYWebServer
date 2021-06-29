
#include <string>

#include "HttpResponse.h"



HttpResponse::HttpResponse() {
	init();
}


void HttpResponse::init() {

	statusCode = HttpResponse::OK200;
	statusMsg = "";
	keep_alive = false;
	version = HTTP_1_0;
	mime = "text/plain";

	body = nullptr;
	file_address = nullptr;
	contentLength = -1;
	write_index = 0;
	iv_count = 0;
    sum_write_bytes = 0;
    write_send_index = 0;
	memset(write_buf, '\0', WRITE_BUFFER_SIZE);
}



bool HttpResponse::process_write(HTTP_CODE code) {

	switch (code) {
		case GET_REQUEST:
		case FILE_REQUEST: {


			add_status_line(version, OK200, "OK");
			if (contentLength <= 0) {

				const char *ok_string = "<html><body>\n That's something wrong.\n</body></html>";
                mime = "text/html";
    			body = ok_string;
				contentLength = strlen(body);
				add_headers(contentLength, keep_alive, mime);
                if (!add_content(body)) {
                    return false;
                }

			} else {

				add_headers(contentLength, keep_alive, mime);
			
				int fd = open(file_path.c_str(), O_RDONLY);
				file_address = (char*)mmap(0, contentLength, PROT_READ, MAP_PRIVATE, fd, 0);
				close(fd);

				iv[0].iov_base = write_buf;
				iv[0].iov_len = write_index;
				iv[1].iov_base = file_address;
				iv[1].iov_len = contentLength;
				iv_count = 2;
                
                sum_write_bytes = write_index + contentLength;
                
				return true;
			}

			break;
		}
		case BAD_REQUEST: {
			
			add_status_line(version, BadRequest400, "BadRequest");
			const char *error_400_content = "<html><body>\nYour request has bad syntax or is inherently impossble to satisfy.\n</html></body>\n";
			body = error_400_content;
            mime = "text/html";
			contentLength = strlen(error_400_content);
			add_headers(contentLength, keep_alive, mime);
            if (!add_content(body)) {
                return false;
            }

			break;
		}
		case NO_RESOURCE: {
			
			add_status_line(version, NotFound404, "NotFound");
			const char *error_404_content = "<html><body>\nThe reqeusted file was not found on this server.\n</html></body>\n";
			body = error_404_content;
            mime = "text/html";
			contentLength = strlen(body);
			add_headers(contentLength, keep_alive, mime);
            if (!add_content(body)) {
                return false;
            }

			break;
		}
		case FORBIDDEN_REQUEST: {
			
			add_status_line(version, Forbiden403, "Forbiden");
			const char *error_403_content = "<html><body>\nYou do not have permission to get file from this server.\n</html></body>\n";
			body = error_403_content;
            mime = "text/html";
			contentLength = strlen(body);
			add_headers(contentLength, keep_alive, mime);
            if (!add_content(body)) {
                return false;
            }

			break;
		}
		case INTERNAL_ERROR: {
			
			add_status_line(version, InternalError500, "InternalError");
			const char *error_500_content = "<html><body>\nThere was an unusual problem serving the requested file.\n</html></body>\n";
			body = error_500_content;
            mime = "text/html";
			contentLength = strlen(body);
			add_headers(contentLength, keep_alive, mime);
            if (!add_content(body)) {
                return false;
            }

			break;
		}
		default: {
			return false;
			// break;
		}
	}

	iv[0].iov_base = write_buf;
	iv[0].iov_len = write_index;
	iv_count = 1;
    
    sum_write_bytes = write_index;
    
	return true;
}




// 往 写缓冲区 中写入待发送的数据
bool HttpResponse::add_response(const char* format, ...) {
    if (write_index >= WRITE_BUFFER_SIZE) {
        return false;
    }

    va_list arg_list;
    va_start(arg_list, format);

    int len = vsnprintf(write_buf + write_index, WRITE_BUFFER_SIZE - 1 - write_index, 
                        format, arg_list);
    write_index += len;
    if (len < 0 || (write_index >= WRITE_BUFFER_SIZE) ) {
    	return false;
    }

    va_end(arg_list);

    return true;
}



bool HttpResponse::add_status_line(const VERSION version, const HttpStatusCode status, const char *msg) {
	statusCode = status;
	statusMsg = msg;

	const char *http_version = (version == HTTP_1_1 ? "HTTP/1.1": "HTTP/1.0"); 
	return add_response("%s %d %s\r\n", http_version, status, msg);
}

bool HttpResponse::add_headers(int content_len, bool keep_alive_, std::string mime) {
	bool res = add_response("Content-Length: %d\r\n", content_len);
    
//     std::cout << "HttpResponse::add_headers  mime:" << mime << std::endl;
    
	res = res && add_response("Content-type: %s\r\n", mime.c_str());
	res = res && add_response("Connection: %s\r\n", (keep_alive_ == true)? "keep-alive":"close");	
	res = res && add_response("Server: %s\r\n", "YYWebServer/1.0");
	res = res && add_response("%s", "\r\n");
	return res;
}



bool HttpResponse::add_content(const char *content)
{
	return add_response("%s", content);
}


// 对内存映射区执行 munmap 操作
void HttpResponse::unmap()
{
    if (file_address)
    {
        munmap(file_address, contentLength);
        file_address = nullptr;
    }
}


// 返回true 表示持续链接，否则表示关闭连接
HttpResponse::HttpWriteStatus HttpResponse::write_to_client(ClientSocket *clientSocket) {

	if (!clientSocket) {
		return HttpWriteStatus_CLOSE;
	}


    int temp = 0;
    
    int bytes_to_send = sum_write_bytes - write_send_index;
    
//     std::cout << "HttpResponse::HttpWriteStatus    待发送字节数: "<< bytes_to_send << "字节"  << std::endl;
    
    // 待发送字节数为0，说明已经发送完毕，则初始化以等待下一次客户数据到来
    if (bytes_to_send == 0)
    {
        // modfd(m_epollfd, m_sockfd, EPOLLIN);
        // init();
        if (keep_alive) {
            return HttpWriteStatus_KEEPALIVE;
        }
        else {
            return HttpWriteStatus_CLOSE;
        }
    }
    
//     std::cout << "HttpResponse::HttpWriteStatus    已发送字节数:  "<< write_send_index << "字节"  << std::endl;
//     std::cout << "HttpResponse::HttpWriteStatus    write_index:  "<< write_index << "字节"  << std::endl;
    
    while(true) {
        
        // 调整发送属性         
        if (write_send_index >= write_index) {

            // write_send_index: 已经发送的字节的下一个位置;  write_index: 已写入缓冲区的字节的下一个位置
            int shift = write_send_index - write_index;
            iv[0].iov_base = file_address + shift;
            iv[0].iov_len = contentLength - shift;    // 也可改为  iv[0].iov_len = sum_write_bytes - write_send_index;    
            iv_count = 1;
        }
        else {
            
            iv[0].iov_base = write_buf + write_send_index;
            iv[0].iov_len = write_index - write_send_index;
            iv_count = 1;
            
            if (file_address) {
                iv[1].iov_base = file_address;
                iv[1].iov_len = contentLength;
                iv_count = 2;
            }
        }
        
//         std::cout << "write " << std::endl;
        
        temp = writev(clientSocket->fd, iv, iv_count);
        if (temp <= -1) {
            // 如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件， 虽然在此期间，服务器无法立即接受到同一用户的
            // 下一个请求，但这可以保证连接的完整性
            if (errno == EAGAIN) {
                // modfd(m_epollfd, m_sockfd, EPOLLOUT);
                
//                 std::cout << "HttpResponse::HttpWriteStatus    TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件" << std::endl;
                
                // 应提示 等待下一轮EPOLLOUT事件
                return HttpWriteStatus_WAITWRITE;
            } 
            else {
                
//                  std::cout << "HttpResponse::write_to_client   temp <= -1 出错 关闭连接 " << std::endl;
	            // 出错， 关闭连接
	            unmap();
	            return HttpWriteStatus_CLOSE;
            }
        }
        
        write_send_index += temp;
        
//         std::cout << "HttpResponse::write_to_client    正在写内容  本次发送"<<temp <<"字节；累计发送："<< write_send_index << "字节"  << std::endl;
        
        
        if (sum_write_bytes <= write_send_index) {
            // 发送HTTP响应 成功， 根据HTTP请求中的 Connection字段决定是否立即关闭连接
            unmap();
            if (keep_alive) {
                // 初始化本实例，等待新的客户请求到来
                // init();
                // modfd(m_epollfd, m_sockfd, EPOLLIN);
                
//                 std::cout << "HttpResponse::write_to_client    最后  长连接"  << std::endl;
                
                return HttpWriteStatus_KEEPALIVE;
            }
            else
            {
//                 std::cout << "HttpResponse::write_to_client    最后  短连接"  << std::endl;
                // delfd(m_epollfd, m_sockfd, EPOLLIN);
                return HttpWriteStatus_CLOSE;
            }
        }

    }

}







