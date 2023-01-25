// thread.cc 
//	Routines to manage threads.  These are the main operations:
//
//	Fork -- create a thread to run a procedure concurrently
//		with the caller (this is done in two steps -- first
//		allocate the Thread object, then call Fork on it)
//	Begin -- called when the forked procedure starts up, to turn
//		interrupts on and clean up after last thread
//	Finish -- called when the forked procedure finishes, to clean up
//	Yield -- relinquish control over the CPU to another ready thread
//	Sleep -- relinquish control over the CPU, but thread is now blocked.
//		In other words, it will not run again, until explicitly 
//		put back on the ready queue.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "sysdep.h"

// this is put at the top of the execution stack, for detecting stack overflows
const int STACK_FENCEPOST = 0xdedbeef;

//----------------------------------------------------------------------
// Thread::Thread
// 	Initialize a thread control block, so that we can then call
//	Thread::Fork.
//
//	"threadName" is an arbitrary string, useful for debugging.
//----------------------------------------------------------------------
Thread::Thread(char* threadName, int threadID, int priority)
{
	ID = threadID;
    name = threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
    
    //+ MP3
    sys_q_stub = new OrderManager(this, priority);
    
    for (int i = 0; i < MachineStateSize; i++) {
	machineState[i] = NULL;		// not strictly necessary, since
					// new thread ignores contents 
					// of machine registers
    }
    space = NULL;
}

//----------------------------------------------------------------------
// Thread::~Thread
// 	De-allocate a thread.
//
// 	NOTE: the current thread *cannot* delete itself directly,
//	since it is still running on the stack that we need to delete.
//
//      NOTE: if this is the main thread, we can't delete the stack
//      because we didn't allocate it -- we got it automatically
//      as part of starting up Nachos.
//----------------------------------------------------------------------

Thread::~Thread()
{
    DEBUG(dbgThread, "Deleting thread: " << name);
    ASSERT(this != kernel->currentThread);
    if (stack != NULL)
	DeallocBoundedArray((char *) stack, StackSize * sizeof(int));

    //+ MP3
    delete sys_q_stub;
}

//----------------------------------------------------------------------
// Thread::Fork
// 	Invoke (*func)(arg), allowing caller and callee to execute 
//	concurrently.
//
//	NOTE: although our definition allows only a single argument
//	to be passed to the procedure, it is possible to pass multiple
//	arguments by making them fields of a structure, and passing a pointer
//	to the structure as "arg".
//
// 	Implemented as the following steps:
//		1. Allocate a stack
//		2. Initialize the stack so that a call to SWITCH will
//		cause it to run the procedure
//		3. Put the thread on the ready queue
// 	
//	"func" is the procedure to run concurrently.
//	"arg" is a single argument to be passed to the procedure.
//----------------------------------------------------------------------

void 
Thread::Fork(VoidFunctionPtr func, void *arg)
{
    Interrupt *interrupt = kernel->interrupt;
    Scheduler *scheduler = kernel->scheduler;
    IntStatus oldLevel;
    
    DEBUG(dbgThread, "Forking thread: " << name << " f(a): " << (int) func << " " << arg);
    StackAllocate(func, arg);

    oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);	// ReadyToRun assumes that interrupts 
					// are disabled!
    (void) interrupt->SetLevel(oldLevel);
}    

//----------------------------------------------------------------------
// Thread::CheckOverflow
// 	Check a thread's stack to see if it has overrun the space
//	that has been allocated for it.  If we had a smarter compiler,
//	we wouldn't need to worry about this, but we don't.
//
// 	NOTE: Nachos will not catch all stack overflow conditions.
//	In other words, your program may still crash because of an overflow.
//
// 	If you get bizarre results (such as seg faults where there is no code)
// 	then you *may* need to increase the stack size.  You can avoid stack
// 	overflows by not putting large data structures on the stack.
// 	Don't do this: void foo() { int bigArray[10000]; ... }
//----------------------------------------------------------------------

