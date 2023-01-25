## trace code 統整與問題討論

### How does Nachos allocate the memory space for a new thread(process)?
1. 在`fork()`的時候我們會呼叫 `StackAllocate(func, arg)`
2. 其中又執行 `stack = (int *) AllocBoundedArray(StackSize * sizeof(int));`
3. 其中 `AllocBoundedArray` 會回傳記憶體位置, 其位置到加上 Size 的區塊為使用的記憶體, 其上下會有保護緩衝區。

### How does Nachos initialize the memory content of a thread(process), including loading the user binary code in the memory?

1. 我們會透過 `AddrSpace()` 來初始化 Thread 的 space , 在 `AddrSpace()`時, `bzero()` 會將 `kernel->machine->mainMemory` 都設成 0
2. 當我們都將 thread 送到 `readyList` 後,會呼叫 `Finish()`,其會把 interrupt 關掉,並呼叫 `sleep()`, 接著依情況進入 `kernel->scheduler->Run(nextThread, finishing)` , 隨後進行 `context switch` 就會執行 `ThreadRoot` 
3. `ThreadRoot` 又會執行 `ThreadBegin`,將 interrupt 打開,接著執行`func`
4. `func` 就是 `ForkExecute()` 包含 `AddrSpace::Load()`,`AddrSpace::Execute()`
5. 其中 `AddrSpace::Load()` 會我們要執行的資料讀進 `mainMemory`

    ```c++
    if (noffH.code.size > 0) {
        // Initializing code segment
        executable->ReadAt(
        &(kernel->machine->mainMemory[noffH.code.virtualAddr]), 
            noffH.code.size, noffH.code.inFileAddr);
    }

    if (noffH.initData.size > 0) {
        // Initializing data segment
        executable->ReadAt(
        &(kernel->machine->mainMemory[noffH.initData.virtualAddr]),
            noffH.initData.size, noffH.initData.inFileAddr);
    }

    #ifdef RDATA // read only case
    if (noffH.readonlyData.size > 0) {
        // Initializing read only data segment
        executable->ReadAt(
        &(kernel->machine->mainMemory[noffH.readonlyData.virtualAddr]),
            noffH.readonlyData.size, noffH.readonlyData.inFileAddr);
    }
    ```

6. `AddrSpace::Execute()` 會呼叫 `AddrSpace::InitRegisters()`,`AddrSpace::RestoreState()` 參考[AddrSpace::Execute()](#AddrSpace::Execute())
   1. `AddrSpace::InitRegisters()`:會初始化 register

    ```c++
    for (i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, 0);
    machine->WriteRegister(PCReg, 0);
    machine->WriteRegister(NextPCReg, 4);
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    ```

   2. `AddrSpace::RestoreState()`:復原 page table

    ```c++
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
    ```

7.  呼叫 `kernel->machine->Run();` 開始執行




### How does Nachos create and manage the page table?

1. `在 new thread` 後,我們要初始化 thread 的 AddrSpace() ,因為目前 virt page = phys page ,所以只要直接對應即可

```c++
    pageTable = new TranslationEntry[NumPhysPages];

    /* Initialize all the entries */
    for (int i = 0; i < NumPhysPages; i++) {
        // for now, virt page # = phys page #
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = i;
        // other bits that shows
        // some properties of entry
        pageTable[i].valid = TRUE;  // 是否在使用
        pageTable[i].use = FALSE;   // 是否被使用過
        pageTable[i].dirty = FALSE; // 對應的物理頁使用情況, true 表被寫過
        pageTable[i].readOnly = FALSE; // 是否只能讀
    }
```

2. 另外 manage 的部分透過 valid (是否在使用),use (是否被使用過),dirty (對應的物理頁使用情況),readOnly (是否只能讀) 來進行操控

3. 關於 manage 的詳細操作可以參考下一題


### How does Nachos translate addresses?

### How Nachos initializes the machine status (registers, etc) before running a thread (process)

1. 我們會透過 `AddrSpace()` 來初始化 Thread 的 space , 在 `AddrSpace()`時, `bzero()` 會將 `kernel->machine->mainMemory` 都設成 0
2. 當我們要讓 Thread 去執行,會呼叫 `fork()` 接著
   1. 呼叫 `StackAllocate(func, arg)` , 會初始化 Kernel Registers (machineState)

        ```c++
        machineState[PCState] = (void*) ThreadRoot;
        machineState[StartupPCState] = (void*) ThreadBegin;
        machineState[InitialPCState] = (void*) func;
        machineState[InitialArgState] = (void*) arg;
        machineState[WhenDonePCState] = (void*) ThreadFinish;
        ```

    2. 將 interrupt 關掉
    3. 調用 `ReadyToRun()` Thread 準備被執行
    4. 回復原本的 interrupt 狀態


### Which object in Nachos acts the role of process control block

`class Thread` 這個 object 相當於 Nachos 中的 process control block ,他的功能如下,

   1. 紀錄基本訊息: name,ID,status 等
   2. 紀錄相關空間:`machineState`,`userRegisters`,`Address Space`
   3. 對於 Thread 的各項操作:
      1. `Fork()`: Make thread run
      2. `Sleep()`: Put the thread to sleep and relinquish the processor
      3. `Finish()`: The thread is done executing
      4. `setStatus()`: 改變 thread 的狀態 ( JUST_CREATED, RUNNING, READY, BLOCKED, ZOMBIE )
      5. `Yield()`: Relinquish the CPU if any other thread is runnable

詳見[`Thread` 物件的構成](#thread-物件的構成)

### When and how does a thread get added into the ReadyToRun queue of Nachos CPU scheduler?

1. 一開始我們會執行 `main()` ,創建 kernel 並初始化他,並將我們要執行的 user program 存到 `Kernel::execfile[]`
2. 呼叫 `Kernel::ExecAll()` 會依照我們有幾個 user program 呼叫 `Kernel::Exec()`
   1. `Kernel::Exec()` 會創一個 thrad , 接著透過 `new AddrSpace()` 來初始化 thread 的 AddrSpace
   2. 最後呼叫 `fork()`, 他會先透過 `StackAllocate(func, arg)` , allocating stack memory , 並關閉 interrupts 接著呼叫 `scheduler->ReadyToRun(this)`, 其會將 thread 設為 `READY`, 並將此 thread 加到 `readyList` 中



### 總整理

`main` 抓指令 --> `ExecAll()` --> `Exec()` 產生thread 