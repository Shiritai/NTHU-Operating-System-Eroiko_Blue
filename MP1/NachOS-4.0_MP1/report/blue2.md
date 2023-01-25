### `SC_PrintInt` 小結

* 當我們要執行一個 SC_Print, 首先產生 exception , 接著會進入到`ExceptionHandler()`, 從 register 4 取得要打印的資料 `val`, 接著呼叫 `SysPrintInt(val)`

* `SysPrintInt(val)` 再呼叫 `SynchConsoleOutput::PutInt()` ,會將  `val` 改成 `char` array , 取得 lock 後呼叫`ConsoleOutput::PutChar()` 打印一個字符, `waitFor->P()` 等待 `SynchConsoleOutput::CallBack()` 中的 `waitFor->V()` 來進行下一個thread

* `ConsoleOutput::PutChar(char ch)` 打印出字符, 並將參數 `putBusy = TRUE` (待稍後 `ConsoleOutput::CallBack()` 才會將他改回) 並呼叫`Interrupt::Schedule()` 安排一個 interrupt

* 在 `Machine::Run()` 執行每個指令後他會呼叫 `Interrupt::OneTick()` 進而更新新的 `Ticks` , 並檢查待辦的 interrupt , 並呼叫 `Interrupt::CheckIfDue()` 進行處理

* `Interrupt::CheckIfDue()` 中斷的處理方式就是 呼叫 `ConsoleOutput::CallBack()`將剛剛提到的 `putBusy` 改回 FALSE 並再呼叫 `SynchConsoleOutput::CallBack()` 執行 `waitFor->V()` 表示可以執行其他 thread 也可以開始新的一輪的打印字符,將所有要打印的字符打印完會執行 `lock->Release()` 歸還lock 結束 `SynchConsoleOutput::PutInt()`

* 最後回到 `ExceptionHandler()` 更新 PCReg

* syscall $\rightarrow$ `ExceptionHandler()` $\rightarrow$ `SysPrintInt(val)` $\rightarrow$ `SynchConsoleOutput::PutInt()` $\rightarrow$ `ConsoleOutput::PutChar()` $\rightarrow$`Interrupt::Schedule()` 

* `Machine::Run()` $\rightarrow$ `Interrupt::OneTick()` $\rightarrow$ `Interrupt::CheckIfDue()` $\rightarrow$ `ConsoleOutput::CallBack()` $\rightarrow$ `SynchConsoleOutput::CallBack()` $\rightarrow$ `SynchConsoleOutput::PutInt()`


在過去的程式經驗中，我寫過的 code 都是從 0 開始，很少有 trace code 的經驗。這次的作業是我第一次看到這麼大量的 code 雖然第一眼看到它們的時候我感到十分的害怕，但經過長時間的 trace 後，發現它們之間都是環環相扣、有跡可循的，(雖然因為不知道該 trace 到多深，導致篇幅略長)，我開始了解他們的前後關係，加上連結到上課的知識，一切都顯得豁然開朗，漸漸開始了解 syscall 在 NachOS 的運作原理。
另外在撰寫報告方面，感謝楊子慶教我使用 Markdown 讓我可以寫出比平常更好看的報告。