void
Thread::CheckOverflow()
{
    if (stack != NULL) {
#ifdef HPUX			// Stacks grow upward on the Snakes
	ASSERT(stack[StackSize - 1] == STACK_FENCEPOST);
#else
	ASSERT(*stack == STACK_FENCEPOST);
#endif
   }
}

//----------------------------------------------------------------------
// Thread::Begin
// 	Called by ThreadRoot when a thread is about to begin
//	executing the forked procedure.
//
// 	It's main responsibilities are:
//	1. deallocate the previously running thread if it finished 
//		(see Thread::Finish())
//	2. enable interrupts (so we can get time-sliced)
//----------------------------------------------------------------------

void Thread::Begin ()
{
    ASSERT(this == kernel->currentThread);
    DEBUG(dbgThread, "Beginning thread: " << name);
    
    kernel->scheduler->CheckToBeDestroyed();
    kernel->interrupt->Enable();
}

//----------------------------------------------------------------------
// Thread::Finish
// 	Called by ThreadRoot when a thread is done executing the 
//	forked procedure.
//
// 	NOTE: we can't immediately de-allocate the thread data structure 
//	or the execution stack, because we're still running in the thread 
//	and we're still on the stack!  Instead, we tell the scheduler
//	to call the destructor, once it is running in the context of a different thread.
//
// 	NOTE: we disable interrupts, because Sleep() assumes interrupts
//	are disabled.
//----------------------------------------------------------------------

//
void Thread::Finish ()
{
    (void) kernel->interrupt->SetLevel(IntOff);		
    ASSERT(this == kernel->currentThread);
    
    DEBUG(dbgThread, "Finishing thread: " << name);
    Sleep(TRUE);				// invokes SWITCH
    // not reached
}


