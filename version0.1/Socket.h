#ifndef Socket_H
#define Socket_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <fcntl.h>

#include "Util.h"

// #include <cstdlib>

 

class ClientSocket {
public:
 	ClientSocket():fd(-1) { len_ = sizeof(addr_); };
 	~ClientSocket();
    
//     ClientSocket &operator=(ClientSocket &client) {
//         this->fd = client.fd;
//         this->addr_ = client.addr_;
//         this->len_ = client.len_;
//     };
    
 	void close();


public:
	int fd;
	struct sockaddr_in addr_;
	socklen_t len_;
 }; 



class ServerSocket {

public:
	ServerSocket(const int port, const char *ip);
	~ServerSocket();

    void close();

	int init();
	int bind();
	int listen();
	int accept(ClientSocket& client) const;

	int listen_fd;



private:
	struct sockaddr_in addr_;
	// socklen_t len_;
	const int port_;
	const char* ip_;

};






 #endif