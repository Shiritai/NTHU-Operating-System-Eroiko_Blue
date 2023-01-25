#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "ts_queue.hpp"

/* Global shared variables */
TSQueue<int>* q;
int num_producer;
int num_consumer;
int** result;

void* produce(void* arg) {
	int tid = *(int*)arg;

	int from = tid * num_consumer;
	int to = tid * num_consumer + num_consumer;
	for (int i = from; i < to; i++) {
		q->enqueue(i);
	}

	return nullptr;
}

void* consume(void* arg) {
	int tid = *(int*)arg;

	for (int i = 0; i < num_producer; i++) {
		int val = q->dequeue();
		result[tid][i] = val;
	}

	return nullptr;
}

struct Thread {
	pthread_t t;
	int id;
};

int main(int argc, char** argv) {
	assert(argc == 3);

	q = new TSQueue<int>(20);
	num_producer = atoi(argv[1]);
	num_consumer = atoi(argv[2]);

	result = new int*[num_consumer];
	for (int i = 0; i < num_consumer; i++)
		result[i] = new int[num_producer];

	Thread* producers = new Thread[num_producer];
	Thread* consumers = new Thread[num_consumer];

	for (int i = 0; i < num_producer; i++) {
		producers[i].id = i;
		pthread_create(&producers[i].t, 0, produce, (void*)&producers[i].id);
	}

	for (int i = 0; i < num_consumer; i++) {
		consumers[i].id = i;
		pthread_create(&consumers[i].t, 0, consume, (void*)&consumers[i].id);
	}

	for (int i = 0; i < num_producer; i++) {
		pthread_join(producers[i].t, 0);
	}
	for (int i = 0; i < num_consumer; i++) {
		pthread_join(consumers[i].t, 0);
	}

	for (int i = 0; i < num_consumer; i++) {
		printf("consumer %d:", i);
		for (int j = 0; j < num_producer; j++)
			printf(" %d", result[i][j]);
		printf("\n");
	}

	return 0;
}
