#include <pthread.h>

#ifndef THREAD_HPP
#define THREAD_HPP

class Thread {
public:
	// to start a new pthread work
	virtual void start() = 0;

	// to wait for the pthread work to complete
	virtual int join();

	// to cancel the pthread work
	virtual int cancel();
protected:
	pthread_t t;
};

int Thread::join() {
	return pthread_join(t, 0);
}

int Thread::cancel() {
	return pthread_cancel(t);
}

#endif // THREAD_HPP
