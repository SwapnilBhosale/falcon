/*
 * ServerSock.cpp
 *
 *  Created on: Sep 16, 2019
 *      Author: lilbase
 */

#include "ServerSock.h"

#include <exception>
extern int count;

ServerSock::ServerSock() {
	sockfd = 0;
	port = DEFAULT_SERVER_PORT;
	mlock = PTHREAD_MUTEX_INITIALIZER;
	obj = ObserverPattern::get_instance();
	_logger = spdlog::get("Server");
}


ServerSock::~ServerSock() {
}


void ServerSock::init() {
	struct sockaddr_in srv_addr;
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd <= 0)
		perror( "error : socket()");

	int enable = 1;
	if (setsockopt(sockfd,
			SOL_SOCKET, SO_REUSEADDR,
			&enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR)");

	//bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sin_family = PF_INET;
	srv_addr.sin_port = htons(port);
	srv_addr.sin_addr.s_addr = INADDR_ANY;

	/* We bind to a port and turn this socket into a listening
	 * socket.
	 * */
	if (bind(sockfd, (const struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0){
		_logger -> error("Error in bind()");
		exit(1);
	}

	if (listen(sockfd, 10) < 0){
		_logger -> error("Error in listen()");
		exit(1);
	}
	_logger -> info("server listening on port {}", port);
}

void * ServerSock::enter_server_loop() {
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	while (1)
	{
		pthread_mutex_lock(&mlock);
		int client_socket = accept(
				sockfd,
				(struct sockaddr *)&client_addr,
				&client_addr_len);
		//std::cout<<"after accepting the client connection, thread id:"<<pthread_self()<<std::endl;
		if (client_socket < 0) {
			_logger -> error("Error: accept()");
			pthread_mutex_unlock(&mlock);
			return NULL;
		}

		pthread_mutex_unlock(&mlock);

		handle_client(client_socket);
	}
}

char* get_line(int sock, int size) {
	ssize_t n;
	int i = 0;
	char buf[size];

	std::cout<<"read in get_line: "<<buf<<std::endl;
	return buf;
}

void ServerSock::handle_client(int client_socket) {
	char buffer[1024];

	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	long int n = recv(client_socket, &buffer, 1024, 0);
		/*while(i<size && c != '\n'){


			if(n > 0){
				//buf[i] = c;
				//i++;
			}
		}*/
	buffer[n]= '\0';
	//buffer = get_line(client_socket, 1024);
	//_logger -> info("before strlen {}",sizeof(buffer));
	unsigned long len = strlen(buffer);
	_logger -> info("received data len: {}, data: {}",len, buffer);
	int index = -1;
	int prevIndex = -1;
	for (int i = 0; i <strlen(buffer); i++){
		if (buffer[i] == '\n'){
			prevIndex = index;
			index = i;
		}
	}
	//_logger -> info("index is {}",prevIndex);
	//std::string str(buffer);
	//std::string payload = str(prevIndex);
	//_logger->info("************* the payoad now {}",payload);
	obj->notify_observants(buffer, client_socket);
	//free(buffer);
	close(client_socket);
	return;
}

