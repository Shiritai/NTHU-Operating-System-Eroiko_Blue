<!-- ! 記得改 -> 號 →  -->
<!-- ! 超連結怪怪的要重新確認 -->


### 前言

此次 trace code 的部分, 像是統整過去所學到的各種知識, 所以大多數的 function 在過去的 MP1、MP2 中都有出現過, 也都有詳細解釋, 因此對於出現過的 function, 我們就不浪費篇幅詳細敘述, 而改以重點提點的方式進行, 如需要完整詳細解釋, 請參閱 MP1、MP2

### 1-1. New → Ready

#### function 功能重點回顧

##### `Kernel::ExecAll()`

1. 連續呼叫 `Kernel::Exec()`, 將方才搜集好的數個 user program 拿去執行
2. 所有 user program 都呼叫執行後, 呼叫 `Thread::Finish()` 方法, 結束當前執行緒

##### `Kernel::Exec(char*)`

1. 建立新執行緒
2. 為設定執行緒的 AddrSpace
3. 呼叫 `Thread::Fork()` 將執行緒 fork 出去

##### `Thread::Fork(VoidFunctionPtr, void*)`

1. 呼叫 `Thread::StackAllocate()` 來 Allocate Stack Memory
2. 呼叫 `Scheduler::ReadyToRun()` 來 schedule current thread using

##### `Thread::StackAllocate(VoidFunctionPtr, void*)`

1. 準備固定大小的 stack memory
2. 將 ThreadRoot 與其他 Routine 存入 Kernel Registers

##### `Scheduler::ReadyToRun(Thread*)`

1. 將執行緒設為準備狀態
2. 將本執行緒放入本 Scheduler 的成員: 待執行隊列 `Scheduler::readyList`

#### 流程目的與解釋

1. 產生新的 thread: 當我們傳入 user program 後, 會透過 `Kernel::ExecAll()`、`Kernel::Exec(char*)`, 來生成新的 thread
2. thread 由 `JUST_CREATED` →`READY`: 調用 `Thread::Fork()`, 待 `Thread::StackAllocate()` 完成, 呼叫 `Scheduler::ReadyToRun()` <font color=#cd5c5c>**將 thread 狀態改為由 `JUST_CREATED`→`READY`**</font>, 並加入到 Scheduler::readyList 中

### 1-2. Running → Ready

#### function 功能重點回顧

##### `Machine::Run()`

1. 模擬的主體, 呼叫 `OneInstruction()` 從 `registers[PCReg]` 讀取指令、 decode、 並執行
2. 呼叫 `OneTick()`

##### `Interrupt::OneTick()`

1. 更新 simulated time
2. 檢查待辦 interrupts
3. 如果 time device handler 要求 context switch, (`yieldOnReturn`==`TRUE`)就執行--呼叫 `kernel->currentThread->Yield()`

* 細節: `Timer::SetInterrupt()` 會在 `timer` 被建構後就被呼叫一次, 其功能是在 100ticks 後 schedule 1個 interrupt, 處理 interrupts 所呼叫的 `Timer::CallBack()` 會再次呼叫 `Timer::SetInterrupt()` 來達到每 100ticks 就中斷一次的功能, 另外他也呼叫 `Alarm::CallBack()`, 其呼叫 `Interrupt::YieldOnReturn()` 改變 `yieldOnReturn` 這個值

* 因為是有些部分第一次 trace, 故附上 code
* 
    ```c++
    Alarm::Alarm(bool doRandom)
    {
        timer = new Timer(doRandom, this);
    }

    Timer::Timer(bool doRandom, CallBackObj *toCall)
    {
        randomize = doRandom;
        callPeriodically = toCall;  // toCall 是 class ALarm
        disable = FALSE;
        SetInterrupt(); //  建構時就呼叫一次
    }

    void Timer::SetInterrupt() 
    {
        if (!disable) {
        int delay = TimerTicks; //  TimerTicks == int (100)
        
        if (randomize) {
            delay = 1 + (RandomNumber() % (TimerTicks * 2));
            }
        // schedule the next timer device interrupt
        kernel->interrupt->Schedule(this, delay, TimerInt); // 建1個 interrupt 在 100ticks 後
        }
    }

    void Timer::CallBack() 
    {
        // invoke the Nachos interrupt handler for this device
        callPeriodically->CallBack();
        
        SetInterrupt(); // do last, to let software interrupt handler
                        // decide if it wants to disable future interrupts
    }

    void Alarm::CallBack() 
    {
        Interrupt *interrupt = kernel->interrupt;
        MachineStatus status = interrupt->getStatus();

        interrupt->YieldOnReturn(); // original
    }

    void Interrupt::YieldOnReturn()
    { 
        ASSERT(inHandler == TRUE);  
        yieldOnReturn = TRUE; 
    }
    ```

