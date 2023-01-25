// thread.h 
//	Data structures for managing threads.  A thread represents
//	sequential execution of code within a program.
//	So the state of a thread includes the program counter,
//	the processor registers, and the execution stack.
//	
// 	Note that because we allocate a fixed size stack for each
//	thread, it is possible to overflow the stack -- for instance,
//	by recursing to too deep a level.  The most common reason
//	for this occuring is allocating large data structures
//	on the stack.  For instance, this will cause problems:
//
//		void foo() { int buf[1000]; ...}
//
//	Instead, you should allocate all data structures dynamically:
//
//		void foo() { int *buf = new int[1000]; ...}
//
//
// 	Bad things happen if you overflow the stack, and in the worst 
//	case, the problem may not be caught explicitly.  Instead,
//	the only symptom may be bizarre segmentation faults.  (Of course,
//	other problems can cause seg faults, so that isn't a sure sign
//	that your thread stacks are too small.)
//	
//	One thing to try if you find yourself with seg faults is to
//	increase the size of thread stack -- ThreadStackSize.
//
//  	In this interface, forking a thread takes two steps.
//	We must first allocate a data structure for it: "t = new Thread".
//	Only then can we do the fork: "t->fork(f, arg)".
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef THREAD_H
#define THREAD_H

#include "copyright.h"
#include "utility.h"
#include "sysdep.h"
#include "machine.h"
#include "addrspace.h"

// CPU register state to be saved on context switch.  
// The x86 needs to save only a few registers, 
// SPARC and MIPS needs to save 10 registers, 
// the Snake needs 18,
// and the RS6000 needs to save 75 (!)
// For simplicity, I just take the maximum over all architectures.

#define MachineStateSize 75 


// Size of the thread's private execution stack.
// WATCH OUT IF THIS ISN'T BIG ENOUGH!!!!!
const int StackSize = (8 * 1024);	// in words


// Thread state
enum ThreadStatus { 
    JUST_CREATED, // new
    RUNNING,      // running
    READY,        // ready
    BLOCKED,      // waiting
    TERMINATED,   // terminated
    ZOMBIE        // ??
};


// The following class defines a "thread control block" -- which
// represents a single thread of execution.
//
//  Every thread has:
//     an execution stack for activation records ("stackTop" and "stack")
//     space to save CPU registers while not running ("machineState")
//     a "status" (running/ready/blocked)
//    
//  Some threads also belong to a user address space; threads
//  that only run in the kernel have a NULL address space.
class Thread {
  private:
    // NOTE: DO NOT CHANGE the order of these first two members.
    // THEY MUST be in this position for SWITCH to work.
    int *stackTop;			 // the current stack pointer
    void *machineState[MachineStateSize];  // all registers except for stackTop

  public:
    Thread(char* debugName, int threadID, int priority = 0);		// initialize a Thread 
    ~Thread(); 				// deallocate a Thread
					// NOTE -- thread being deleted
					// must not be running when delete 
					// is called

    // basic thread operations

    void Fork(VoidFunctionPtr func, void *arg); 
    				// Make thread run (*func)(arg)
    void Yield();  		// Relinquish the CPU if any 
				// other thread is runnable
    void Sleep(bool finishing); // Put the thread to sleep and 
				// relinquish the processor
    void Begin();		// Startup code for the thread	
    void Finish();  		// The thread is done executing
    
    void CheckOverflow();   	// Check if thread stack has overflowed

    //+ MP3
    /** 
     * Always change status through this method.
     * Both for outer and inner use.
     * @param last must be provided if we're READY --> RUNNING, 
     * except for the first execution of the main thread
     */
    void setStatus(ThreadStatus st, Thread * last = NULL);
    
    ThreadStatus getStatus() { return (status); }
    char* getName() { return (name); }

    //+ MP3
    /** 
     * Get the priority of this thread
     */
    int getPriority() { return sys_q_stub->getPriority(); }

    /** 
     * Compare the remain burst time of the two thread
     * @return [= 0]: t1 and t2 has the same priority.
     *         [< 0]: t1 has higher priority.
     *         [> 0]: t2 has higher priority.
     */
    static int cmpRemainTime(Thread * t1, Thread * t2);

