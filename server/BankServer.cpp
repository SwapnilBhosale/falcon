/*
 * Server.cpp
 *
 *  Created on: Sep 16, 2019
 *      Author: lilbase
 */

#include "BankServer.h"

BankServer::BankServer() {
	//std::cout.flush();
	intrest_service__thread = 0;
	serverSock = NULL;
	mutex_map = PTHREAD_MUTEX_INITIALIZER;

}

void BankServer::update_customer_map(Customer c){
	pthread_mutex_lock(&mutex_map);
	customer_map[c.getAccountNumber()] = c;
	pthread_mutex_unlock(&mutex_map);
}

void *BankServer::handle_intrest_service(){
	std::unordered_map<int, Customer>:: iterator itr;
	while(true){
		sleep(INTREST_SERVICE_SCHEDULE);
		for (itr = customer_map.begin(); itr != customer_map.end(); itr++) {
			int act_no = itr -> first;
			Customer c = itr -> second;
			double updated_bal = c.calculate_intrest();
			c.add_money(updated_bal);
			update_customer_map(c);
		}
		std::cout << "Intrest service ran!"<<std::endl;
	}
}

void BankServer::create_intrest_service(){
	pthread_create(&intrest_service__thread, NULL, &BankServer::intrest_service_invoke_helper, this);
}

std::string BankServer::withdrawal(std::string tstamp, std::string acc_no, std::string amt){
	std::string msg;
	int int_acc_no = stoi(acc_no);
	if (customer_map.find(int_acc_no) == customer_map.end()){
		std::cout<<"Cutomer id : "<<int_acc_no<<" not present!"<<std::endl;
		msg = "Customer id: "+acc_no+" not present!";
	}

	Customer c  = customer_map.at(stoi(acc_no));
	std::vector<Transaction> &v = transaction_map[int_acc_no];
	long amount = stol(amt);
	if(!c.can_withdraw(amount)) {
		std::cout<<"Can't perform withdrawal for "<<c.getName()
										<<". Current balance is "<<c.getBalance()<<" less than requested amount: "
										<<amount<<std::endl;
		std::stringstream stream;
		stream << std::fixed << std::setprecision(2) << c.getBalance();
		msg = "Withdrawal fail for customer: "+acc_no+". Tried to withdraw $"+amt+
				" but available balance is: $"+stream.str();

	}else{
		TransactionBuilder b;
		Transaction trans = b.set_account_number(int_acc_no)
												.set_timestamp(tstamp)
												.set_transaction_type('W')
												.set_amount(amount)
												.build();
		v.push_back(trans);
		c.reduce_money(amount);
		update_customer_map(c);
		msg = "Successfull withdrawl: $"+amt+", for customer: "+acc_no;
	}
	return msg;
	//std::cout<<"Successfull withdrawl :"<<amount<<", balance is : "<<c.getBalance()<<std::endl;
}

std::string BankServer::deposit(std::string tstamp, std::string acc_no, std::string amt){
	std::string msg;
	int int_acc_no = stoi(acc_no);
	if (customer_map.find(int_acc_no) == customer_map.end()){
		std::cout<<"Cutomer id : "<<int_acc_no<<" not present!"<<std::endl;
		msg = "Customer id: "+acc_no+" not present!";
	} else {
		Customer c  = customer_map.at(stoi(acc_no));
		std::vector<Transaction> &v = transaction_map[int_acc_no];
		TransactionBuilder b;
		long amount = stol(amt);
		Transaction trans = b.set_account_number(int_acc_no)
													.set_timestamp(tstamp)
													.set_transaction_type('W')
													.set_amount(amount)
													.build();
		v.push_back(trans);
		c.add_money(amount);
		update_customer_map(c);
		msg = "Successfull deposit :$"+amt+", for customer: "+acc_no;
	}
	//std::cout<<"Successfull deposit :"<<amount<<", balance is : "<<c.getBalance()<<std::endl;
	return msg;
}

void BankServer::do_action(char * data, int clientSocket){
	char * buf;
	std::string msg;
	std::string arr[4];
	splitString(arr, data);

	char choice = arr[2][0];
	switch(toupper(choice)) {
	case 'W':
		msg = withdrawal(arr[0], arr[1], arr[3]);
		break;
	case 'D':
		msg = deposit(arr[0], arr[1], arr[3]);
		break;
	default:
		break;
	}
	buf = strcpy(new char[msg.length() + 1], msg.c_str());
	send(clientSocket, buf, msg.length(), 0);
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
	file.open("./server/Records.txt");
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
		std::cout<<"loaded static data" <<customer_map.size()<<std::endl;
	}
	file.close();
}



void BankServer::print_stats(int signal_Number) {
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
	BankServer server;
	//Gui gui;
	//gui.create_window(argc, argv);
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
