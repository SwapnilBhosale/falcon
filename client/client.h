/*
 * client.h
 *
 *  Created on: Sep 21, 2019
 *      Author: lilbase
 */

#ifndef CLIENT_H_
#define CLIENT_H_
#include <iostream>
#include <fstream>
#include <pthread.h>
#include "../server/util.h"
#include "../server/Transaction.h"
#include "../server/constants.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <string>

class Client {
private:
	pthread_t client_service_thread;
	int client_socket;
public:
	static void *client_service_invoke_helper(void *context){
		return ((Client *)context)->handle_client_service();
	}
	void create_client_Service();
	void  * handle_client_service();

	pthread_t getClientServiceThread() const
	{
		return client_service_thread;
	}

	void setClientServiceThread(pthread_t clientServiceThread)
	{
		client_service_thread = clientServiceThread;
	}

	std::string do_transaction(Transaction t);
};

#endif /* CLIENT_H_ */