    /** 
     * Compare the priority of the two threads
     * @return [= 0]: t1 and t2 has the same priority.
     *         [< 0]: t1 has higher priority.
     *         [> 0]: t2 has higher priority.
     */
    static int cmpPriority(Thread * t1, Thread * t2);

    /** 
     * Apply Aging Mechanism for the given thread
     */
    static void Aging(Thread * thread);

    int getID() { return (ID); }
    void Print() { cout << name; }
    void SelfTest();		// test whether thread impl is working

  private:
    // some of the private data for this class is listed above
    
    int *stack; 	 	// Bottom of the stack 
				// NULL if this is the main thread
				// (If NULL, don't deallocate stack)
    ThreadStatus status;	// ready, running, blocked, just created and terminated
    char* name;
    int ID;

    //+ MP3
    /** 
     * Manager the order of thread in scheduler queue, 
     * i.e. the stub in thread for SystemQueue class.
     * Managed by Thread and provide info for SystemQueue.
     * Isolating from the main logic of Thread for better development.
     * Note that it is accessible only for Thread class!
     */
    class OrderManager {
    private:
        Thread * ref;               // reference of thread
        double remain_burst_time;   // remaining burst time
        double last_burst_time;     // last accumulate time
        double current_burst_time;  // current running time
        int priority;               // priority of thread
        const int init_priority;    // initial priority
        int tick_cache;             // caches starting tick

        /** 
         * Set the priority of this thread 
         * with correctness sanity check
         */
        int setPriority(int priority);
        
        /** 
         * Update status of Order Manager 
         * while changing state of the thread (XXX -> READY)
         * 
         * Reset tick_cache and priority
         */
        void toReady();

        /** 
         * Update status of Order Manager 
         * while changing state of the thread (RUN -> XXX)
         * 
         * Accumulate the value of current execution time
         * when the thread ENDs running!
         */
        void leaveRun();

    public:
        static const int AGING_TICK;
            
        OrderManager(Thread * t, int priority = 0);

        /** 
         * Get remained execution time needed (approximated)
         */
        double getRemainTime();

        /** 
         * Get the priority of this thread
         */
        int getPriority() { return priority; }

        /** 
         * Apply the aging check and aging mechanism
         */
        void aging();
        
        /** 
         * Collect information when the thread is turning to wait state
         */
        void runToWait();
        
        /** 
         * Collect information when the thread start running
         * @param last must be provided except for
         * the first execution of the main thread
         */
        void readyToRun(Thread * last);
        
        /** 
         * Collect information when the thread is interrupt
         */
        void runToReady();
        
        /** 
         * Collect information when the thread is 
         * back to ready from waiting
         */
        void waitToReady() { toReady(); }
        
        /** 
         * Collect information when the thread is 
         * initialized to ready queue
         */
        void newToReady() { toReady(); }
        
        /** 
         * Collect information when the thread terminating
         */
        void runToTerminated();
        
    };
    
    OrderManager * sys_q_stub; // stub object in thread for System Queue

    void StackAllocate(VoidFunctionPtr func, void *arg);
    				// Allocate a stack for thread.
				// Used internally by Fork()

// A thread running a user program actually has *two* sets of CPU registers -- 
// one for its state while executing user code, one for its state 
// while executing kernel code.

    int userRegisters[NumTotalRegs];	// user-level CPU register state

  public:
    void SaveUserState();		// save user-level register state
    void RestoreUserState();		// restore user-level register state

    AddrSpace *space;			// User code this thread is running.
};

// external function, dummy routine whose sole job is to call Thread::Print
extern void ThreadPrint(Thread *thread);	 

// Magical machine-dependent routines, defined in switch.s

extern "C" {
// First frame on thread execution stack; 
//   	call ThreadBegin
//	call "func"
//	(when func returns, if ever) call ThreadFinish()
void ThreadRoot();

// Stop running oldThread and start running newThread
void SWITCH(Thread *oldThread, Thread *newThread);
}

#endif // THREAD_H
