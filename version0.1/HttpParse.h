// 工具类, 解析HTTP请求

#ifndef HTTPPARSE_H
#define HTTPPARSE_H

#include <string>
#include <unordered_map>
#include <iostream>
#include <algorithm>

#include "Util.h"
#include "HttpRequest.h"


#define CR '\r'
#define LF '\n'

#define LINE_END '\0'


// class HttpRequest;

class HttpParse {

public:
	// httpparse();
	// ~httpparse();


	static LINE_STATE parse_line(char *buffer, int &checked_index, int &read_index);

	static HTTP_CODE parse_request_line(char *line, CHECK_STATE &check_state, HttpRequest *request);

	static HTTP_CODE parse_header(char *line, CHECK_STATE &check_state, HttpRequest *request);

	static HTTP_CODE parse_body(char *line, HttpRequest *request);

	static HTTP_CODE parse_content(char *buffer, int &checked_index, int &read_index, 
	int &start_line, CHECK_STATE &check_state, HttpRequest *request);

};










#endif