// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in machine.h.
//----------------------------------------------------------------------
void
ExceptionHandler(ExceptionType which)
{
    char ch;
    int val;
    int type = kernel->machine->ReadRegister(2);	
    int status, exit, threadID, programID, fileID, numChar;
    DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");
    DEBUG(dbgTraCode, "In ExceptionHandler(), Received Exception " << which << " type: " << type << ", " << kernel->stats->totalTicks);
    switch (which) {

    case SyscallException:
		switch(type) {

		case SC_Halt:
			DEBUG(dbgSys, "Shutdown, initiated by user program.\n");
			SysHalt();
			cout<<"in exception\n";
			ASSERTNOTREACHED();
			break;

		case SC_PrintInt:
			DEBUG(dbgSys, "Print Int\n");
			val=kernel->machine->ReadRegister(4);//從register4讀取要print的資料
			DEBUG(dbgTraCode, "In ExceptionHandler(), into SysPrintInt, " << kernel->stats->totalTicks);    
			SysPrintInt(val); 	
			DEBUG(dbgTraCode, "In ExceptionHandler(), return from SysPrintInt, " << kernel->stats->totalTicks);
			// 更新 Program Counter
			kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));// 將PCReg 的資料寫到PrevPCReg
			kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);// 將PCReg +4的資料寫到PCReg
			kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);//將(舊的PCReg +4)+4 存到NextPCReg
			return;
			ASSERTNOTREACHED();
			break;

		case SC_MSG: 
			DEBUG(dbgSys, "Message received.\n");
			val = kernel->machine->ReadRegister(4);
			{
				char *msg = &(kernel->machine->mainMemory[val]);
				cout << msg << endl;
			}
			SysHalt();
			ASSERTNOTREACHED();
			break;

		case SC_Create:
			// fetch the argument store in a0 register, 
			// which should be a file pointer
			val = kernel->machine->ReadRegister(4);
			{ // use a block to initialize a variable inside the switch-case 
				// fetch the file name stored inside the main memory 
				char *filename = &(kernel->machine->mainMemory[val]);

				// my debug message
				DEBUG(dbgSys, "Create file: " << filename << endl);

				// create a file with the filename, 
				// implement in ksyscall.h
				status = SysCreate(filename);
				
				// write the file-creation status to register 2
				kernel->machine->WriteRegister(2, (int) status);
			}
			// store current PC to the "previous PC", for debugging
			kernel->machine->WriteRegister(
				PrevPCReg, kernel->machine->ReadRegister(PCReg));
			// PC += 4, i.e. point to the next instruction
			kernel->machine->WriteRegister(
				PCReg, kernel->machine->ReadRegister(PCReg) + 4);
			// "next PC" = PC + 4, 
			// i.e. calculate the next instruction for branch delaying
			kernel->machine->WriteRegister(
				NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			return; // end of function call
			ASSERTNOTREACHED(); // should never reach this
			break;

		case SC_Add:
			DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");
			/* Process SysAdd Systemcall*/
			int result;
			result = SysAdd(/* int op1 */(int)kernel->machine->ReadRegister(4),
			/* int op2 */(int)kernel->machine->ReadRegister(5));
			DEBUG(dbgSys, "Add returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)result);	
			/* Modify return point */
			/* set previous programm counter (debugging only)*/
			kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));
			/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
			kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
			/* set next programm counter for brach execution */
			kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
			cout << "result is " << result << "\n";	
			return;	
			ASSERTNOTREACHED();
			break;

		//+ start of the implementation of MP1
		case SC_Open:
			// DEBUG(dbgSys, "File Open\n");
			val = kernel->machine->ReadRegister(4);
			{
				char * filename = &(kernel->machine->mainMemory[val]);
				status = SysOpen(filename);

				// my debug message
				DEBUG(dbgSys, "Open file: " << filename << endl);
				
				kernel->machine->WriteRegister(2, status);
			}
			kernel->machine->WriteRegister(
				PrevPCReg, kernel->machine->ReadRegister(PCReg));
			kernel->machine->WriteRegister(
				PCReg, kernel->machine->ReadRegister(PCReg) + 4);
			kernel->machine->WriteRegister(
				NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
			return; // end of function call
			ASSERTNOTREACHED(); // should never reach this
			break;

		case SC_Read:
			// DEBUG(dbgSys, "File Read\n");
			val = kernel->machine->ReadRegister(4);
			{
				char * buffer = &(kernel->machine->mainMemory[val]);
				int size = kernel->machine->ReadRegister(5);
				OpenFileId id = kernel->machine->ReadRegister(6);
				status = SysRead(buffer, size, id);

				// my debug message
				DEBUG(dbgSys, "Read: " << buffer << " for file id: " << id << " with size: "  << size << endl);
				
				kernel->machine->WriteRegister(2, status);
			}
			kernel->machine->WriteRegister(
				PrevPCReg, kernel->machine->ReadRegister(PCReg));
			kernel->machine->WriteRegister(
				PCReg, kernel->machine->ReadRegister(PCReg) + 4);
			kernel->machine->WriteRegister(
				NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
			return; // end of function call
			ASSERTNOTREACHED(); // should never reach this
			break;

		case SC_Write:
			// DEBUG(dbgSys, "File Write\n");
			val = kernel->machine->ReadRegister(4);
			{
				char * buffer = &(kernel->machine->mainMemory[val]);
				int size = kernel->machine->ReadRegister(5);
				OpenFileId id = kernel->machine->ReadRegister(6);
				status = SysWrite(buffer, size, id);

				// my debug message
				DEBUG(dbgSys, "Write: " << buffer << " for file id: " << id << " with size: "  << size << endl);
				
				kernel->machine->WriteRegister(2, status);
			}
			kernel->machine->WriteRegister(
				PrevPCReg, kernel->machine->ReadRegister(PCReg));
			kernel->machine->WriteRegister(
				PCReg, kernel->machine->ReadRegister(PCReg) + 4);
			kernel->machine->WriteRegister(
				NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
			return; // end of function call
			ASSERTNOTREACHED(); // should never reach this
			break;

		case SC_Close:
			DEBUG(dbgSys, "File Close\n");
			val = kernel->machine->ReadRegister(4);
			status = SysClose(val);
			kernel->machine->WriteRegister(2, status);
			kernel->machine->WriteRegister(
				PrevPCReg, kernel->machine->ReadRegister(PCReg));
			kernel->machine->WriteRegister(
				PCReg, kernel->machine->ReadRegister(PCReg) + 4);
			kernel->machine->WriteRegister(
				NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
			return; // end of function call
			ASSERTNOTREACHED(); // should never reach this
			break;
		//+ end of the implementation of MP1

		case SC_Exit:
			DEBUG(dbgAddr, "Program exit\n");
					val=kernel->machine->ReadRegister(4);
					cout << "return value:" << val << endl;
			kernel->currentThread->Finish();
			break;
		default:
			cerr << "Unexpected system call " << type << "\n";
			break;
		}
		break;
	default:
		cerr << "Unexpected user mode exception " << (int)which << "\n";
		break;
    }
    ASSERTNOTREACHED();
}

