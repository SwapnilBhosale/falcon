/*
 * Server.cpp
 *
 *  Created on: Sep 16, 2019
 *      Author: lilbase
 */

#include "BankServer.h"

BankServer::BankServer() {
	sem_init(&x,0,1);
	sem_init(&wsem,0,1);
	sem_init(&y,0,1);
	sem_init(&z,0,1);
	sem_init(&rsem,0,1);
	//std::cout.flush();
	intrest_service__thread = 0;
	serverSock = NULL;
	mutex_map = PTHREAD_MUTEX_INITIALIZER;
	_logger = spdlog::get("Server");

}

void BankServer::update_customer_map(Customer c){
	pthread_mutex_lock(&mutex_map);
	BankServer::customer_map[c.getAccountNumber()] = c;
	pthread_mutex_unlock(&mutex_map);
}

void *BankServer::handle_intrest_service(){
	std::unordered_map<int, Customer>:: iterator itr;
	while(true){
		sleep(INTREST_SERVICE_SCHEDULE);
		for (itr = BankServer::customer_map.begin(); itr != BankServer::customer_map.end(); itr++) {
			int act_no = itr -> first;
			Customer c = itr -> second;
			double updated_bal = c.calculate_intrest();
			c.add_money(updated_bal);
			update_customer_map(c);
		}
		_logger -> info("Intrest service ran!");
	}
}

void BankServer::create_intrest_service(){
	pthread_create(&intrest_service__thread, NULL, &BankServer::intrest_service_invoke_helper, this);
}

std::string BankServer::withdrawal(std::string tstamp, std::string acc_no, std::string amt){
	std::string msg;
	int int_acc_no = stoi(acc_no);
	if (BankServer::customer_map.find(int_acc_no) == BankServer::customer_map.end()){
		msg = "Customer id: "+acc_no+" not present!";
	}

	Customer c  = get_customer_by_id(stoi(acc_no));
	//std::vector<Transaction> &v = transaction_map[int_acc_no];
	double amount = stod(amt);
	/*TransactionBuilder b;
	Transaction trans = b.set_account_number(int_acc_no)
																.set_timestamp(tstamp)
																.set_transaction_type('W')
																.set_amount(amount)
																.build();*/
	//v.push_back(trans);
	msg = update_customer_by_id(stoi(acc_no), amount, 0);

	_logger -> info("{}",msg);
	return msg;
}


Customer BankServer::get_customer_by_id(int id){
	Customer c;

	sem_wait(&z);
	sem_wait(&rsem);
	sem_wait(&x);
	readcount++;
	if(readcount==1)
		sem_wait(&wsem);
	sem_post(&x);
	sem_post(&rsem);
	sem_post(&z);
	try{
		c =	BankServer::customer_map.at(id);
	} catch (const std::out_of_range& oor) {
	}
	sem_wait(&x);
	readcount--;
	if(readcount==0)
		sem_post(&wsem);
	sem_post(&x);
	return c;
}

std::string BankServer::update_customer_by_id(int id, double amount, int op) {

	Customer c = get_customer_by_id(id);
	std::string msg;
	sem_wait(&y);
	writecount++;
	if(writecount==1)
		sem_wait(&rsem);
	sem_post(&y);
	sem_wait(&wsem);

	if(op == 1){
		c.add_money(amount);
		msg = c.get_deposit_success_msg(amount);
	}
	else {
		if(c.can_withdraw(amount)){
			c.reduce_money(amount);
			msg = c.get_withdrawl_success_msg(amount);

		}else{
			msg = c.get_withdraw_fail_msg(amount);
		}

	}
	BankServer::customer_map[id] = c;
	sem_post(&wsem);
	sem_wait(&y);
	writecount--;
	if(writecount==0)
		sem_post(&rsem);
	sem_post(&y);
	return msg;

}
std::string BankServer::deposit(std::string tstamp, std::string acc_no, std::string amt){
	std::string msg;
	int int_acc_no = stoi(acc_no);
	if (BankServer::customer_map.find(int_acc_no) == BankServer::customer_map.end()){
		std::cout<<"Cutomer id : "<<int_acc_no<<" not present!"<<std::endl;
		msg = "Customer id: "+acc_no+" not present!";
	} else {

		double amount = stod(amt);
		//std::vector<Transaction> &v = BankServer::transaction_map[int_acc_no];
		/*TransactionBuilder b;

		Transaction trans = b.set_account_number(int_acc_no)
															.set_timestamp(tstamp)
															.set_transaction_type('W')
															.set_amount(amount)
															.build();*/

		msg = update_customer_by_id(int_acc_no, amount, 1);
	}
	_logger -> info(msg);
	return msg;
}

