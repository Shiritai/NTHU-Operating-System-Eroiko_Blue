# blue_trace

## halt

* 功能: 退出系统, 打印狀態

### $\tt{Machine::Run()}$

> in `machine/mipssim.cc`

* code

    ```cpp
    void
    Machine::Run()
    {
        Instruction *instr = new Instruction; // storage for decoded instruction
        if (debug->IsEnabled('m')) {
            cout << "Starting program in thread: " << kernel->currentThread->getName();
            cout << ", at time: " << kernel->stats->totalTicks << "\n";
        }
        kernel->interrupt->setStatus(UserMode); // 改變系統狀態為"UserMode"
        for (;;) {
            DEBUG(dbgTraCode, "In Machine::Run(), into OneInstruction " << "== Tick " << kernel->stats->totalTicks << " ==");
            
            OneInstruction(instr); // 執行instruction
                
            DEBUG(dbgTraCode, "In Machine::Run(), return from OneInstruction  " << "== Tick " << kernel->stats->totalTicks << " ==");
            DEBUG(dbgTraCode, "In Machine::Run(), into OneTick " << "== Tick " << kernel->stats->totalTicks << " ==");
                
            kernel->interrupt->OneTick(); // 將時間向前一單位
                
            DEBUG(dbgTraCode, "In Machine::Run(), return from OneTick " << "== Tick " << kernel->stats->totalTicks << " ==");
            if (singleStep && (runUntilTime <= kernel->stats->totalTicks))
                Debugger();
        }
    }
    ```
  
* `Machine::Run()` 功能: 在 NachOS 上, 模擬 **user level** 的運行, 當 program 開始運行, 就會 called by kernel。

* `Machine::Run()` 做了什麼:

    1. 首先會透過 `setStatus()` 將系統狀態改成user mode。
    2. 呼叫 `OneInstruction()` (稍後會詳細提到)
        1. 取出指令
        2. 對指令進行操作 (decode)
        3. 進行模擬執行
    3. 透過 `OneTick()` 時間前進一個單位
    4. 再回到 **2.** 因為是在無限迴圈中

* `void setStatus(MachineStatus st) { status = st; }`: 是定義在 `class Interrupt` 中的 function, 他可以改變系統的狀態
**{ idle , kernel , user }**

* `void Machine::OneInstruction(Instruction *instr)`: 是定義在 `class Machine` 的 function, 他會執行一個 user program 的 instruction。稍後會再次提到。

* `void Interrupt::OneTick()`: 是定義在`class Interrupt` 的 function。將時間前進一個單位, 並檢查有沒有 `pending interrupts` 待調用, 有則進行中斷處理。晚點會再次提到。(執行 instruction、interrupt 重啟都會調用此 function)

#### $\tt{Machine::OneInstruction()}$

> in `machine/mipssim.cc`

* code: 因版面關係, 先略

* `Machine::OneInstruction()` 功能: 執行 user的指令

* `Machine::OneInstruction()` 做了什麼:

    1. 獲得指令

        ```cpp
        if (!ReadMem(registers[PCReg], 4, &raw)) return; 
        // 讀到的值存在raw中
        // 如果有 exception  直接return
        instr->value = raw;
        ```

        1. 用 `ReadMem(int addr, int size, int *value)` 獲得 register 中的資料, 有exception `ReadMem()` 會回傳 false 之後直接 return。

    2. decode 指令

        ```cpp
        instr->Decode();
        ```

        1. 假設一切正常, 我們會從 `register[PCReg]` 得到資料, 之後用 `Decode()` 轉換他, 分離出 opcode, rs, rt, rd..., 不同 type 的指令分離出不同東西

    3. 執行指令

        ```cpp
        // 巨量的 case 故截取部分 
        switch (instr->opCode) {
           case OP_SYSCALL:
              DEBUG(dbgTraCode, "In Machine::OneInstruction, RaiseException(SyscallException, 0), " << kernel->stats->totalTicks);
              RaiseException(SyscallException, 0);
           return; 
        }
        ```

        1. 透過 switch 與大量的 case, decode 出來的 opcode 可以找到對應要做的事情
        2. 以上面的 code 為例, decode 出來的opCode 是 **OP_SYSCALL** 的類型, 就會呼叫 `RaiseException(SyscallException, 0)`, 詳細過程請看下一點

    4. 呼叫 `DelayedLoad(nextLoadReg, nextLoadValue)` 進行延遲加載

    5. 更新 program counter

    ```cpp
    registers[PrevPCReg] = registers[PCReg]; // for debugging
    registers[PCReg] = registers[NextPCReg];
    registers[NextPCReg] = pcAfter;
    ```

