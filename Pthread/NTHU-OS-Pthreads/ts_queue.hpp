#include <pthread.h>

#ifndef TS_QUEUE_HPP
#define TS_QUEUE_HPP

#define DEFAULT_BUFFER_SIZE 200

template <class T>
class TSQueue {
public:
	// constructor
	TSQueue();

	explicit TSQueue(int max_buffer_size);

	// destructor
	~TSQueue();

	// add an element to the end of the queue
	void enqueue(T item);

	// remove and return the first element of the queue
	T dequeue();

	// return the number of elements in the queue
	int get_size();
private:
	// the maximum buffer size
	int buffer_size;
	// the buffer containing values of the queue
	T* buffer;
	// the current size of the buffer
	int size;
	// the index of first item in the queue
	int head;
	// the index of last item in the queue
	int tail;

	// pthread mutex lock
	pthread_mutex_t mutex;
	// pthread conditional variable
	pthread_cond_t cond_enqueue, cond_dequeue;
};

// Implementation start

template <class T>
TSQueue<T>::TSQueue() : TSQueue(DEFAULT_BUFFER_SIZE) {
}

template <class T>
TSQueue<T>::TSQueue(int buffer_size) : buffer_size(buffer_size) {
	// TODO: implements TSQueue constructor
	// initialize members
	buffer = new T[buffer_size];
	size = 0;
	head = buffer_size - 1;
	tail = 0;
	// initialize mutex for critical section
	pthread_mutex_init(&mutex, NULL);
	// initialize conditional variables
	pthread_cond_init(&cond_enqueue, nullptr);
	pthread_cond_init(&cond_dequeue, nullptr);
}

template <class T>
TSQueue<T>::~TSQueue() {
	// TODO: implements TSQueue destructor
	// free members
	delete [] buffer;
	// destroy condition variables
	pthread_cond_destroy(&cond_enqueue);
	pthread_cond_destroy(&cond_dequeue);
	// delete the mutex
	pthread_mutex_destroy(&mutex);
}

template <class T>
void TSQueue<T>::enqueue(T item) {
	// TODO: enqueues an element to the end of the queue
	pthread_mutex_lock(&mutex); // enter critical section
	// ****** critical section ******
	// block if the queue is full
	while (size == buffer_size)
		pthread_cond_wait(&cond_enqueue, &mutex);
	// enqueue, size < buffer_size
	buffer[tail] = item;
	tail = (tail + 1) % buffer_size;
	++size;
	// notify dequeue since we have at least one element
	pthread_cond_signal(&cond_dequeue);
	// ****** critical section ******
	pthread_mutex_unlock(&mutex); // leave critical section
}

template <class T>
T TSQueue<T>::dequeue() {
	// TODO: dequeues the first element of the queue
	T ret; // element to be dequeued
	pthread_mutex_lock(&mutex); // enter critical section
	// ****** critical section ******
	// block if no element to be dequeue
	while (!size)
		pthread_cond_wait(&cond_dequeue, &mutex);
	// dequeue, size >= 1
	head = (head + 1) % buffer_size;
	ret = buffer[head];
	--size;
	// notify enqueue since we have a least one empty space
	pthread_cond_signal(&cond_enqueue);
	// ****** critical section ******
	pthread_mutex_unlock(&mutex); // leave critical section
	return ret;
}

template <class T>
int TSQueue<T>::get_size() {
	// TODO: returns the size of the queue
	return size;
}

#endif // TS_QUEUE_HPP
