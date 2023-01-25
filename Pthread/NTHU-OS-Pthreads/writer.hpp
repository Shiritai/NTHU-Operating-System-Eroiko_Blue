#include <fstream>
#include "thread.hpp"
#include "ts_queue.hpp"
#include "item.hpp"

#ifndef WRITER_HPP
#define WRITER_HPP

class Writer : public Thread {
public:
	// constructor
	Writer(int expected_lines, std::string output_file, TSQueue<Item*>* output_queue);

	// destructor
	~Writer();

	virtual void start() override;
private:
	// the expected lines to write,
	// the writer thread finished after output expected lines of item
	int expected_lines;

	std::ofstream ofs;
	TSQueue<Item*> *output_queue;

	// the method for pthread to create a writer thread
	static void* process(void* arg);
};

// Implementation start

Writer::Writer(int expected_lines, std::string output_file, TSQueue<Item*>* output_queue)
	: expected_lines(expected_lines), output_queue(output_queue) {
	// std::cout << "Writer::Writer" << std::endl;
	ofs = std::ofstream(output_file);
	// std::cout << "Writer::Writer" << std::endl;
}

Writer::~Writer() {
	// std::cout << "Writer::~Writer" << std::endl;
	ofs.close();
	// std::cout << "Writer::~Writer" << std::endl;
}

void Writer::start() {
	// TODO: starts a Writer thread
	// std::cout << "Writer::start" << std::endl;
	pthread_create(&t, nullptr, Writer::process, (void *) this);
	// std::cout << "Writer::start" << std::endl;
}

void* Writer::process(void* arg) {
	// TODO: implements the Writer's work
	// std::cout << "Writer::process" << std::endl;
	auto writer = (Writer *) arg;
	while (writer->expected_lines--) 
		writer->ofs << *writer->output_queue->dequeue();
	// std::cout << "Writer::process" << std::endl;
	return nullptr;
}

#endif // WRITER_HPP