* `ReadMem(int addr, int size, int *value)`: 是定義在 `translate.cc` 的 function, 可以讀取 addr(virtual address) 的資料。 其中的`Translate()` 可以將virtual address translate into physical address (用 page table、TLB)

* `Decode()`: 是定義在 `class Instruction` 中的 function, 透過 shift, and 分離出 instruction 的資料,包含何種 type formate,opcode, rs, rt, rd...等資料 (不同 type 會得到的東西也不同)

* `void Machine::DelayedLoad(int nextReg, int nextValue)`: 是定義在 `mipssim.cc` 的 function, 會將上次 delay 的資料存入該存的地方, 並將參數的資料存入 delay 相關的 register

#### $\tt{Machine::RaiseException()}$

> in `machine/machine.cc`

* code:

    ```cpp
    
    void Machine::RaiseException(ExceptionType which, int badVAddr)   //which 是出錯的類型 badVAddr是出錯的地址
    {
        DEBUG(dbgMach, "Exception: " << exceptionNames[which]);
        
        registers[BadVAddrReg] = badVAddr; //將出錯的地址存到registers[BadVAddrReg]
        DelayedLoad(0, 0);  // finish anything in progress
        kernel->interrupt->setStatus(SystemMode);     //改變mode
        ExceptionHandler(which);  // interrupts are enabled at this point
        kernel->interrupt->setStatus(UserMode);       //改變mode
    }
    ```

* `Machine::RaiseException()` 的功能: 呼叫`ExceptionHandler()` 來處理 exception, 包含**system call** 或是其他錯誤, 如錯誤的地址等

* `Machine::RaiseException()`做了什麼

  1. 參數:
     1. which = 出錯的類型, 源自於`Machine::OneInstruction()` 在不同情況會填入不同的 exception 類型 ex.(SyscallException)
     2. badVAddr = 出錯的地址

  2. 將錯誤地址存到 registers[BadVAddrReg] 中

  3. 使用 `DelayedLoad(0, 0)` 結束當前運行的東西, 並在進入 system mode 前,將還沒 Load 的東西 Load 好 (輸入 0, 0 對往後不影響, 因為register 0 只會是 0)

  4. 狀態從 user mode 變為 system mode

  5. 呼叫 `ExceptionHandler(which)`, 處理exception, 稍後會提到

  6. 將狀態改回 user mode

#### $\tt{ExceptionHandler()}$

> in `userprog/exception.cc`

* code
  
    ``` cpp
    // 由於case多,只擷取與halt 有關的
    void ExceptionHandler(ExceptionType which)
    {
        char ch;
        int val;
        int type = kernel->machine->ReadRegister(2); // 從register 2 讀取資料 什摩type 的 exception ex.SC_Halt 
        int status, exit, threadID, programID, fileID, numChar;
        DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");
        DEBUG(dbgTraCode, "In ExceptionHandler(), Received Exception " << which << " type: " << type << ", " << kernel->stats->totalTicks);
        switch (which) {  //從Machine::RaiseException()丟進來哪種exception 和前面type不同的是 這裡是指 SyscallException 還是OverflowException 的那種不同  
            case SyscallException:
                switch(type) {    //是哪種type 的syscall exception 
                case SC_Halt:
                   DEBUG(dbgSys, "Shutdown, initiated by user program.\n");
                   SysHalt();
                   cout<<"in exception\n";
                   ASSERTNOTREACHED(); // 照理說不應該執行
                   break;
                }
    ```

