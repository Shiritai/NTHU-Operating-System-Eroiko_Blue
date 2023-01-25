# Trace record

## $\tt{SC\_Create}$

```cpp
case SC_Create:
    // Read the function argument
    val = kernel->machine->ReadRegister(4);
    {
        char *filename = &(kernel->machine->mainMemory[val]);
        //cout << filename << endl;
        status = SysCreate(filename);
        kernel->machine->WriteRegister(2, (int) status);
    }
    kernel->machine->WriteRegister(
        PrevPCReg, 
        kernel->machine->ReadRegister(PCReg));
    kernel->machine->WriteRegister(
        PCReg, 
        kernel->machine->ReadRegister(PCReg) + 4);
    kernel->machine->WriteRegister(
        NextPCReg, 
        kernel->machine->ReadRegister(PCReg)+4);
    return;
    ASSERTNOTREACHED();
    break;
```