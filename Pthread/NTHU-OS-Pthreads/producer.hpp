#include <pthread.h>
#include "thread.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "transformer.hpp"

#ifndef PRODUCER_HPP
#define PRODUCER_HPP

class Producer : public Thread {
public:
	// constructor
	Producer(TSQueue<Item*>* input_queue, TSQueue<Item*>* worker_queue, Transformer* transformer);

	// destructor
	~Producer();

	virtual void start();
private:
	TSQueue<Item*>* input_queue;
	TSQueue<Item*>* worker_queue;

	Transformer* transformer;

	// the method for pthread to create a producer thread
	static void* process(void* arg);
};

Producer::Producer(TSQueue<Item*>* input_queue, TSQueue<Item*>* worker_queue, Transformer* transformer)
	: input_queue(input_queue), worker_queue(worker_queue), transformer(transformer) {
	// std::cout << "Producer::Producer" << std::endl;
	// std::cout << "Producer::Producer" << std::endl;
}

Producer::~Producer() {
	// std::cout << "Producer::~Producer" << std::endl;
	// std::cout << "Producer::~Producer" << std::endl;
}

void Producer::start() {
	// TODO: starts a Producer thread
	// std::cout << "Producer::start" << std::endl;
	pthread_create(&t, nullptr, Producer::process, (void *) this);
	// std::cout << "Producer::start" << std::endl;
}

void* Producer::process(void* arg) {
	// TODO: implements the Producer's work
	// std::cout << "Producer::process" << std::endl;
	auto producer = (Producer *) arg;
	while (true) {
		if (producer->input_queue->get_size()) {
			Item * it = producer->input_queue->dequeue(); // item to be processed and deleted
			auto val = producer->transformer->producer_transform(it->opcode, it->val); // new value
			producer->worker_queue->enqueue(new Item(it->key, val, it->opcode));
			delete it;
		}
	}
	// std::cout << "Producer::process" << std::endl;
	return nullptr;
}

#endif // PRODUCER_HPP