* `ExceptionHandler()` 的功能: 根據不同exception, 進行不同行為, 如這次要 trace `的就是SC_Halt` 是怎麼處理的

* `ExceptionHandler()` 做了什麼:

  1. 這次 trace SC_Halt 會用到的參數
     1. which: 由 `Machine::RaiseException()`, 呼叫時傳入, 源自 `Machine::OneInstruction()`

     2. type: 為透過`ReadRegister(2)`, 讀取到register 2 的資料

  2. 透過 `ReadRegister(2)` 取得 exception 的 type

  3. 透過 `switch case` 找到對應處理行為, 以這部分為例, `which = SyscallException`, `type = SC_Halt`, 將執行 `SysHalt()`

* `int Machine::ReadRegister(int num)`: 是 `class machine` 中定義的 function, 會回傳 `registers[num]` 中的資料

* `SysHalt()`: 是在 `ksyscall.h` 中定義的function, 他會呼叫 `kernel->interrupt->Halt()` 等等就會講到

#### $\tt{SysHalt()}$

> in `ksyscall.h`

* code

    ```cpp
    void SysHalt()
    {
       kernel->interrupt->Halt(); //呼叫在
    }
    ```

* `SysHalt()` 的功用: 呼叫`kernel->interrupt->Halt()`, 為了達到 `SC_Halt` 的目的

* `SysHalt()`做了什麼: 呼叫`kernel->interrupt->Halt()`

#### $\tt{Interrupt::Halt()}$

> in `machine/interrupt.cc`

* code

    ```cpp
    void Interrupt::Halt()
    {
        cout << "Machine halting!\n\n";
        cout << "This is halt\n";
        kernel->stats->Print();
        delete kernel;// Never returns.刪掉kernel所有東西就沒了
    }
    ```

* `Interrupt::Halt()` 的功用: 打印出參數,並結束系統

* `Interrupt::Halt()` 做了什麼:
  
  1. 透過 `kernel->stats->Print()` 打印參數

  2. 刪除 kernel, 也就結束整個系統了

* `kernel->stats->Print()`: 是定義在 `class Statistic` 的 function, 他會打印出許多數據。

    ```cpp
    void Statistics::Print()
    {
        cout << "Ticks: total " << totalTicks << ", idle " << idleTicks;
        cout << ", system " << systemTicks << ", user " << userTicks <<"\n";
        cout << "Disk I/O: reads " << numDiskReads;
        cout << ", writes " << numDiskWrites << "\n";
        cout << "Console I/O: reads " << numConsoleCharsRead;
        cout << ", writes " << numConsoleCharsWritten << "\n";
        cout << "Paging: faults " << numPageFaults << "\n";
        cout << "Network I/O: packets received " << numPacketsRecvd;
        cout << ", sent " << numPacketsSent << "\n";
    }
    ```

### `SC_Halt()` 小結

* `Machine::Run()` 是模擬的主體, called by kernel, 會將狀態改為 user mode, 透過`Machine::OneInstruction()` 不斷讀取指令、執行指令

* 當 `Machine::OneInstruction()` 接收到 `OP_SYSCALL` 的指令時, 會 call `Machine::RaiseException(SyscallException, 0)`, exception 的種類為 **SyscallException**
  
* `Machine::RaiseException(which,badVAddr)` 會改變狀態為 SystemMode, 呼叫 `ExceptionHandler(which)` 處理 exception, `ExceptionHandler()` 會從 `Machine::RaiseException()` 知道 exception 的類型 (**SyscallException**), 並從register 2 得到他的 type (**SC_Halt**), 並呼叫 `SysHalt()`

