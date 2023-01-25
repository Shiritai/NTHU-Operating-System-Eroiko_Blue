// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.
class Scheduler {
  public:
    Scheduler();		// Initialize list of ready threads 
    ~Scheduler();		// De-allocate ready list

    void ReadyToRun(Thread* thread);	
    				// Thread can be dispatched.
    Thread* FindNextToRun();	// Dequeue first thread on the ready 
				// list, if any, and return thread.
    void Run(Thread* nextThread, bool finishing);
    				// Cause nextThread to start running
    void CheckToBeDestroyed();// Check if thread that had been
    				// running needs to be deleted
    void Print();		// Print contents of ready list

    //+ MP3
    /** 
     * Check and Apply Aging Mechanism to System Queue
     */
    void Aging();

    /** 
     * Check whether we should preempt current thread
     */
    bool ShouldPreempt();
    
    // SelfTest for scheduler is implemented in class Thread
    

    //+ MP3
    
  private:
    //+ MP3 SystemQueue declaration
    /** 
     * System Ready Queue, with three level mechanism
     */
    class SystemQueue {
    private:
        // level in system queue, integer
        enum Level { 
            Lv1 = 1, // level 1 (highest level)
            Lv2 = 2, // level 2 (median level)
            Lv3 = 3, // level 3 (lowest level)
        };

        /** 
         * Get the level of priority queues w.r.t. given thread
         * @return SystemQueue::Level (integer)
         */
        Level getLevel(Thread * t);

        SortedList<Thread *> * L1; // L1 ready queue (priority 100 ~ 149), preemptive SJF
        SortedList<Thread *> * L2; // L2 ready queue (priority 50 ~ 99), non-preemptive Priority Queue
        List<Thread *> * L3; // L3 ready queue (priority 0 ~ 49), round robin per 100 ticks

    public:
        SystemQueue();
        ~SystemQueue();


        /** 
         * Append a thread to System Queue
         * @param thread the thread to append
         */
        void Append(Thread * thread);

        /** 
         * Check whether System Queue is empty
         */
        bool IsEmpty();

        /** 
         * Remove the front thread in System Queue
         */
        Thread * RemoveFront();

        /** 
         * Peek the front thread in System Queue
         */
        Thread * Front();

        /** 
         * Apply a callback function to all the threads
         */
        void Apply(void (* callback)(Thread *));

        /** 
         * Apply Aging Mechanism to System Queue
         */
        void Aging();

        /** 
         * Check whether we should preempt current thread
         */
        bool ShouldPreempt();
    };
  
    //- List<Thread *> *readyList; // queue of threads that are ready to run,
    //+ MP3
    SystemQueue * readyList; // queue of threads that are ready to run,
				// but not running
    Thread *toBeDestroyed;	// finishing thread to be destroyed
    				// by the next thread that runs
};

#endif // SCHEDULER_H
