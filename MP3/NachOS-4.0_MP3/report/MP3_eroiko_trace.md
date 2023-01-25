# Tiny Note

> define `machineStatus` as `m`

```c++
class Thread {
private:
    int *stackTop;           // the current stack pointer
    void *machineState[MachineStateSize];  // all registers except for stackTop
}
```

|位移|位移量|對應 `Thread` 的成員|陣列位移量|
|:-:|:-:|:-:|:-:|
|`_ESP`|0 |`stackTop`|-|
|`_EAX`|4 |`m[0]`  |0|
|`_EBX`|8 |`m[1]`  |1|
|`_ECX`|12|`m[StartupState]`|2|
|`_EDX`|16|`m[InitialArgState]`|3|
|`_EBP`|20|`m[FPState]`|4|
|`_ESI`|24|`m[InitialPCState]`|5|
|`_EDI`|28|`m[WhenDonePCState]`|6|
|`_PC` |32|`m[PCState]`|7|

```c
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
```

```C#
        .comm   _eax_save,4

        .globl  SWITCH
    .globl  _SWITCH
_SWITCH:
SWITCH:
    movl    %eax,_eax_save      # 暫存 eax 的值
    movl    4(%esp),%eax        # eax 存 t1 的指標, 用於接下來
                                # 儲存 t1 之暫存器的資料
    # 儲存 t1 之暫存器的資料
    movl    %ebx,_EBX(%eax)     # 存入 ebx 到 m[1]
    movl    %ecx,_ECX(%eax)     # 存入 ecx 到 m[2]
    movl    %edx,_EDX(%eax)     # 存入 edx 到 m[3]
    movl    %esi,_ESI(%eax)     # 存入 esi 到 m[5]
    movl    %edi,_EDI(%eax)     # 存入 edi 到 m[6]
    movl    %ebp,_EBP(%eax)     # 存入 ebp 到 m[4]
    movl    %esp,_ESP(%eax)     # 存入 esp 到 stackTop
    movl    _eax_save,%ebx      # 取回暫存的 eax 值至 ebx
    movl    %ebx,_EAX(%eax)     # 存入 ebx 到 m[0]
    movl    0(%esp),%ebx        # 將 RA 暫存入 ebx
    movl    %ebx,_PC(%eax)      # 存入 ebx (RA) 到 m[7]
    # 至此完成將所有 t1 的暫存器資料存入 t1 的前兩位成員中

    movl    8(%esp),%eax        # eax 存 t2 的指標, 用於接下來
                                # 載入 t2 存在成員中的暫存器資料

    # 載入 t2 存在成員中的暫存器資料
    movl    _EAX(%eax),%ebx     # 把 m[0] 資料存入 ebx (應該存入 eax, 
                                # 不過現在 eax 存 t2 指標, 有其他用途)
    movl    %ebx,_eax_save      # 暫存最後要放入 eax 的資料
    movl    _EBX(%eax),%ebx     # 載回 m[1] 到 ebx
    movl    _ECX(%eax),%ecx     # 載回 m[2] 到 ecx
    movl    _EDX(%eax),%edx     # 載回 m[3] 到 edx
    movl    _ESI(%eax),%esi     # 載回 m[5] 到 esi
    movl    _EDI(%eax),%edi     # 載回 m[6] 到 edi
    movl    _EBP(%eax),%ebp     # 載回 m[4] 到 ebp
    movl    _ESP(%eax),%esp     # 載回 stackTop 到 esp
    movl    _PC(%eax),%eax      # 載回 RA 到 eax
    movl    %eax,4(%esp)        # 將 RA 寫入 stack 次位
    movl    _eax_save,%eax      # 載回應該放入 eax 的 m[0] 資料

    ret                         # 結束 Context Switch, 結束函式
```