##### `Thread::Yield()`

1. 為定義在 class Thread 的 function, 透過呼叫 `FindNextToRun();`、`ReadyToRun(this)`、`Run(nextThread, FALSE)`, 找next thread, 將原本的 thread 放到 ready list 的末端, 並將 cpu 分給 next thread
2. 上述提到的每個 function 等等複習到

* 首次 trace, 附上 code
  
  ```c++
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
  ```

##### `Scheduler::FindNextToRun()`

1. 確定當前並非中斷狀態後, 在本 `Scheduler` 的成員: 待執行隊列 `Scheduler::readyList` 中, 尋找下一個可執行的執行緒。有則回傳之, 無則回傳 `NULL`。

##### `Scheduler::ReadyToRun(Thread*)`

1. 首先確定當前並非中斷狀態, 再來將當前執行緒設為準備狀態, 最後將本執行續放入本 Scheduler 的成員: 待執行隊列

##### `Scheduler::Run(Thread*, bool)`

1. 執行新的執行緒 (本小節與此 function 並無太大關聯, 故不詳細介紹, 如要完整介紹, 請參照
[1-6. Ready→Running](#1-6-ready-→-running)

#### 流程目的與解釋

1. `Machine::Run()` 角度: `Machine::Run()` 會不斷的執行 instruction, 執行完後呼叫 `OneTick()`, 更新時間, 如果 `yieldOnReturn`==`TRUE` 呼叫 `kernel->currentThread->Yield()`, 其呼叫的 `Scheduler::ReadyToRun(Thread*)` 將正在執行的 <font color=#cd5c5c>**thread 從 `RUNNING`→`READY`**</font>, 並呼叫 `Scheduler::Run(Thread*, bool)` 去執行 `Scheduler::FindNextToRun()` 找到的 `nextThread` 
2. `Alarm::Alarm` 的角度: `Alarm::Alarm` 建構後就會 new 一個 `Timer::Timer`, 其建構好後會執行 `Timer::SetInterrupt()` 在 100ticks 後設一個 interrupts, 處理 interrupts 時會呼叫 `Timer::CallBack()` 其又呼叫`Timer::SetInterrupt()` (來達到 100ticks 就中斷一次的功能)並呼叫 `Alarm::CallBack()`, 其又呼叫 `Interrupt::YieldOnReturn()`, 其會將 `yieldOnReturn` 設為 `TRUE`, 讓 `Yield()` 順利被呼叫
3. 目的： 每 100ticks 會有一個 timer 設的 interrupts 強制目前執行的 thread 釋放 cpu, 並執行 next thread

### 1-3. Running → Waiting

#### function 功能重點回顧

* 本小節的 code 有些沒有 trace 過, 故附上 code

##### `SynchConsoleOutput::PutChar(char)`

```c++
    SynchConsoleOutput::SynchConsoleOutput(char *outputFile)
    {
        consoleOutput = new ConsoleOutput(outputFile, this);
        lock = new Lock("console out");
        waitFor = new Semaphore("console out", 0);
    }

    void SynchConsoleOutput::PutChar(char ch)
    {
        lock->Acquire();   
        consoleOutput->PutChar(ch);
        waitFor->P();
        lock->Release();
    }


    void Lock::Acquire()
    {
        semaphore->P();
        lockHolder = kernel->currentThread;
    }

    void Lock::Release()
    {
        ASSERT(IsHeldByCurrentThread());
        lockHolder = NULL;
        semaphore->V();
    }
```

1. 調用 `lock->Acquire()` 取得 lock,  來達到同時間只會有一個 writer, 拿到 lock 的 thread 才能執行 ( 最後會釋放 )
2. `PutChar(ch)` 打印的字輸出到模擬顯示器
3. 調用 `waitfor->p()`: 等待 semaphore value > 0 ( wait for callBack )
4. `lock->Release()`,　釋放 lock

* 不論 lock or waifor 都是為了避免同步問題
  1. lock: 目的是 only one writer at a time, 同時只能有一個 thread 用`SynchConsoleOutput`
  2. waitFor: 目的是 確保 putchar 已經做完, wait for callback
  3. 以上兩者皆是 (`class SynchConsoleOutput` 的 private 參數), 但目的有些微不同
  4. `lock->Acquire()`、`lock->Release()`、`semaphore->P()` 都會調用 `Semaphore::P()`, 稍後會再提到

* waitfor 細節補充
    1. waitfor 的目的是確保 putchar 已經做完, 才可以繼續往下一步進行, 可能是做下一次的 `consoleOutput->PutChar(ch)`, 這點從 `SynchConsoleOutput::PutInt(int value)` 可以明顯看出, 或者就直接歸還 lock
    2. waitfor 這個 `Semaphore` 訊號的初始值為0 (從`SynchConsoleOutput::SynchConsoleOutput(char *outputFile` 建構子可以看出)
    3. `PutChar(ch)` 後 `waitFor->P()` 開始等待 `SynchConsoleOutput::CallBack()` 中的 `waitFor->V()`, `PutChar(ch)` 會安排1個 interrupts, 之後會呼叫 `ConsoleOutput::CallBack()`, 其又呼叫 `SynchConsoleOutput::CallBack()`
    4. 待 callback fuction 都執行完就代表 `PutChar(ch)` 完成了!
    5. 然後看是要歸回 lock 或是 執行下一次的 `PutChar(ch)`

##### `Semaphore::P()`

* code 

```c++
void Semaphore::P()
{
    DEBUG(dbgTraCode, "In Semaphore::P(), " << kernel->stats->totalTicks);
    Interrupt *interrupt = kernel->interrupt;
    Thread *currentThread = kernel->currentThread;
    
    // disable interrupts
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
    while (value == 0) {        // semaphore not available
    queue->Append(currentThread);   // so go to sleep
    currentThread->Sleep(FALSE);
    } 
    value--;    // semaphore available, consume its value
   
    // re-enable interrupts
    (void) interrupt->SetLevel(oldLevel);
}
```

1. 將 interrupt disable
2. 如果 `value == 0` 就將 `currentThread` 放到 queue 的尾端
3. 呼叫 `currentThread->Sleep(FALSE)` (稍後會介紹)
4. semaphore available, consume its value
5. re-enable interrupts

* 相關連結: [`Semaphore::V()`](#semaphorev)

##### `List<T>::Append(T)`

* code
  
```c++
template <class T>
void List<T>::Append(T item)
{
    ListElement<T> *element = new ListElement<T>(item);

    ASSERT(!IsInList(item));
    if (IsEmpty()) {    // list is empty
        first = element;
        last = element;
    } 
    else {              // else put it after last
        last->next = element;
        last = element;
    }
    numInList++;
    ASSERT(IsInList(item));
}
```

1. `List<T>` 是 NachOS 定義並實作好的資料結構
2. `Semaphore::P()` 所呼叫的就是 `queue->Append(currentThread)`  將 `currentThread` 放到這個 `queue` 的尾端

##### `Thread::Sleep(bool)`

1. 確保呼叫此方法的合法性
2. 將當前執行緒的狀態設為 `BLOCKED`
3. 以 `Scheduler::FindNextToRun()` 尋找並呼叫下一個待執行的執行緒, 若下一個執行緒...
    * 存在, 則呼叫 `Scheduler::Run()` 方法, 完成本函數的邏輯:「終止當前執行緒, 並執行下一個執行緒」
    * 沒有, 則呼叫 `Interrupt::Idle()` 方法, 處理尚未完成的 interrupt, 抑或直接關機

##### `Scheduler::FindNextToRun()`

1. 確定當前並非中斷狀態後, 在本 `Scheduler` 的成員: 待執行隊列 `Scheduler::readyList` 中, 尋找下一個可執行的執行緒。有則回傳之, 無則回傳 `NULL`。

##### `Scheduler::Run(Thread*, bool)`

1. 執行新的執行緒 (本小節與此 function 並無太大關聯, 故不詳細介紹, 如要完整介紹, 請參照
[1-6. Ready→Running](#1-6-ready-→-running)

#### 流程目的與解釋

1. 不論是 lock or waiting 都有的 `Semaphore::P()`, 其通常代表資源的佔用與驗證, 如果 `value>0` 就代表資源可用, 於是將他減1, 讓`value==0`, 代表有人佔用, 如果有人再要求此資源時, `value==0` (已被占用), 就會將 `currentThread` 加入到 `Semaphore` 的成員 `queue` 中, 並呼叫 `currentThread->Sleep(FALSE)`, <font color=#cd5c5c>**其會將 `currentThread` 的狀態從 `RUNNING`→`BLOCKED`**</font>, 並找 nextthread 並執行他, 如果沒有 nextthread 就會進入 `Idle()` 去處理尚未完成的 interrupts
2. 總結來說, 假如要資源要不到 (要做I/O但別人在用, 我目前的 thread 就會<font color=#cd5c5c>**`RUNNING`→`BLOCKED`**</font>)

### 1-4. Waiting → Ready

#### function 功能重點回顧

##### `Semaphore::V()`

* code

```c++
void Semaphore::V()
{
    DEBUG(dbgTraCode, "In Semaphore::V(), " << kernel->stats->totalTicks);
    Interrupt *interrupt = kernel->interrupt;
    
    // disable interrupts
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
    if (!queue->IsEmpty()) {  // make thread ready.
    kernel->scheduler->ReadyToRun(queue->RemoveFront());
    }
    value++;
    
    // re-enable interrupts
    (void) interrupt->SetLevel(oldLevel);
}
```

1. 通常在 callback function 會呼叫此 function
2. disable interrupt
3. 將 `Semaphore::P()` 存到 `queue` 中 thread 透過 `kernel->scheduler->ReadyToRun(queue->RemoveFront())` 改變狀態為 `READY` 並放到執行隊列 `Scheduler::readyList`
4. value 會加回來
5. interrupt 的狀態也會改回來

* 相關連結: [`Semaphore::P()`](#semaphorep)

##### `Scheduler::ReadyToRun(Thread*)`

1. 將執行緒設為準備狀態
2. 將本執行緒放入本 Scheduler 的成員: 待執行隊列 `Scheduler::readyList`

#### 流程目的與解釋

1. 在歸還 lock ( ex:[`SynchConsoleOutput::PutChar(char)`](#synchconsoleoutputputcharchar) 的 `lock->Release()`) 或是許多 callback fuction 中 ( ex: SynchConsoleOutput::CallBack()), 都會有 `Semaphore::V()`, 通常代表著資源釋放 (與 `Semaphore::P()` 是相對的), 會將 `value+1` 代表其他人可以用這個資源了
2. 同時也會從 `Semaphore` 的成員 `queue` 取出一個因為需要此資源而被 `BLOCKED` 的 `thread`, <font color=#cd5c5c>**將狀態改變從 `BLOCKED`→`READY`**</font>, 並加入執行隊列 `Scheduler::readyList`

### 1-5. Running → Terminated

#### function 功能重點回顧

##### `ExceptionHandler(ExceptionType) case SC_Exit`

1. 從參數 `which` 得知哪種 exception (此例為 `SyscallException`), 從`Machine::RaiseException()`, 呼叫時傳入, 源自 `Machine::OneInstruction()`
2. 透過 `ReadRegister(2)`, 讀取到 register 2 的資料, 獲得哪種 type (此例為 `case SC_Exit`) 資料
3. 執行 `case SC_Exit`
    * code
  
   ```c++
    DEBUG(dbgAddr, "Program exit\n");
    val=kernel->machine->ReadRegister(4);
    cout << "return value:" << val << endl;
    kernel->currentThread->Finish();
    break;
   ```

    * 讀取資料後, 呼叫 `currentThread` 自身的方法 `kernel->currentThread->Finish()`
 
##### `Thread::Finish()`

1. 首先確保僅 `kernel->currentThread` 可以呼叫此方法, 並呼叫自身物件之 `Thread::Sleep()` 方法來完成執行緒的中止行為, 其傳入 `TRUE` 表示此執行緒已經結束, 請排程器此執行緒刪掉。如此一來 `Thread::Sleep()` 方法同時實作了正常的 sleep 邏輯和結束執行緒的邏輯。

##### `Thread::Sleep(bool)`

1. 確保呼叫此方法的合法性
2. 將當前執行緒的狀態設為 `BLOCKED`
3. 以 `Scheduler::FindNextToRun()` 尋找並呼叫下一個待執行的執行緒, 若下一個執行緒...
    * 存在, 則呼叫 `Scheduler::Run()` 方法, 完成本函數的邏輯:「終止當前執行緒, 並執行下一個執行緒」, 此時會傳一個 bool `finishing`, 其值 ==`TRUE`
    * 沒有, 則呼叫 `Interrupt::Idle()` 方法, 處理尚未完成的 interrupt, 抑或直接關機

##### `Scheduler::FindNextToRun()`

1. 將執行緒設為準備狀態
2. 將本執行緒放入本 Scheduler 的成員: 待執行隊列 `Scheduler::readyList`

##### `Scheduler::Run(Thread*, bool)`

1. `Context switch` 前若輸入的參數 `finishing` 為 `true`...
   *  表示切換到下一個執行緒後, 當前執行緒已經沒用了, 應該被刪除
   *  因此先儲存當前執行緒的指標至 `class Scheduler` 的成員 `Thread *toBeDestroyed`
2. 刪除 thread (兩種情況)
   1. 進行 `Context switch` 另一 thread 曾 `switch` 過:
      *  switch 過的 thread 會接著 `Scheduler::Run(Thread*, bool)` 中的 `SWICH()` 後執行, 因此執行 `Thread::CheckToBeDestroyed()` 將欲刪除的 thread 刪除
   2. 進行 `Context switch` 另一 thread 為新生: 
      * 當 thread 為新生要開始執型時, 會執行 `Thread::Begin ()`  其中會調用 `CheckToBeDestroyed()` 將要刪除的 thread 刪除
3. 注意! 原本的 code 並沒有將 thread 的狀態改為 `TERMINATED`, 此次實作有加上

#### 流程目的與解釋

1. 當 `Machine::Run()` 讀取到 user program 的最後時, 應該會讀到要 **Exit** 的 syscall, 其表示 userprogram 要結束了, 於是透過 `ExceptionHandler` 會調用 `currentThread` 自身的方法 `kernel->currentThread->Finish()`
2. `Finish()` 會呼叫 `sleep(TRUE)`, 其又會呼叫 `Run(nextThread, finishing)`
3. 在 `Scheduler::Run(Thread*, bool)` 中, 會將 `currentThread` 傳入 `toBeDestroyed`, 最終依不同情況調用 `Thread::CheckToBeDestroyed()` 刪除要刪的 `thread`, <font color=#cd5c5c>**要刪除的 thread 狀態 `RUNNIG`→`刪除` (原本並沒有會改成 `TERMINATED`, 實作 MP3 後才有)**</font>
4. 刪除 thread 的時機有二
   1. context switch 到 nextthread, nexthread 從 `SWITCH()` 往後執行
   2. 新的 thread 呼叫 `begin()` 時

### 1-6. Ready → Running

#### function 功能重點回顧

##### `Scheduler::FindNextToRun()`

1. 確定當前並非中斷狀態後, 在本 `Scheduler` 的成員: 待執行隊列 `Scheduler::readyList` 中, 尋找下一個可執行的執行緒。有則回傳之, 無則回傳 `NULL`。

##### `Scheduler::Run(Thread*, bool)`

* code

```cpp
void Scheduler::Run(Thread *nextThread, bool finishing) {
    // fetching current thread
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    // finishing == true:
    //      need to delete current thread
    if (finishing) {
        // assert the "toBeDestroyed" member
        //      doesn't hold any thread that
        //      should have been destroyed
        ASSERT(toBeDestroyed == NULL);
        toBeDestroyed = oldThread;
    }
    
    // if this thread is a user program
    if (oldThread->space != NULL) {
        // save the user's CPU registers and program block
        oldThread->SaveUserState();
        oldThread->space->SaveState();
    }

    // check if the old thread
    //      had an undetected stack overflow
    oldThread->CheckOverflow();

    // switch to the next thread
    kernel->currentThread = nextThread; 
    // nextThread is now running
    nextThread->setStatus(RUNNING);
    
    // context switch
    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
    //      interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    // check if thread we were running
    //      before this one has finished
    //      and needs to be cleaned up
    CheckToBeDestroyed();

    // if this thread is a user program
    if (oldThread->space != NULL) {
        // restore status of it
        oldThread->RestoreUserState();
        oldThread->space->RestoreState();
    }
}
```

執行新的執行緒, 我們需要進行 context switch, 執行完後也可能需要 context switch 回來。

以下簡述其流程。

1. Context switch 之前

   * 若輸入的參數 `finishing` 為 `true`...

     * 表示切換到下一個執行緒後, 當前執行緒已經沒用了, 應該被刪除

     * 因此先儲存當前執行緒的指標至 `Thread::toBeDestroyed`

   * 若當前執行緒為 user program, 則在 context switch 之前

     * 將 registers 的值寫入 `Thread` 物件 (thread control block) 的成員中

     * 將 Address Space 保留下來

     * 設定切換之執行緒的狀態為 `RUNNING`

2. Context switch

   * 切換執行緒

   * 呼叫 `SWITCH` 進行 context switch

   * 切換 thread

3. Context switch back

   * 如果切換到的 thread 之前有 switch 過, 則往下執行, 如果是完全新的 thread, 則會開始執行 thread (從 threadRoot 開始) 詳見 [`SWITCH`](#switchthread-thread)

   * 執行 `Thread::CheckToBeDestroyed()`

   * 若此執行緒為 user program, 則在 context switch 回來時

     * 將 `Thread` 的成員中取回之前執行緒執行時的 registers

     * 載入 Address Space

##### `SWITCH(Thread*, Thread*)`

* code

```python

        .text
        .align  2

        .globl  ThreadRoot
        .globl  _ThreadRoot

/* void ThreadRoot( void )
**
** expects the following registers to be initialized:
**      eax     points to startup function (interrupt enable)
**      edx     contains inital argument to thread function
**      esi     points to thread function
**      edi     point to Thread::Finish()
*/
_ThreadRoot:
ThreadRoot:
        pushl   %ebp
        movl    %esp,%ebp
        pushl   InitialArg
        call    *StartupPC
        call    *InitialPC
        call    *WhenDonePC

        # NOT REACHED
        movl    %ebp,%esp
        popl    %ebp
        ret



/* void SWITCH( thread *t1, thread *t2 )
**
** on entry, stack looks like this:
**      8(esp)  ->              thread *t2
**      4(esp)  ->              thread *t1
**       (esp)  ->              return address
**
** we push the current eax on the stack so that we can use it as
** a pointer to t1, this decrements esp by 4, so when we use it
** to reference stuff on the stack, we add 4 to the offset.
*/
        .comm   _eax_save,4 #用來對齊

        .globl  SWITCH
    .globl  _SWITCH
_SWITCH:    
SWITCH:                                 
        movl    %eax,_eax_save          # save the value of eax
        movl    4(%esp),%eax            # move pointer to t1 into eax
        movl    %ebx,_EBX(%eax)         # save registers
        movl    %ecx,_ECX(%eax)
        movl    %edx,_EDX(%eax)
        movl    %esi,_ESI(%eax)
        movl    %edi,_EDI(%eax)
        movl    %ebp,_EBP(%eax)
        movl    %esp,_ESP(%eax)         # save stack pointer
        movl    _eax_save,%ebx          # get the saved value of eax
        movl    %ebx,_EAX(%eax)         # store it
        movl    0(%esp),%ebx            # get return address from stack into ebx
        movl    %ebx,_PC(%eax)          # save it into the pc storage

        movl    8(%esp),%eax            # move pointer to t2 into eax

        movl    _EAX(%eax),%ebx         # get new value for eax into ebx
        movl    %ebx,_eax_save          # save it
        movl    _EBX(%eax),%ebx         # retore old registers
        movl    _ECX(%eax),%ecx
        movl    _EDX(%eax),%edx
        movl    _ESI(%eax),%esi
        movl    _EDI(%eax),%edi
        movl    _EBP(%eax),%ebp
        movl    _ESP(%eax),%esp         # restore stack pointer
        movl    _PC(%eax),%eax          # restore return address into eax
        movl    %eax,4(%esp)            # copy over the ret address on the stack
        movl    _eax_save,%eax

        ret

#endif // x86

```

1. 首先 AT&T 語法的組語, src在左, dst在右

2. 我們先回顧一下當我們新創一個 thread 時, 我們會呼叫 `Thread::StackAllocate (VoidFunctionPtr func, void *arg)`

    1. code
        ```c++
        void Thread::StackAllocate (VoidFunctionPtr func, void *arg)
        {
            stack = (int *) AllocBoundedArray(StackSize * sizeof(int));
            // the x86 passes the return address on the stack.  In order for SWITCH() 
            // to go to ThreadRoot when we switch to this thread, the return addres 
            // used in SWITCH() must be the starting address of ThreadRoot.
            stackTop = stack + StackSize - 4;   // -4 to be on the safe side!
            *(--stackTop) = (int) ThreadRoot;
            *stack = STACK_FENCEPOST;
            machineState[PCState] = (void*)ThreadRoot;
            machineState[StartupPCState] = (void*)ThreadBegin;
            machineState[InitialPCState] = (void*)func;
            machineState[InitialArgState] = (void*)arg;
            machineState[WhenDonePCState] = (void*)ThreadFinish;
        }
        ```

        所以我們知道...
        `machineState[7]` 儲存 `(void*)ThreadRoot`
        `machineState[2]` 儲存 `(void*)ThreadBegin`
        `machineState[5]` 儲存 `(void*)func`
        `machineState[3]` 儲存 `(void*)arg`
        `machineState[6]` 儲存 `(void*)ThreadFinish`



3. `SWITCH` 的內容

   1. 當我們呼叫 `SWITCH(oldThread, nextThread)`, 對應的 code 是...

        ```asm
            push Thread * nextThread
            push Thread * oldThread
            call SWITCH
        ```

   2. 所以我們的 stack 由上而下是 `Thread*nextThread` → `Thread*oldThread` → Return address ( esp: 指到這)

        ![stack](https://i.imgur.com/7F2fpKg.png)

   3. thread 中的成員對應到記憶體位址

        ```c++
        class Thread {
            private:
            int *stackTop;             // the current stack pointer
            void *machineState[MachineStateSize];  // all registers except for stackTop
        }
        ```

        所以...
        `Thread*` + 0 就會存取到 stackTop
        `Thread*` + 4 就會存取到 `machineState[0]`
        `Thread*` + 8 就會存取到 `machineState[1]`

   4. code 解釋 ( 請看註解 )

        ```python
        movl    %eax,_eax_save         
        # %eax 中資料存到 _eax_save
        
        movl    4(%esp),%eax            
        # %esp 中的值為 stack pointer , +4 後就會指到 old thread
        # 把 old thread 的地址存到 %eax 
        # %eax 為 old thread
        movl    %ebx,_EBX(%eax)    # 將 registers 資料存到 oldThread 的 machineState[1] 
        movl    %ecx,_ECX(%eax)    # 將 registers 資料存到 oldThread 的 machineState[2]
        movl    %edx,_EDX(%eax)    # 將 registers 資料存到 oldThread 的 machineState[3]
        movl    %esi,_ESI(%eax)    # 將 registers 資料存到 oldThread 的 machineState[5]
        movl    %edi,_EDI(%eax)    # 將 registers 資料存到 oldThread 的 machineState[6]
        movl    %ebp,_EBP(%eax)    # 將 registers 資料存到 oldThread 的 machineState[4]
        movl    %esp,_ESP(%eax)    # save stack pointer at oldThread 的成員 stackTop
        #define _ESP     0
        #define _EAX     4
        #define _EBX     8
        #define _ECX     12
        #define _EDX     16
        #define _EBP     20
        #define _ESI     24
        #define _EDI     28
        #define _PC      32

        movl    _eax_save,%ebx          # get the saved value of eax
        movl    %ebx,_EAX(%eax)         # store it in oldThread 的 machineState[0]
        movl    0(%esp),%ebx            # get return address from stack into ebx
        movl    %ebx,_PC(%eax)          # save it into the pc storage machineState[7]

        movl    8(%esp),%eax            # 讓%eax 的值指到 new thread

        movl    _EAX(%eax),%ebx         # get new value for eax into ebx
        movl    %ebx,_eax_save          # save it
        movl    _EBX(%eax),%ebx         # 將 new thread 中的資料放到對應的 registers 中
        movl    _ECX(%eax),%ecx         # 將 new thread 中的資料放到對應的 registers 中
        movl    _EDX(%eax),%edx         # 將 new thread 中的資料放到對應的 registers 中
        movl    _ESI(%eax),%esi         # 將 new thread 中的資料放到對應的 registers 中
        movl    _EDI(%eax),%edi         # 將 new thread 中的資料放到對應的 registers 中
        movl    _EBP(%eax),%ebp         # 將 new thread 中的資料放到對應的 registers 中
        movl    _ESP(%eax),%esp         # 從 stackTop 取得資料 restore stack pointer 
        
        movl    _PC(%eax),%eax          # restore return address into eax
        movl    %eax,4(%esp)            # copy over the ret address on the stack
        movl    _eax_save,%eax
        ret
        ```

   5. 執行 `SWITCH()` 就是將 registers 的資料存進 old thread, 將new thread 中的資料拿出來(因為要準備執行此 thread), 此時會發生兩種情況:

      1. nextThread 並未經歷過 `SWITCH（)`:
         * 因為是首此執行, 所以會執行 `stackTop` 中的 `ThreadRoot`, 進而開啟全新的 thread, 並且在 `ThreadRoot` 的上方放的是 `ret`, 待 `ThreadRoot` 完全執行完後才會執行到

      2. nextThread 經歷過 `SWITCH()`:
         * 我在前次的 `SWITCH()` 會因為 `movl %esp, _ESP(%eax)`, 而導致我的 `stackTop`, 被複寫成 `ret` 所以會執行 `SWITCH()` 下方的程式碼

4. `ThreadRoot`
   1. expects the following registers to be initialized:
      * eax points to startup function (interrupt enable)
      * edx contains inital argument to thread function
      * esi points to thread function
      * edi point to Thread::Finish()

   2. code

        ```python
        pushl   %ebp            
        movl    %esp,%ebp
        pushl   InitialArg      # arg
        call    *StartupPC      # 執行(void*)ThreadBegin
        call    *InitialPC      # (void*)func
        call    *WhenDonePC     # 執行(void*)ThreadFinish

        # NOT REACHED
        movl    %ebp,%esp
        popl    %ebp
        ret
        ```

    3. 執行 `ThreadRoot` 後會執行

       1. `ThreadBegin`:
           1. code:

                ```c
                static void ThreadBegin() { kernel->currentThread->Begin(); }
                ```

                ```c++
                void Thread::Begin ()
                {
                    ASSERT(this == kernel->currentThread);
                    DEBUG(dbgThread, "Beginning thread: " << name);
                    
                    kernel->scheduler->CheckToBeDestroyed();
                    kernel->interrupt->Enable();
                }
                ```

           2.  透過調用 `CheckToBeDestroyed()` 將要刪除的 thread 刪除 (可以乎應1-5), 同時 enable interrupts

       2. `(void*)func`: 其實也就是 `ForkExecute(Thread *t)`
          1. code:

            ```c++
            void ForkExecute(Thread *t) {
                // load executable and check if success or not
                if ( !t->space->Load(t->getName()) ) 
                    return;             // executable not found
                // success -> execute given thread
                t->space->Execute(t->getName());
            }
             ```
        
           2. 其中會呼叫 `AddrSpace::Load()`, 將執行檔載入記憶體, 並呼叫 `AddrSpace::Execute()`, 其中會初始化 registers, 並呼叫 `kernel->machine->Run()`, 開始執行

       3. `ThreadFinish()`:

           1. code:
            
                ```c
                static void ThreadFinish(){ kernel->currentThread->Finish(); }
                ```

                ```c++
                void Thread::Finish ()
                {
                    (void) kernel->interrupt->SetLevel(IntOff);
                    ASSERT(this == kernel->currentThread);
                    
                    DEBUG(dbgThread, "Finishing thread: " << name);
                    Sleep(TRUE);               // invokes SWITCH
                    // not reached
                }
                ```
            2. 當 thread 執行完後, 就會執行 `ThreadFinish()`, 其中會呼叫 `Thread::Finish ()`, 並進行一連串的操作[(可以參考1-5)](#1-5-running-→-terminated), 刪除此 thread

##### `for loop in Machine::Run()`

1. 模擬的主體, 不斷的呼叫 `OneInstruction()` 從 `registers[PCReg]` 讀取指令、 decode、 並執行、 呼叫 `OneTick()`

2. 在 loop 中執行此 thread

#### 流程目的與解釋

1. 透過 `Scheduler::FindNextToRun()` 找到待執行的執行緒, 並進入`Scheduler::Run(Thread*, bool)`
2. 在 `Scheduler::Run(Thread*, bool)` 中, <font color=#cd5c5c>**要執行的 thread 狀態 `READY`→`RUNNING`**</font>, 接著會進行 `SWITCH()`
3. 此時依是否有被 `SWITCH()` 分兩種情況
   1. 有：因為 `stackTop` 被覆寫, 因而執行上次 `SWITCH()` 後的程式碼
   2. 無：會呼叫 `ThreadRoot`, 並依序呼叫 `ThreadBegin`, `(void*)func`, `ThreadFinish()`
      1. `ThreadBegin`: 呼叫 `Thread::Begin()`, 刪除待刪除的 thread, enable interrupts
      2. `(void*)func` (`ForkExecute(Thread *t)`): 尋找檔案資料, 初始化, 並進入 `Machine::Run()` 的 loop 中執行
      3. `ThreadFinish()`: 當執行完 `(void*)func`, 代表此 thread 完成了, 呼叫`ThreadFinish()`, 其又呼叫 `kernel->currentThread->Finish()`, 結束此thread