* `SysHalt()` 又會呼叫 `kernel->stats->Halt()`, 刪除 kernel, 結束系統

#### $\tt{ExceptionHandler()}$

>in `userprog/exception.cc`

* 先前在 `SC_Halt()` 已有介紹過, 故重點放在不同的地方

* code

```cpp
case SC_PrintInt:
    DEBUG(dbgSys, "Print Int\n");
    val=kernel->machine->ReadRegister(4); //從register4讀取要print的資料
    DEBUG(dbgTraCode, "In ExceptionHandler(), into SysPrintInt, " << kernel->stats->totalTicks);    
    SysPrintInt(val);                   //請看下一點
    DEBUG(dbgTraCode, "In ExceptionHandler(), return from SysPrintInt, " << kernel->stats->totalTicks);
    // 更新 Program Counter
    
    kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg)); // 將PCReg 的資料寫到PrevPCReg
    kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);// 將PCReg +4的資料寫到PCReg
    kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4); //將(舊的PCReg +4)+4 存到NextPCReg
    return;
    ASSERTNOTREACHED();
    break;
```

* `ExceptionHandler()` 的功能: 根據不同exception, 進行不同行為, 如這次要 trace 的就是 `SC_PrintInt` 是怎麼處理的

* `ExceptionHandler()` 做了什麼:

   1. 用 `ReadRegister(4)` 從 register 4 讀取要 print 的資料

   2. 執行 `SysPrintInt(val)`, 稍後會講解

   3. 更新 PCReg。PCReg 的資料寫到 PrevPCReg; PCReg + 4 的資料寫到 PCReg; 將 (舊的 PCReg  + 4) + 4 存到 NextPCReg

* `void Machine::WriteRegister(int num, int value)`: 為定義在 `machine.cc` 的 function, 可以將 value 寫進 `register[num]` 中

#### $\tt{SysPrintInt()}$

> in `userprog/ksyscall.h`

* code

    ```cpp
    void SysPrintInt(int val)
    { 
        DEBUG(dbgTraCode, "In ksyscall.h:SysPrintInt, into synchConsoleOut->PutInt, " << kernel->stats->totalTicks);
        kernel->synchConsoleOut->PutInt(val);
        DEBUG(dbgTraCode, "In ksyscall.h:SysPrintInt, return from synchConsoleOut->PutInt, " << kernel->stats->totalTicks);
    }
    ```

* `SysPrintInt()` 的功能: 調用`kernel->synchConsoleOut->PutInt(val);` 稍後介紹

* `SysPrintInt(int val)`的參數:

   1. val: 由 `ExceptionHandler()` 調用此function 時傳入

#### $\tt{SynchConsoleOutput::PutInt()}$

> in `userprog/syschconsole`

* code
  
    ```cpp
    void SynchConsoleOutput::PutInt(int value)
    {
        char str[15];
        int idx=0;
        //sprintf(str, "%d\n\0", value);  the true one
        sprintf(str, "%d\n\0", value); //將value存到str,轉成文字型態
        
        lock->Acquire(); //取得lock
        do{
        DEBUG(dbgTraCode, "In SynchConsoleOutput::PutChar, into consoleOutput->PutChar, " << kernel->stats->totalTicks);
        
        consoleOutput->PutChar(str[idx]); //輸出
        DEBUG(dbgTraCode, "In SynchConsoleOutput::PutChar, return from consoleOutput->PutChar, " << kernel->stats->totalTicks);
        
        idx++;
    
        DEBUG(dbgTraCode, "In SynchConsoleOutput::PutChar, into waitFor->P(), " << kernel->stats->totalTicks);
        
        waitFor->P(); //等待
    
        DEBUG(dbgTraCode, "In SynchConsoleOutput::PutChar, return form  waitFor->P(), " << kernel->stats->totalTicks);
        } while (str[idx] != '\0');
        
        lock->Release();//釋放lock
    }
    ```

* `SynchConsoleOutput::PutInt()` 的功能:

* `SynchConsoleOutput::PutInt()` 做了什麼:

  1. 透過 cpp 的 function `sprintf` 將 value 從 int 轉換為 char array 並加上了 `\n\0` 存進 `str`

  2. 用 `lock->Acquire()` 來達到同時間只會有一個 writer, 拿到 lock 的 thread 才能執行

  3. 調用 `consoleOutput->PutChar(str[idx])`, 將要打印的字輸出到模擬顯示器, 細節稍後會再介紹

  4. 調用`waitfor->p()`: 等待 semaphore value > 0

  5. `lock->Release()` 將 lock 釋放出來

* `lock->Acquire(),lock->Release()`: 是定義在`class Lock` 的 function, 功能是為了避免多執行緒所衍生的錯誤 (ex. 多執行緒同時存取、修改某資料)

* `consoleOutput->PutChar(str[idx])`: 定義在`class ConsoleOutput` 的 function, 能將字符輸出到模擬顯示器, 細節稍後會提到

*`waitFor->P()`: 定義在 `class Semaphore`下的function, 會讓 thread sleep, 禁止進行其他 thread, 直到獲得 call back

#### $\tt{SynchConsoleOutput::PutChar()}$

> in `userprog/syschconsole`

* code

    ```cpp
    void SynchConsoleOutput::PutChar(char ch)
    {
        lock->Acquire();
        consoleOutput->PutChar(ch);
        waitFor->P();
        lock->Release();
    }
    ```

* 基本上和 `SynchConsoleOutput::PutInt(int value)` 一樣, 差在一個需要先將 `int` 轉成 char array, 而現在要輸出的直接就是 `char`, 其他原理操作都一樣

#### $\tt{ConsoleOutput::PutChar()}$

>in `machine/console.cc`

* code

    ```cpp
    void ConsoleOutput::PutChar(char ch)
    {
        ASSERT(putBusy == FALSE); //判斷putBusy == FALSE flase 則abort
        WriteFile(writeFileNo, &ch, sizeof(char));//將ch寫到writeFileNo
        putBusy = TRUE;      //
        kernel->interrupt->Schedule(this, ConsoleTime, ConsoleWriteInt);//安排interrupt
    }
    ```

* `ConsoleOutput::PutChar(char ch)` 的功能: 將 `ch` 寫到 simulated display, 並安排 interrupt

* `ConsoleOutput::PutChar(char ch)` 做了什麼:

  1. 判斷 `putBusy == FALSE` false 則 abort

  2. 調用 `WriteFile()` 將要輸出的 char 寫到`writeFileNo` (他是模擬顯示的 UNIX 文件)

  3. 調用 `Schedule()` 安排 interrupt

`WriteFile()`: 定義在 `sysdep.cc` 的 function, 他可以將 characters 寫到 open file

`Schedule()`: 是定義在 `class Interrupt` 下的function, 細節稍後會介紹

#### $\tt{Interrupt::Schedule()}$

>in`machine/interrupt.cc`

* code

    ```cpp
    void Interrupt::Schedule(CallBackObj *toCall, int fromNow, IntType type)
    {
        int when = kernel->stats->totalTicks + fromNow; //現在時間+多久後
        PendingInterrupt *toOccur = new PendingInterrupt(toCall, when, type);
    
        DEBUG(dbgInt, "Scheduling interrupt handler the " << intTypeNames[type] << " at time = " << when);
        ASSERT(fromNow > 0);
    
        pending->Insert(toOccur);
    }
    ```

* `Interrupt::Schedule()` 的功能:
  
* `Interrupt::Schedule()` 做了什麼:

    1. 參數:

        1. `toCall`: interrupt 發生時要 Call 的 object

        2. `fromNow`: 多久後 interrupt 發生 (in simulated time)

        3. `type`: 產生 interrupt 的硬體類型

    2. 什麼時候要 interrupt = `totalTicks` (現在) + `fromNow`

    3. `new` 一個 `PendingInterrupt(toCall, when, type)` 存 interrupt 的相關資訊到 `toOccur`

    4. 調用 `pending->Insert(toOccur)`, 將剛剛紀錄的 interrupt 加到 `pending` 中

