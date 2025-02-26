/*
 * Server.h
 *
 *  Created on: Sep 16, 2019
 *      Author: Swapnil Bhosale
 */

#include <iostream>
#include <unordered_map>
#include <list>
#include <pthread.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <string>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>

#include "../include/spdlog/spdlog.h"
#include "../include/spdlog/sinks/stdout_color_sinks.h"
#include "../include/spdlog/sinks/rotating_file_sink.h"
#include "ServerSock.h"
#include "../utils/constants.h"
#include "../utils/ObserverPattern.h"
#include "../utils/util.h"
#include "../utils/Customer.h"
#include "../utils/Transaction.h"

static int count = 0;

class BankServer: public Observer{
public:
	BankServer();
	virtual ~BankServer();
	void init(std::string serverFile, int port, int threadCount);

	void static print_stats(int signal_number);
	void create_thread(int index, ServerSock *serverSock);
	void do_action(char * data, int clientSocket);

	void notify(char * data, int clientSocket){
		do_action(data, clientSocket);
	}
	static void *intrest_service_invoke_helper(void *context){
		return ((BankServer *)context)->handle_intrest_service();
	}

	void  * handle_intrest_service();

	pthread_t get_intrest_Service_thread(){
		return intrest_service__thread;
	}
	inline static int count;

private:
	ServerSock *serverSock;
	pthread_t threads[THREADS_COUNT];
	pthread_t intrest_service__thread;
	inline static std::unordered_map<int, Customer> customer_map;
	inline static std::unordered_map< int, std::vector< Transaction > > transaction_map;
	std::string withdrawal(std::string tstamp, std::string account_id, std::string amount);
	std::string deposit(std::string tstamp, std::string account_id, std::string amount);
	void initialize_static_data(std::string ipFile);
	void create_intrest_service();
	void update_customer_map(Customer c);
	pthread_mutex_t mutex_map;
	std::shared_ptr<spdlog::logger> _logger;
	Customer get_customer_by_id(int id);
	std::string update_customer_by_id(int id, double amount, int op);
	pthread_mutex_t mutex1,mwrite,mread,rallow;
	int readcount=0,writecount=0;
};

