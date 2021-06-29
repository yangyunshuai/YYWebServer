#include "HttpData.h"


// HttpData::~HttpData() {
// 	delete request;
// 	delete response;
// 	delete clientSocket;
// }

std::unordered_map<std::string, std::string> HttpData::Mime_map = {
        {".html", "text/html"},
        {".xml", "text/xml"},
        {".xhtml", "application/xhtml+xml"},
        {".txt", "text/plain"},
        {".rtf", "application/rtf"},
        {".pdf", "application/pdf"},
        {".word", "application/msword"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".au", "audio/basic"},
        {".mpeg", "video/mpeg"},
        {".mpg", "video/mpeg"},
        {".avi", "video/x-msvideo"},
        {".gz", "application/x-gzip"},
        {".tar", "application/x-tar"},
        {".css", "text/css"},
        {"", "text/plain"},
        {"default","text/plain"}
};


void HttpData::transformHttpResponseFromHttpRequest() {

	if (!request || !response) {
		return;
	}

	response->version = request->version;
	response->file_path = request->file_path;
	response->contentLength = request->file_stat.st_size;
    
    auto it = HttpData::Mime_map.find(request->file_type);
    if (it != HttpData::Mime_map.end()) {
        response->mime = it->second;
    }

    auto it_Conn = request->headers.find(Connection);
    if (it_Conn != request->headers.end()) {
        if (it_Conn->second == "keep-alive" || it_Conn->second == "Keep-Alive") {
            response->keep_alive = true;
            // timeout=20s
            // sharedHttpData->response_->addHeader("Keep-Alive", std::string("timeout=20"));
        } else {
            response->keep_alive = false;
        }
    }
    
//     std::cout<< "HttpData::transformHttpResponseFromHttpRequest" << (*request)  << std::endl;
    
}

void HttpData::closeTimer() {
    if (timeNode) {
        
//         std::cout<< "HttpData::closeTimer() " << std::endl;
        timeNode->setDeleted();
    }
    
//     if (timeNode == nullptr) {
//         std::cout<< "HttpData::closeTimer() timeNode == NULL " << std::endl;
//     }
    
    timeNode = nullptr;
}