* `pending` 是 `class Interrupt` 的參數, 儲存 interrupt 的 list

#### $\tt{Machine::Run()}$

* 略, 過去已介紹過

#### $\tt{Interrupt::OneTick()}$

> in `machine/interrupt.cc`

* code

    ```cpp
    void
    Interrupt::OneTick()
    {
       MachineStatus oldStatus = status;
       Statistics *stats = kernel->stats;
    
       // advance simulated time
        if (status == SystemMode) {
          stats->totalTicks += SystemTick;//SystemTick=10
          stats->systemTicks += SystemTick;
        } else {
          stats->totalTicks += UserTick; //UserTick=1
          stats->userTicks += UserTick;
        }
        DEBUG(dbgInt, "== Tick " << stats->totalTicks << " ==");
    
    // check any pending interrupts are now ready to fire
        ChangeLevel(IntOn, IntOff);  // first, turn off interrupts
       // (interrupt handlers run with
       // interrupts disabled)
        CheckIfDue(FALSE);  // check for pending interrupts
        ChangeLevel(IntOff, IntOn);  // re-enable interrupts
        if (yieldOnReturn) {   // if the timer device handler asked 
                               // for a context switch, ok to do it now
       yieldOnReturn = FALSE;
       status = SystemMode;  // yield is a kernel routine
       kernel->currentThread->Yield();
       status = oldStatus;
        }
    }
    ```

* `Interrupt::OneTick()`功能: 更新simulated time,檢查有沒有待辦的interrupts要執行

* `Interrupt::OneTick()`做了什麼:

   1. 根據不同的status更新simulated time,SystemMode:+10,other +1

   2. 檢查待辦interrupt:

      1. 調用 `ChangeLevel()` 改變 interrupt 狀態 on -> off

      2. 調用 `CheckIfDue()` 檢查待辦 interrupt

      3. 調用 `ChangeLevel()` 改變 interrupt 狀態 off -> on

   3. 如果 timer device handler 要求 context switch 就執行

* `ChangeLevel()`: 定義在 `class Interrupt` 的 function, 改變 interrupts 狀態

* `CheckIfDue()`: 定義在 `class Interrupt` 的function, 稍後會再提到

* `kernel->currentThread->Yield()`: 定義在`class Thread` 的 function, 將 thread 放到ready list 的末端, 將 cpu 分給 next thread

#### $\tt{Interrupt::CheckIfDue()}$

> in `machine/interrupt.cc`

* code

    ```cpp
    bool Interrupt::CheckIfDue(bool advanceClock)
    {
        PendingInterrupt *next;
        Statistics *stats = kernel->stats;
    
        ASSERT(level == IntOff);  // interrupts need to be disabled,
                                  // to invoke an interrupt handler
        if (debug->IsEnabled(dbgInt)) {
            DumpState();
        }
        if (pending->IsEmpty()) { // no pending interrupts 判斷是否有待辦 interrupt
            return FALSE;
        }
        next = pending->Front();   //取出最前面的待辦interrupt
        if (next->when > stats->totalTicks) {
            if (!advanceClock) {     // not time yet
                return FALSE;
            }
            else {   // advance the clock to next interrupt
                stats->idleTicks += (next->when - stats->totalTicks);
                stats->totalTicks = next->when;
                // UDelay(1000L); // rcgood - to stop nachos from spinning.
            }
        }
    
        DEBUG(dbgInt, "Invoking interrupt handler for the ");
        DEBUG(dbgInt, intTypeNames[next->type] << " at time " << next->when);
    
        if (kernel->machine != NULL) {
            kernel->machine->DelayedLoad(0, 0);
        }
       
        inHandler = TRUE;  //如果我們在運行interrupt handler 會打開
        do {
           next = pending->RemoveFront();    // pull interrupt off list
           DEBUG(dbgTraCode, "In Interrupt::CheckIfDue, into callOnInterrupt->CallBack, " << stats->totalTicks);
           
           next->callOnInterrupt->CallBack();// call the interrupt handler
    
           DEBUG(dbgTraCode, "In Interrupt::CheckIfDue, return from callOnInterrupt->CallBack, " << stats->totalTicks);
           delete next;         //釋放interrupt 佔用的記憶體
        } while (!pending->IsEmpty() && (pending->Front()->when <= stats->totalTicks));
        inHandler = FALSE;
        return TRUE;
    }
    ```