void BankServer::do_action(char * data, int clientSocket){
	count += 1;
	char * buf;
	std::string msg;
	std::string arr[4];
	splitString(arr, std::string(data));

	char choice = arr[2][0];
	switch(toupper(choice)) {
	case 'W':
		msg = withdrawal(arr[0], arr[1], arr[3]);
		break;
	case 'D':
		msg = deposit(arr[0], arr[1], arr[3]);
		break;
	default:
		_logger -> warn("Invalid Transaction type received: {}",arr[2]);
		break;
	}

	std::string payload("HTTP/1.1 200 OK\r\n");
	payload.append("Server: Swapnil\r\n");
	payload.append("Content Type: text/html\r\n");
	payload.append("Connection: close\r\n");
	payload.append("Content-Length: ");
	payload.append(std::to_string(msg.length()).append("\r\n"));
	payload.append(msg);
	buf = strcpy(new char[payload.length() + 1], payload.c_str());
	_logger -> debug("Sending message to client: {}", msg);
	send(clientSocket, buf, payload.length(), 0);
}

BankServer::~BankServer() {
	delete serverSock;
}

void BankServer::init(){
	initialize_static_data();
	serverSock = new ServerSock();
	serverSock -> init();
	create_intrest_service();
	for (int i = 0; i < THREADS_COUNT; ++i) {
		create_thread(i, serverSock);
	}
}


void BankServer::initialize_static_data(){
	std::ifstream file;
	file.open("./src/Records.txt");
	std::string line;
	if(file.is_open()) {
		while(getline(file, line)){
			std::string arr[3];
			splitString(arr, line);

			CustomerBuilder b;
			Customer c = b.set_account_number(std::stoi(arr[0]))
																	.set_name(arr[1])
																	.set_balance(std::stol(arr[2]))
																	.build();
			update_customer_map(c);
		}
		_logger->info("loaded static data of size {}",customer_map.size());
	}
	file.close();
}



void BankServer::print_stats(int signal_Number) {
	for(auto it = customer_map.cbegin(); it != customer_map.cend(); ++it)
	{
		std::cout<<""<<it->second;
	}
	std::cout<<"Total request received : "<<count<<std::endl;
	double user, sys;
	struct rusage   myusage;

	if (getrusage(RUSAGE_SELF, &myusage) < 0)
		std::cout<< "Error: getrusage()";

	user = (double) myusage.ru_utime.tv_sec +
			myusage.ru_utime.tv_usec/1000000.0;
	sys = (double) myusage.ru_stime.tv_sec +
			myusage.ru_stime.tv_usec/1000000.0;

	printf("\nuser time = %g, sys time = %g\n", user, sys);
	exit(0);
}


void BankServer::create_thread(int index, ServerSock *serverSock) {
	pthread_create(&threads[index], NULL, &ServerSock::thread_pool_loop_helper, serverSock);
}


int main(int argc, char **argv) {

	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("./logs/server.txt",1024 * 1024 * 5, 10, true));
	auto combined_logger = std::make_shared<spdlog::logger>("Server", begin(sinks), end(sinks));
	combined_logger -> set_level(spdlog::level::info);
	combined_logger -> set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Thread - %t] [%l] %v");
	spdlog::register_logger(combined_logger);
	BankServer server;

	ObserverPattern *obj = ObserverPattern::get_instance();
	Observer *ob = &server;
	obj -> add_observant(ob);
	std::signal(SIGINT, server.print_stats);
	signal(SIGPIPE, server.print_stats);
	server.init();
	(void) pthread_join(server.get_intrest_Service_thread(), NULL);

	//for (;;)
	//  pause();

}
