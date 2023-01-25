#include <pthread.h>
#include <unistd.h>
#include <signal.h> // for time interval and signal handler
#include <sys/time.h> // for time interval and signal handler
#include <vector>
#include <iostream>
#include "consumer.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "transformer.hpp"

#ifndef CONSUMER_CONTROLLER
#define CONSUMER_CONTROLLER

// recorder for consumer numbers for experiment
std::vector<int> * rec_ptr = nullptr;
bool print_scaling_msg = true;

class ConsumerController : public Thread {
public:
	// constructor
	ConsumerController(
		TSQueue<Item*>* worker_queue,
		TSQueue<Item*>* writer_queue,
		Transformer* transformer,
		int check_period,
		int low_threshold,
		int high_threshold
	);

	// destructor
	~ConsumerController();

	virtual void start();

private:
	std::vector<Consumer*> consumers;

	TSQueue<Item*>* worker_queue;
	TSQueue<Item*>* writer_queue;

	Transformer* transformer;

	// Check to scale down or scale up every check period in microseconds.
	int check_period;
	// When the number of items in the worker queue is lower than low_threshold,
	// the number of consumers scaled down by 1.
	int low_threshold;
	// When the number of items in the worker queue is higher than high_threshold,
	// the number of consumers scaled up by 1.
	int high_threshold;

	static void* process(void* arg);

	// global consumer controller
	static ConsumerController * global_ctr;
	
	// signal handler for global consumer controller
	static void handler(int signo);
};

// Implementation start

ConsumerController::ConsumerController(
	TSQueue<Item*>* worker_queue,
	TSQueue<Item*>* writer_queue,
	Transformer* transformer,
	int check_period,
	int low_threshold,
	int high_threshold
) : worker_queue(worker_queue),
	writer_queue(writer_queue),
	transformer(transformer),
	check_period(check_period),
	low_threshold(low_threshold),
	high_threshold(high_threshold) {
}

ConsumerController::~ConsumerController() {
	// std::cout << "ConsumerController::~ConsumerController" << std::endl;
	if (global_ctr == this)
		global_ctr = nullptr;
	// std::cout << "ConsumerController::~ConsumerController" << std::endl;
}

ConsumerController * ConsumerController::global_ctr = nullptr;

void ConsumerController::start() {
	// TODO: starts a ConsumerController thread
	// std::cout << "ConsumerController::start" << std::endl;
	if (rec_ptr) {
		rec_ptr->clear();
		rec_ptr->push_back(0);
	}
	global_ctr = this;
	pthread_create(&t, nullptr, ConsumerController::process, (void *) this);
	// std::cout << "ConsumerController::start" << std::endl;
}

void ConsumerController::handler(int signo) {
	// std::cout << "ConsumerController::handler" << std::endl;
	if (global_ctr) {
		int wk_sz = global_ctr->worker_queue->get_size(), // size of work_queue
			cm_sz = global_ctr->consumers.size(); // size of consumer
		switch (signo) {
		case SIGALRM:
			if (wk_sz > global_ctr->high_threshold) {
				global_ctr->consumers.push_back(new Consumer(global_ctr->worker_queue, global_ctr->writer_queue, global_ctr->transformer));
				global_ctr->consumers.back()->start();
				if (print_scaling_msg)
					std::cout << "Scaling up consumers from " << cm_sz << " to " << (cm_sz + 1) << std::endl;
				if (rec_ptr) 
					rec_ptr->push_back(cm_sz + 1);
			}
			else if (wk_sz < global_ctr->low_threshold && cm_sz > 1) {
				global_ctr->consumers.back()->cancel();
				global_ctr->consumers.pop_back();
				if (print_scaling_msg)
					std::cout << "Scaling down consumers from " << cm_sz << " to " << (cm_sz - 1) << std::endl;
				if (rec_ptr) 
					rec_ptr->push_back(cm_sz + 1);
			}
			break;
		}
	}
	// std::cout << "ConsumerController::handler" << std::endl;
}

void* ConsumerController::process(void* arg) {
	// TODO: implements the ConsumerController's work
	// std::cout << "ConsumerController::process" << std::endl;
	ConsumerController * cur = (ConsumerController *) arg;
	
	// signal action alarm handler
	struct sigaction act; // make a signal action
	act.sa_handler = handler; // assign handler
	sigaction(SIGALRM, &act, nullptr); // set signal triggering
	
	// set periodic timer
	long u_period = cur->check_period; // interval in microsecond
	itimerval itv; // interval timer
	itv.it_interval = itv.it_value = { u_period / 1000000, u_period % 1000000 }; // init timer value
	setitimer(ITIMER_REAL, &itv, nullptr); // set timer w.r.t. real system time
	
	// push the first consumer
	// cur->consumers.push_back(new Consumer(cur->worker_queue, cur->writer_queue, cur->transformer));
	// cur->consumers.back()->start();
	// std::cout << "Scaling up consumers from 0 to 1" << std::endl;
	
	// infinite loop
	while (global_ctr);
	
	// std::cout << "ConsumerController::process" << std::endl;
	return nullptr;
}

#endif // CONSUMER_CONTROLLER_HPP
