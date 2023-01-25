/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__ 
#define __USERPROG_KSYSCALL_H__ 

#include "kernel.h"

#include "synchconsole.h"


void SysHalt()
{
	kernel->interrupt->Halt();
}

void SysPrintInt(int val)
{ 
	DEBUG(dbgTraCode, "In ksyscall.h:SysPrintInt, into synchConsoleOut->PutInt, " << kernel->stats->totalTicks);
	kernel->synchConsoleOut->PutInt(val);
	DEBUG(dbgTraCode, "In ksyscall.h:SysPrintInt, return from synchConsoleOut->PutInt, " << kernel->stats->totalTicks);
}

int SysAdd(int op1, int op2)
{
	return op1 + op2;
}

int SysCreate(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->fileSystem->Create(filename);
}

OpenFileId SysOpen(char *name)
{
	// return value
	// -1: failed
	// otherwise: success
	return kernel->fileSystem->OpenAFile(name);
}

int SysRead(char * buffer, int size, OpenFileId id) 
{
	// return value: size of read bytes
	return kernel->fileSystem->ReadAFile(buffer, size, id);
}

int SysWrite(char * buffer, int size, OpenFileId id) 
{
	// return value: size of written bytes
	return kernel->fileSystem->WriteAFile(buffer, size, id);
}

int SysClose(OpenFileId id) 
{
	// return value
	// otherwise: success
	return kernel->fileSystem->CloseAFile(id);
}
#endif /* ! __USERPROG_KSYSCALL_H__ */
