
#include "HttpParse.h"
// #include "HttpRequest.h"

std::unordered_map<std::string, HTTP_HEADER> HttpRequest::header_map = {
    {"HOST",                      Host},
    {"USER-AGENT",                User_Agent},
    {"CONNECTION",                Connection},
    {"ACCEPT-ENCODING",           Accept_Encoding},
    {"ACCEPT-LANGUAGE",           Accept_Language},
    {"ACCEPT",                    Accept},
    {"CACHE-CONTROL",             Cache_Control},
    {"UPGRADE-INSECURE-REQUESTS", Upgrade_Insecure_Requests}
};



LINE_STATE HttpParse::parse_line(char* buffer, int& checked_index, int& read_index) {


	char temp;
	// checked_index指向buffer(应用程序的读缓冲区)中当前正在分析的字节，read_index指向buffer中客户数据
	// 的尾部的下一字节，buffer中滴0～checked_index字节都已分析完毕，第checked_index ~ (read_index-1)
	// 字节由下面的循环挨个分析
	for (; checked_index < read_index; ++checked_index) {
		temp = buffer[checked_index];
		// 如果当前的字节是'\r',即回车符，则说明可能读取到一个完整的行
		if (temp == CR) {
			// 如果'\r'字符碰巧是目前buffer中的最后一个已经被读入的客户数据，那么这次分析没有读取到一个完整的行
			// 返回LINE_OPEN 以表示还需要继续读取客户数据才能进一步分析
			if ((checked_index + 1) == read_index) {
				return LINE_STATE_OPEN;
			}
			// 如果下一个字符是'\n'，则说明我们成功读取到一个完整的行
			else if (buffer[checked_index + 1] == LF) {
				buffer[checked_index++] = LINE_END;
				buffer[checked_index++] = LINE_END;

				return LINE_STATE_OK;
			}
			// 否则的话，说明客户发送的 HTTP 请求存在语法问题
			return LINE_STATE_BAD;
		}
		// 如果当前的字节是'\n'，即换行符，则也说明可能读取到一个完整的行
		else if (temp == LF) {
			if ((checked_index > 1) && buffer[checked_index - 1] == CR) {
				buffer[checked_index - 1] = LINE_END;
				buffer[checked_index ++] = LINE_END;
				return LINE_STATE_OK;
			}
			// 否则的话，说明客户发送的 HTTP 请求存在语法问题
			return LINE_STATE_BAD;
		}
	}

	// 如果所有内容都分析完毕也没遇到'\r'字符， 则返回LINE_OPEN, 表示还需要继续读取客户数据才能进一步分析 
	return LINE_STATE_OPEN;
}


HTTP_CODE HttpParse::parse_request_line(char *line, CHECK_STATE &check_state, HttpRequest *request) {
	char* url = strpbrk(line, " \t");
	// 如果请求行中没有空白字符或"\t"字符，则HTP请求必有问题
	if ( !url ) {
		return BAD_REQUEST;
	}
	*url++ = '\0';

	// METHOD
	char* method = line;
	if (strcasecmp(method, "GET") == 0) {	// 仅支持GET方法
// 		printf("The request method is GET\n");
		request->method = GET;
	} else if (strcasecmp(method, "POST") == 0) {
		request->method = POST;
    } else {
        return BAD_REQUEST;
    }

	url += strspn(url, " \t");

	// VERSION
	char* version = strpbrk(url, " \t");
	if ( !version ) {
		return BAD_REQUEST;
	}
	*version++ = '\0';
	version += strspn(version, " \t");
	if (strcasecmp(version, "HTTP/1.1") == 0) {	//支持 HTTP/1.1  HTTP/1.0
		request->version = HTTP_1_1;
	} else if (strcasecmp(version, "HTTP/1.0") == 0){
		request->version = HTTP_1_0;
	} else {
		request->version = OTHERVERSION;
		return BAD_REQUEST;
	}

	// URL  
	if (strncasecmp(url, "http://", 7) == 0) {
		url += 7;
		url = strchr(url, '/');
	}
	if (!url || url[0] != '/') {
		return BAD_REQUEST;
	}
// 	printf("The request URL is: %s\n", url);
	request->url = std::string(url);

	// HTTP 请求行处理完毕， 状态转移到头部数据的分析
	check_state = CHECK_STATE_HEADER;

	return NO_REQUEST; 
}


HTTP_CODE HttpParse::parse_header(char* line, CHECK_STATE& check_state, HttpRequest *request) {

	if (line[0] == '\0') {
		if (request->method == GET) {
			return GET_REQUEST;
		}
		check_state = CHECK_STATE_BODY;
		return NO_REQUEST;
	}


	// reference to webserver write by 编程指北
    char key[100], value[300];

    // FIXME 需要修改有些value里也包含了':'符号
    sscanf(line, "%[^:]:%[^:]", key, value);


    decltype(HttpRequest::header_map)::iterator it;

    std::string key_s(key);
    std::transform(key_s.begin(), key_s.end(), key_s.begin(), ::toupper);
    std::string value_s(value);


    if ((it = HttpRequest::header_map.find(trim(key_s))) != (HttpRequest::header_map.end())) {
        request->headers.insert(std::make_pair(it->second, trim(value_s)));
    } else {
        //std::cout << "Header no support: " << key << " : " << value << std::endl;
    }

	return NO_REQUEST;
}


HTTP_CODE HttpParse::parse_body(char *line, HttpRequest *request) {
	request->content = line;
	return GET_REQUEST;
}


HTTP_CODE HttpParse::parse_content(char *buffer, int &checked_index, int &read_index, 
	int &start_line, CHECK_STATE &check_state, HttpRequest *request) {


	// int checked_index = 0;
	// int read_index = 0;
	// int start_line = 0;

	// HttpParse::CHECK_STATE check_state = CHECK_STATE_REQUESTLINE;
	LINE_STATE line_state = LINE_STATE_OPEN;
	HTTP_CODE ret = NO_REQUEST;

	while ((line_state = parse_line(buffer, checked_index, read_index)) == LINE_STATE_OK) {

		char *line = buffer + start_line;	// 新的一行 在buffer中的起始位置
		start_line = checked_index; 		// 下一行起始位置

		switch (check_state) {
			case CHECK_STATE_REQUESTLINE: {
				ret = parse_request_line(line, check_state, request);
				if (ret == BAD_REQUEST) {
					return BAD_REQUEST;
				}
				break;
			}
			case CHECK_STATE_HEADER: {
				ret = parse_header(line, check_state, request);
				if (ret == BAD_REQUEST) {
					return BAD_REQUEST;
				}
                else if (ret == GET_REQUEST) {
					return GET_REQUEST;
				}
				break;
			}
			case CHECK_STATE_BODY: {
				ret = parse_body(line, request);
				if (ret == GET_REQUEST) {
					return GET_REQUEST;
				}
				return BAD_REQUEST;
				break;
			}
			default: {
				return INTERNAL_ERROR;
			}
		}
	}

	if (line_state == LINE_STATE_OPEN) {
        
        std::cout<< "HttpParse::parse_content  line_state == LINE_STATE_OPEN" << std::endl;
        std::cout<< "HttpParse::parse_content  read_index: " << read_index << "   checked_index:  " << checked_index << std::endl;
//         std::cout<< "HttpParse::parse_content  buffer: " << buffer << std::endl;
        
		return NO_REQUEST;
	} else {	// LINE_BAD
		return BAD_REQUEST;
	}
}