//----------------------------------------------------------------------
// Thread::Yield
// 	Relinquish the CPU if any other thread is ready to run.
//	If so, put the thread on the end of the ready list, so that
//	it will eventually be re-scheduled.
//
//	NOTE: returns immediately if no other thread on the ready queue.
//	Otherwise returns when the thread eventually works its way
//	to the front of the ready list and gets re-scheduled.
//
//	NOTE: we disable interrupts, so that looking at the thread
//	on the front of the ready list, and switching to it, can be done
//	atomically.  On return, we re-set the interrupt level to its
//	original state, in case we are called with interrupts disabled. 
//
// 	Similar to Thread::Sleep(), but a little different.
//----------------------------------------------------------------------
void Thread::Yield ()
{
    Thread *nextThread;
    IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
    
    ASSERT(this == kernel->currentThread);
    
    DEBUG(dbgThread, "Yielding thread: " << name);
    
    nextThread = kernel->scheduler->FindNextToRun();
    if (nextThread != NULL) {
	    kernel->scheduler->ReadyToRun(this);
	    kernel->scheduler->Run(nextThread, FALSE);
    }
    (void) kernel->interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Sleep
// 	Relinquish the CPU, because the current thread has either
//	finished or is blocked waiting on a synchronization 
//	variable (Semaphore, Lock, or Condition).  In the latter case,
//	eventually some thread will wake this thread up, and put it
//	back on the ready queue, so that it can be re-scheduled.
//
//	NOTE: if there are no threads on the ready queue, that means
//	we have no thread to run.  "Interrupt::Idle" is called
//	to signify that we should idle the CPU until the next I/O interrupt
//	occurs (the only thing that could cause a thread to become
//	ready to run).
//
//	NOTE: we assume interrupts are already disabled, because it
//	is called from the synchronization routines which must
//	disable interrupts for atomicity.   We need interrupts off 
//	so that there can't be a time slice between pulling the first thread
//	off the ready list, and switching to it.
//----------------------------------------------------------------------
void
Thread::Sleep (bool finishing)
{
    Thread *nextThread;
    
    ASSERT(this == kernel->currentThread);
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    
    DEBUG(dbgThread, "Sleeping thread: " << name);
    DEBUG(dbgTraCode, "In Thread::Sleep, Sleeping thread: " << name << ", " << kernel->stats->totalTicks);

    // status = BLOCKED;
    //+ MP3
    if (finishing)
        setStatus(TERMINATED); // running -> terminated
    else 
        setStatus(BLOCKED); // running -> waiting
    
	//cout << "debug Thread::Sleep " << name << "wait for Idle\n";
    while ((nextThread = kernel->scheduler->FindNextToRun()) == NULL) {
		kernel->interrupt->Idle();	// no one to run, wait for an interrupt
	}    
    // returns when it's time for us to run
    kernel->scheduler->Run(nextThread, finishing); 
}

//----------------------------------------------------------------------
// ThreadBegin, ThreadFinish,  ThreadPrint
//	Dummy functions because C++ does not (easily) allow pointers to member
//	functions.  So we create a dummy C function
//	(which we can pass a pointer to), that then simply calls the 
//	member function.
//----------------------------------------------------------------------

static void ThreadFinish()    { kernel->currentThread->Finish(); }
static void ThreadBegin() { kernel->currentThread->Begin(); }
void ThreadPrint(Thread *t) { t->Print(); }

#ifdef PARISC

//----------------------------------------------------------------------
// PLabelToAddr
//	On HPUX, function pointers don't always directly point to code,
//	so we need to do the conversion.
//----------------------------------------------------------------------

static void *
PLabelToAddr(void *plabel)
{
    int funcPtr = (int) plabel;

    if (funcPtr & 0x02) {
        // L-Field is set.  This is a PLT pointer
        funcPtr -= 2;	// Get rid of the L bit
        return (*(void **)funcPtr);
    } else {
        // L-field not set.
        return plabel;
    }
}
#endif

//----------------------------------------------------------------------
// Thread::StackAllocate
//	Allocate and initialize an execution stack.  The stack is
//	initialized with an initial stack frame for ThreadRoot, which:
//		enables interrupts
//		calls (*func)(arg)
//		calls Thread::Finish
//
//	"func" is the procedure to be forked
//	"arg" is the parameter to be passed to the procedure
//----------------------------------------------------------------------
void Thread::StackAllocate (VoidFunctionPtr func, void *arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(int));

#ifdef PARISC
    // HP stack works from low addresses to high addresses
    // everyone else works the other way: from high addresses to low addresses
    stackTop = stack + 16;	// HP requires 64-byte frame marker
    stack[StackSize - 1] = STACK_FENCEPOST;
#endif

#ifdef SPARC
    stackTop = stack + StackSize - 96; 	// SPARC stack must contains at 
					// least 1 activation record 
					// to start with.
    *stack = STACK_FENCEPOST;
#endif 

#ifdef PowerPC // RS6000
    stackTop = stack + StackSize - 16; 	// RS6000 requires 64-byte frame marker
    *stack = STACK_FENCEPOST;
#endif 

#ifdef DECMIPS
    stackTop = stack + StackSize - 4;	// -4 to be on the safe side!
    *stack = STACK_FENCEPOST;
#endif

#ifdef ALPHA
    stackTop = stack + StackSize - 8;	// -8 to be on the safe side!
    *stack = STACK_FENCEPOST;
#endif


#ifdef x86
    // the x86 passes the return address on the stack.  In order for SWITCH() 
    // to go to ThreadRoot when we switch to this thread, the return addres 
    // used in SWITCH() must be the starting address of ThreadRoot.
    stackTop = stack + StackSize - 4;	// -4 to be on the safe side!
    *(--stackTop) = (int) ThreadRoot;
    *stack = STACK_FENCEPOST;
#endif
    
#ifdef PARISC
    machineState[PCState] = PLabelToAddr(ThreadRoot);
    machineState[StartupPCState] = PLabelToAddr(ThreadBegin);
    machineState[InitialPCState] = PLabelToAddr(func);
    machineState[InitialArgState] = arg;
    machineState[WhenDonePCState] = PLabelToAddr(ThreadFinish);
#else
    machineState[PCState] = (void*)ThreadRoot;
    machineState[StartupPCState] = (void*)ThreadBegin;
    machineState[InitialPCState] = (void*)func;
    machineState[InitialArgState] = (void*)arg;
    machineState[WhenDonePCState] = (void*)ThreadFinish;
#endif
}

