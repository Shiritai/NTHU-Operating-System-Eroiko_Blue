#include <unistd.h>
#include "ts_queue.hpp"
#include "writer.hpp"

int main() {
	TSQueue<Item*>* q = new TSQueue<Item*>;

	Writer* writer = new Writer(80, "./tests/00.out", q);

	writer->start();

	sleep(1);

	for (int i = 0; i < 20; i++)
		q->enqueue(new Item(i, i, 'A'));

	sleep(1);

	for (int i = 0; i < 40; i++)
		q->enqueue(new Item(i + 20, i + 20, 'B'));

	sleep(1);
	for (int i = 0; i < 20; i++)
		q->enqueue(new Item(i + 60, i + 60, 'C'));

	writer->join();	

	delete writer;
	delete q;

	return 0;;
}
