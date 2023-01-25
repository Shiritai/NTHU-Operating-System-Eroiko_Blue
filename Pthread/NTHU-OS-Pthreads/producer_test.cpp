#include "ts_queue.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "producer.hpp"

int main() {
	TSQueue<Item*>* q1;
	TSQueue<Item*>* q2;

	q1 = new TSQueue<Item*>;
	q2 = new TSQueue<Item*>;

	Transformer* transformer = new Transformer;

	Reader* reader = new Reader(80, "./tests/00.in", q1);
	Writer* writer = new Writer(80, "./tests/00.out", q2);

	Producer* p1 = new Producer(q1, q2, transformer);
	Producer* p2 = new Producer(q1, q2, transformer);
	Producer* p3 = new Producer(q1, q2, transformer);
	Producer* p4 = new Producer(q1, q2, transformer);

	reader->start();
	writer->start();

	p1->start();
	p2->start();
	p3->start();
	p4->start();

	reader->join();
	writer->join();

	delete p2;
	delete p1;
	delete writer;
	delete reader;
	delete q2;
	delete q1;

	return 0;
}