#include "machine.h"

//----------------------------------------------------------------------
// Thread::SaveUserState
//	Save the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine saves the former.
//----------------------------------------------------------------------

void
Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	userRegisters[i] = kernel->machine->ReadRegister(i);
}

//----------------------------------------------------------------------
// Thread::RestoreUserState
//	Restore the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine restores the former.
//----------------------------------------------------------------------

void
Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	kernel->machine->WriteRegister(i, userRegisters[i]);
}


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

static void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	cout << "*** thread " << which << " looped " << num << " times\n";
        kernel->currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// Thread::SelfTest
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------
void
Thread::SelfTest()
{
    DEBUG(dbgThread, "Entering Thread::SelfTest");

    Thread *t = new Thread("forked thread", 1);

    t->Fork((VoidFunctionPtr) SimpleThread, (void *) 1);
    kernel->currentThread->Yield();
    SimpleThread(0);
}

//+ MP3

void Thread::setStatus(ThreadStatus st, Thread * last){
    if (st == READY) {
        switch (status) {
        case JUST_CREATED:
            // DEBUG(myDbgSch, "[new] -> [ready]");
            sys_q_stub->newToReady();
            break;
        case RUNNING:
            // DEBUG(myDbgSch, "[run] -> [ready]");
            sys_q_stub->runToReady();
            break;
        case BLOCKED:
            // DEBUG(myDbgSch, "[wait] -> [ready]");
            sys_q_stub->waitToReady();
            break;
        default:
            ASSERT_MSG(FALSE, "Invalid status transfer from " 
                << status << " to " << st); // error, abort
            break; // never reach
        }
    }
    else if (status == RUNNING && st == BLOCKED) {
        // DEBUG(myDbgSch, "[run] -> [wait]");
        sys_q_stub->runToWait();
    }
    else if (st == RUNNING) {
        ASSERT_MSG(status == READY, 
            "Can only transfer to running from ready status, but got " << status); // must be ready -> run
        // DEBUG(myDbgSch, "[ready] -> [run]");
        sys_q_stub->readyToRun(last);
    }
    else if (st == TERMINATED) {
        ASSERT_MSG(status == RUNNING, 
            "Can only terminate a thread from running status, but got " << status); // must be run -> terminate
        // DEBUG(myDbgSch, "[run] -> [terminate]");
        DEBUG(dbgThread, "Thread terminating");
        sys_q_stub->runToTerminated();
    }
    
    status = st;
}

void Thread::Aging(Thread * thread) {
    thread->sys_q_stub->aging();
}

int Thread::cmpRemainTime(Thread * t1, Thread * t2) {
    // compare the remain burst time is only meaningful
    // in one of the below situations
    ASSERT_MSG((t1->status == READY && t2->status == RUNNING) ||
               (t2->status == READY && t1->status == RUNNING) ||
               (t1->status == READY && t2->status == READY), 
               "attempt to compare the priority of two thread[" << t1->ID 
               << "] (status:" << t1->status << ") and thread[" << t2->ID 
               << "] (status:" << t2->status 
               << "), the status of them may be invalid");
    return static_cast<int>(t1->sys_q_stub->getRemainTime() - 
                            t2->sys_q_stub->getRemainTime());
}

int Thread::cmpPriority(Thread * t1, Thread * t2) {
    // compare the priority is only meaningful
    // in one of the below situations
    ASSERT_MSG((t1->status == READY && t2->status == RUNNING) ||
               (t2->status == READY && t1->status == RUNNING) ||
               (t1->status == READY && t2->status == READY), 
               "attempt to compare the priority of two thread[" << t1->ID 
               << "] (status:" << t1->status << ") and thread[" << t2->ID 
               << "] (status:" << t2->status 
               << "), the status of them may be invalid");
    return t2->sys_q_stub->getPriority() - 
           t1->sys_q_stub->getPriority();
}

