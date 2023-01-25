# Note

```cpp
bool 
AddrSpace::Load(char *fileName) {
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;

    if (executable == NULL) 
        return FALSE;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    // manipulate noffH.noffMagic when 
    //      the host is big endian
    if ((noffH.noffMagic != NOFFMAGIC) && 
        (WordToHost(noffH.noffMagic) == NOFFMAGIC)) {
        // swap from little endian to big endian format
        SwapHeader(&noffH);
    }
    // noffH.noffMagic should be NOFFMAGIC, 
    //      marking that file-read works successfully
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    // Before virtual memory is implemented,
    //      we should check this
    ASSERT(numPages <= NumPhysPages);

    /** 
     * copy in our code and data segments into memory
     * Note: this code assumes that
     * virtual address == physical address
     * 
     * Reading content of executable to Memory if
     * the corresponding segment exist
     */

    if (noffH.code.size > 0) {
        /* Initializing code segment */
        executable->ReadAt(
        &(kernel->machine->mainMemory[noffH.code.virtualAddr]), 
            noffH.code.size, noffH.code.inFileAddr);
    }

    if (noffH.initData.size > 0) {
        /* Initializing data segment */
        executable->ReadAt(
        &(kernel->machine->mainMemory[noffH.initData.virtualAddr]),
            noffH.initData.size, noffH.initData.inFileAddr);
    }

#ifdef RDATA // read only case
    if (noffH.readonlyData.size > 0) {
        /* Initializing read only data segment */
        executable->ReadAt(
        &(kernel->machine->mainMemory[noffH.readonlyData.virtualAddr]),
            noffH.readonlyData.size, noffH.readonlyData.inFileAddr);
    }
#endif

    delete executable; // close file
    return TRUE; // success
}
```

```cpp
void Kernel::ExecAll() {
    for (int i=1;i<=execfileNum;i++) {
        int a = Exec(execfile[i]);
    }
    currentThread->Finish();
    //Kernel::Exec();    
}
```

```cpp
int Kernel::Exec(char* name)
{
    t[threadNum] = new Thread(name, threadNum);
    t[threadNum]->space = new AddrSpace();
    t[threadNum]->Fork((VoidFunctionPtr) &ForkExecute, (void *)t[threadNum]);
    threadNum++;

    return threadNum-1;
}
```

```cpp
//     Initialize a thread control block, so that we can then call
//    Thread::Fork.
//
//    "threadName" is an arbitrary string, useful for debugging.
Thread::Thread(char* threadName, int threadID)
{
    ID = threadID;
    name = threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
    for (int i = 0; i < MachineStateSize; i++) {
    machineState[i] = NULL;        // not strictly necessary, since
                    // new thread ignores contents 
                    // of machine registers
    }
    space = NULL;
}
```

```cpp
AddrSpace::AddrSpace()
{
    pageTable = new TranslationEntry[NumPhysPages];
    for (int i = 0; i < NumPhysPages; i++) {
        pageTable[i].virtualPage = i;    // for now, virt page # = phys page #
        pageTable[i].physicalPage = i;
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  
    }
    
    // zero out the entire address space
    bzero(kernel->machine->mainMemory, MemorySize); // memset to zero
}
```

```cpp
void
Thread::StackAllocate (VoidFunctionPtr func, void *arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(int));
    machineState[PCState] = (void*)ThreadRoot;
    machineState[StartupPCState] = (void*)ThreadBegin;
    machineState[InitialPCState] = (void*)func;
    machineState[InitialArgState] = (void*)arg;
    machineState[WhenDonePCState] = (void*)ThreadFinish;
}
```

```cpp
void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
    thread->setStatus(READY);
    readyList->Append(thread);
}
```

```cpp
void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {    // mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
     toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {    // if this thread is a user program,
        oldThread->SaveUserState();     // save the user's CPU registers
    oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();            // check if the old thread
                        // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
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

    CheckToBeDestroyed();        // check if thread we were running
                    // before this one has finished
                    // and needs to be cleaned up
    
    if (oldThread->space != NULL) {        // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
    oldThread->space->RestoreState();
    }
}
```
