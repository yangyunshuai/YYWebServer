#include "Socket.h"

void setReusePort(int fd) {
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
}


ServerSocket::ServerSocket(const int port, const char *ip):port_(port), ip_(ip), listen_fd(-1) {
	// 初始化连接
	init();
}

int ServerSocket::init() {

// 	bzero(&addr_, sizeof(addr_));	
    memset(&addr_, '\0', sizeof(addr_));
    
	addr_.sin_family = AF_INET;
	if (ip_ != nullptr) {
		::inet_pton(AF_INET, ip_, &addr_.sin_addr);
	} else {
		addr_.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	
	addr_.sin_port= htons(port_);

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd == -1) {

        std::cout << "creat socket error in file <" << __FILE__ << "> "<< "at " << __LINE__ << std::endl;
		return -1;
		// exit(0);
	}

    setReusePort(listen_fd);	// 测试用，实际项目中应注释
    setnonblocking(listen_fd); // FIXME 之前没加，导致循环接受阻塞了

	return 0;
}

int ServerSocket::bind() {

	int ret = ::bind(listen_fd, (struct sockaddr* )&addr_, sizeof(addr_));
	if (ret == -1) {
        std::cout << "bind error in file <" << __FILE__ << "> "<< "at " << __LINE__ << std::endl;
		return -1;
		// exit(0);
	}
	return 0;
}

int ServerSocket::listen() {

	int ret = ::listen(listen_fd, 1024);
	if (ret == -1) {
        std::cout << "listen error in file <" << __FILE__ << "> "<< "at " << __LINE__ << std::endl;
		return -1;
		// exit(0);
	}
	return 0;
}


int ServerSocket::accept(ClientSocket& client) const {

	int client_fd = ::accept(listen_fd, (struct sockaddr* )&client.addr_, &client.len_);
    
//     std::cout<< "ServerSocket::accept  client_fd: " << client_fd << std::endl;
    
	if (client_fd < 0) {
        if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
//             std::cout << "ServerSocket::accept client_fd:  "<< client_fd  << __FILE__ << "> "<< "at " << __LINE__ << std::endl;
            return client_fd;
        } else {
        	std::cout << "accept error in file <" << __FILE__ << "> "<< "at " << __LINE__ << std::endl;
            exit(-1);
        }
	}
    
    
//     std::cout<< "ServerSocket::accept  client_fd: " << client_fd << std::endl;
    
    client.fd = client_fd;
	return client_fd;
}


void ServerSocket::close() {
	if (listen_fd >= 0) {
		::close(listen_fd);
		listen_fd = -1; 
	}
}


ServerSocket::~ServerSocket() {
	close();
}


//////  ClientSocket ////// 
void ClientSocket::close() {
	if (fd >= 0) {
		::close(fd);
		fd = -1; 
	}
}


ClientSocket::~ClientSocket() {
	close();
}