//+ MP3 implementation of OrderManager

const int Thread::OrderManager::AGING_TICK = 1500;

Thread::OrderManager::OrderManager(Thread * t, int priority):
    ref(t), remain_burst_time(0), current_burst_time(0),
    last_burst_time(0), tick_cache(kernel->stats->totalTicks), 
    init_priority(priority), priority(priority) {}

void Thread::OrderManager::toReady() {
    tick_cache = kernel->stats->totalTicks;
    priority = init_priority; // reset the priority
}

void Thread::OrderManager::leaveRun() {
    // only accumulate when the thread ends running
    ASSERT_MSG(ref->status == RUNNING, 
        "attempt to accumulate current bursting time "
        << "when a thread isn't in RUNNING status"
        << ", but rather " << ref->status);
    int cur_tick = kernel->stats->totalTicks;
    current_burst_time += cur_tick - tick_cache;
    tick_cache = cur_tick;
    // save the last burst time for debug message
    last_burst_time = current_burst_time;
}

double Thread::OrderManager::getRemainTime(){
    // MP3 Spec v2 + info from discussion area
    if (ref->status == RUNNING)
        return remain_burst_time - 
            (current_burst_time + kernel->stats->totalTicks - tick_cache);
    else
        return remain_burst_time - current_burst_time;
}

int Thread::OrderManager::setPriority(int priority){
    ASSERT_MSG(priority >= 0 && priority < 150, 
        "attempt to set an invalid priority: " << priority);
    if (this->priority != priority) {
        DEBUG(dbgSch, "[C] Tick [" << kernel->stats->totalTicks 
            << "]: Thread [" << ref->ID 
            << "] changes its priority from [" 
            << this->priority << "] to [" << priority << "]");
        this->priority = priority;
    }
}

void Thread::OrderManager::aging() {
    // only apply aging mechanism in READY status
    ASSERT_MSG(ref->status == READY, 
        "attempt to apply aging mechanism when a thread[" << ref->ID 
        << "] is not in ready status");
    int age = (kernel->stats->totalTicks - tick_cache) / AGING_TICK;
    if (age) { // apply aging
        tick_cache += AGING_TICK * age; // update tick_cache
        setPriority(min(priority + 10 * age, 149));
    }
}

void Thread::OrderManager::runToWait() {
    // update burst time
    leaveRun();
    double old_remain_burst_time = remain_burst_time;

    if (current_burst_time) { // has been executed
        remain_burst_time = 0.5 * current_burst_time + 0.5 * remain_burst_time;

        DEBUG(dbgSch, "[D] Tick [" << kernel->stats->totalTicks 
            << "]: Thread [" << ref->ID 
            << "] update approximate burst time, from: [" 
            << old_remain_burst_time << "], add [" 
            << current_burst_time << "], to [" << remain_burst_time << "]");
        
        current_burst_time = 0.; // reset CPU burst time
    }
}

void Thread::OrderManager::runToReady() {
    // record the burst time before interrupt in current_burst_time
    leaveRun();
    toReady();
}

void Thread::OrderManager::readyToRun(Thread * last) {
    // update run starting tick
    tick_cache = kernel->stats->totalTicks;
    if (last) {
        DEBUG(dbgSch, "[E] Tick [" << kernel->stats->totalTicks 
            << "]: Thread [" << ref->ID 
            << "] is now selected for execution, thread [" 
            << last->ID << "] is replaced, and it has executed [" 
            << last->sys_q_stub->last_burst_time << "] ticks");
    }
    else { // the first launch (and the end) of the main thread
        DEBUG(dbgSch, "[E] Tick [" << kernel->stats->totalTicks 
            << "]: Thread [" << ref->ID 
            << "] (main thread) starts its execution");
    }
}

void Thread::OrderManager::runToTerminated() {
    // counting the last accumulated running time
    leaveRun();
}