* `Interrupt::CheckIfDue()` 的功能: 檢查現在有沒有 interrupt, 並根據不同情況進行處理

* `Interrupt::CheckIfDue()` 做了什麼:

   1. 判斷是否有待辦 interrupt

   2. 取出最前面的待辦 interrupt

   3. 如果時間還沒到:

      1. 如果沒設置 `advanceClock`, 則 return (OneTick: false)

      2. 如果有設置 `advanceClock`, 則將時間跳到, interrupt 要發生的時間 (Idle: true)

   4. 使用 `DelayedLoad(0, 0)` 結束當前運行的東西, 將還沒 Load 的東西 Load 好 (輸入0, 0 對往後不影響, 因為register 0 只會是 0)

   5. 中斷發生

      1. inHandler 設定成 True, 代表我正在運行interrupt handler

      2. 進行中斷處理, 調用 `callback()`, 如果還有待辦的 interrupt 且待辦的時間 <= 現在, 則會繼續進行新的一輪中斷處理

      3. inHandler 設定成 False

      4. return

* `callback()`: 是定義在 `class CallBackObj` 下的函數, 透過 c++ virtual functions 實現, 他會調用我們當初設定的 `CallBackObj` class 的 `CallBack()`, 以目前為例, 我們在 `ConsoleOutput::PutChar()` 中會調用`kernel->interrupt->Schedule(this, ConsoleTime, ConsoleWriteInt)`, 其中參數 this 指的是 `class ConsoleOutput`, 隨著 function 的調用, 他會被包含在 interrupt 中, 所以在進行中段處理時就會調用 `ConsoleOutput::CallBack()`

#### $\tt{ConsoleOutput::CallBack()}$

> in `machine/console.cc`

* code

    ```cpp
    void ConsoleOutput::CallBack()
    {
        DEBUG(dbgTraCode, "In ConsoleOutput::CallBack(), " << kernel->stats->totalTicks);
        putBusy = FALSE; //將putBusy改回來
        kernel->stats->numConsoleCharsWritten++;//紀錄有多少字符被顯示
        callWhenDone->CallBack();
    }
    ```

* `ConsoleOutput::CallBack()` 的功能: 改變一些變數, 代表可以去寫下一個 character, 並再次呼叫 `CallBack()`

* `ConsoleOutput::CallBack()` 做了什麼:

   1. `putBusy` 回到 false

   2. `numConsoleCharsWritten++`

   3. 調用 `SynchConsoleOutput::CallBack()`

#### $\tt{SynchConsoleOutput::CallBack()}$

> in `userprog/synchonsole`

* code

    ```cpp
    void SynchConsoleOutput::CallBack()
    {
        DEBUG(dbgTraCode, "In SynchConsoleOutput::CallBack(), " << kernel->stats->totalTicks);
        waitFor->V();
    }
    ```

* `SynchConsoleOutput::CallBack()` 的功能: 呼叫 `V()`, 回傳訊號, 表示可以執行其他 thread

* `SynchConsoleOutput::CallBack()` 做了什麼:

  1. 呼叫 `waitFor->V()`

* `V()`: 是定義在 `class Semaphore` 的function, 如果必要會喚醒在 `P()` 等待的 thread

### `SC_PrintInt` 小結

* 