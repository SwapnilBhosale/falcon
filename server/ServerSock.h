/*
 * ServerSock.h
 *
 *  Created on: Sep 16, 2019
 *      Author: Swapnil Bhosale
 */

#ifndef SERVERSOCK_H_
#define SERVERSOCK_H_

#include <string>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <time.h>
#include <locale.h>

#include "ObserverPattern.h"
#include "util.h"


class ServerSock {
public:
	ServerSock();
	virtual ~ServerSock();
	void init();
	void * enter_server_loop();
	static void *thread_pool_loop_helper(void *context){
		return ((ServerSock *)context)->enter_server_loop();
	}
private:
	void handle_client(int client_socket);
	unsigned int port;
	int sockfd;
	pthread_mutex_t mlock;
	ObserverPattern *obj;
	void perform_action(char *arr);
};

#endif /* SERVERSOCK_H_ */
