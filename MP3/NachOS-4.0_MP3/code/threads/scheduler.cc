// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------
Scheduler::Scheduler()
{ 
    //- readyList = new List<Thread *>; 
    //+ MP3
    readyList = new SystemQueue(); 
    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------
Scheduler::~Scheduler()
{ 
    delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------
void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    readyList->Append(thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------
Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (readyList->IsEmpty()) {
		return NULL;
    } else {
    	return readyList->RemoveFront();
    }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------
void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	    oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    
    //* MP3
    nextThread->setStatus(RUNNING, oldThread);  // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	    oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------
void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}

//+ MP3

void Scheduler::Aging() {
    readyList->Aging();
}

bool Scheduler::ShouldPreempt() {
    return readyList->ShouldPreempt();
}

//+ MP3 SystemQueue implementation

Scheduler::SystemQueue::SystemQueue(): 
    L1(new SortedList<Thread*>(Thread::cmpRemainTime)), 
    L2(new SortedList<Thread*>(Thread::cmpPriority)), 
    L3(new List<Thread*>()) {}

Scheduler::SystemQueue::~SystemQueue() {
    delete L1;
    delete L2;
    delete L3;
}

void Scheduler::SystemQueue::Append(Thread * thread) {
    int level = getLevel(thread);
    switch (level) {
    case Lv1:
        L1->Insert(thread);
        break;
    case Lv2:
        L2->Insert(thread);
        break;
    case Lv3:
        L3->Append(thread);
        break;
    default:
        ASSERT_MSG(FALSE, "Bad queue level, maybe the SystemQueue::getLevel method corrupts"); // error, abort
        break; // never reach
    }
    DEBUG(dbgSch, "[A] Tick [" 
        << kernel->stats->totalTicks << "]: Thread [" 
        << thread->getID() << "] is inserted into queue L[" 
        << level << "]"); // print debug message
}

Thread * Scheduler::SystemQueue::RemoveFront() {
    ASSERT_MSG(!IsEmpty(), 
        "Can't remove thread in SystemQueue when queue is empty");
    
    Thread * ret;
    Level level;

    if (!L1->IsEmpty()) {
        ret = L1->RemoveFront();
        level = Lv1;
    }
    else if (!L2->IsEmpty()) {
        ret = L2->RemoveFront();
        level = Lv2;
    }
    else {
        ret = L3->RemoveFront();
        level = Lv3;
    }
    
    DEBUG(dbgSch, "[B] Tick [" << kernel->stats->totalTicks 
        << "]: Thread [" << ret->getID() 
        << "] is removed from queue L[" << level << "]");

    return ret;
}

Thread * Scheduler::SystemQueue::Front() {
    if (!L1->IsEmpty())
        return L1->Front();
    else if (!L2->IsEmpty())
        return L2->Front();
    else if (!L3->IsEmpty())
        return L3->Front();
    
    ASSERT_MSG(FALSE, "There is no element in SystemQueue"); // error, abort
    return NULL; // never reach
}

void Scheduler::SystemQueue::Apply(void (* callback)(Thread *)) {
    L1->Apply(callback);
    L2->Apply(callback);
    L3->Apply(callback);
}

bool Scheduler::SystemQueue::IsEmpty() {
    return L1->IsEmpty() && L2->IsEmpty() && L3->IsEmpty();
}

void Scheduler::SystemQueue::Aging() {
    Apply(Thread::Aging); // apply aging to all threads
    Thread * to_ch = NULL; // caching the thread to be changed

    // Update L3 queue
    for (ListIterator<Thread *> it(L3); ; it.Next()) {
        if (to_ch) {
            L3->Remove(to_ch);
            Append(to_ch);
            to_ch = NULL; // reset to_ch cache state
        }
        if (it.IsDone())
            break;
        if (getLevel(it.Item()) != Lv3)
            to_ch = it.Item(); // set to_ch cache state
    }

    // Update L2 queue
    for (ListIterator<Thread *> it(L2); ; it.Next()) {
        if (to_ch) {
            L2->Remove(to_ch);
            Append(to_ch);
            to_ch = NULL; // reset to_ch cache state
        }
        if (it.IsDone())
            break;
        if (getLevel(it.Item()) != Lv2)
            to_ch = it.Item(); // set to_ch cache state
    }
}

bool Scheduler::SystemQueue::ShouldPreempt() {
    Thread * t = kernel->currentThread;
    bool ret = false;

    /** 
     * if cur is L3 and has burst 100 ticks, then preempt (round robin)
     */
    if (getLevel(t) == Lv3) {
        // DEBUG(dbgSch, "[P] Tick [" << kernel->stats->totalTicks 
        //     << "]: current thread[" << t->getID() << "] will be preempted since it is L[3]");
        ret = true;
    }
    /** 
     * if L1 not empty 
     * -> if cur is L2, then preempt
     * -> if cur is L1 but has less priority, then preempt
     **/
    else if (!L1->IsEmpty() && (getLevel(t) == Lv2 || Thread::cmpRemainTime(t, L1->Front()) > 0)) {
        // DEBUG(dbgSch, "[P] Tick [" << kernel->stats->totalTicks 
        //     << "]: current thread[" << t->getID() 
        //     << "] will be preempted by thread[" << L1->Front()->getID() 
        //     << "] since cmp_burst(t, L1->Front()) = " 
        //     << Thread::cmpRemainTime(t, L1->Front()));
        ret = true;
    }
    // else {
    //     DEBUG(dbgSch, "[P] Tick [" << kernel->stats->totalTicks 
    //         << "]: no preemption for thread[" << t->getID() << "]");
    // }

    return ret;
}

Scheduler::SystemQueue::Level Scheduler::SystemQueue::getLevel(Thread * t) {
    int p = t->getPriority();
    if (p >= 0 && p < 50)
        return Lv3;
    else if (p >= 50 && p < 100)
        return Lv2;
    else
        return Lv1;
}