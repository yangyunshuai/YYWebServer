
#include <iostream>
#include <string>
#include <signal.h>

#include "Socket.h"
// #include "HttpParse.h"
// #include "HttpRequest.h"
#include "HttpServer.h"


void handle_for_sigpipe()
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))
        return;
}




int main(int argc, char const *argv[]) {

	int port = 3001;		// 默认端口	
	const char *ip = "0.0.0.0";	// 默认ip

    int threadNumber = 4;   //  默认线程数

	handle_for_sigpipe();
	HttpServer server(port, ip);
	server.run(threadNumber);

	return 0;
}