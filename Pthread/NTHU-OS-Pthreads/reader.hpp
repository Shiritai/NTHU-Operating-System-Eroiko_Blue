#include <fstream>
#include "thread.hpp"
#include "ts_queue.hpp"
#include "item.hpp"

#ifndef READER_HPP
#define READER_HPP

class Reader : public Thread {
public:
	// constructor
	Reader(int expected_lines, std::string input_file, TSQueue<Item*>* input_queue);

	// destructor
	~Reader();

	virtual void start() override;
private:
	// the expected lines to read,
	// the reader thread finished after input expected lines of item
	int expected_lines;

	std::ifstream ifs;
	TSQueue<Item*>* input_queue;

	// the method for pthread to create a reader thread
	static void* process(void* arg);
};

// Implementation start

Reader::Reader(int expected_lines, std::string input_file, TSQueue<Item*>* input_queue)
	: expected_lines(expected_lines), input_queue(input_queue) {
	// std::cout << "Reader::Reader" << std::endl;
	ifs = std::ifstream(input_file);
	// std::cout << "Reader::Reader" << std::endl;
}

Reader::~Reader() {
	// std::cout << "Reader::~Reader" << std::endl;
	ifs.close();
	// std::cout << "Reader::~Reader" << std::endl;
}

void Reader::start() {
	// std::cout << "Reader::start" << std::endl;
	pthread_create(&t, 0, Reader::process, (void*)this);
	// std::cout << "Reader::start" << std::endl;
}

void* Reader::process(void* arg) {
	// std::cout << "Reader::process" << std::endl;
	Reader* reader = (Reader*) arg;

	while (reader->expected_lines--) {
		Item *item = new Item;
		reader->ifs >> *item;
		reader->input_queue->enqueue(item);
	}

	// std::cout << "Reader::process" << std::endl;
	return nullptr;
}

#endif // READER_HPP
