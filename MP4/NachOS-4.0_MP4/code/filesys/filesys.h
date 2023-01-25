// filesys.h
//	Data structures to represent the Nachos file system.
//
//	A file system is a set of files stored on disk, organized
//	into directories.  Operations on the file system have to
//	do with "naming" -- creating, opening, and deleting files,
//	given a textual file name.  Operations on an individual
//	"open" file (read, write, close) are to be found in the OpenFile
//	class (openfile.h).
//
//	We define two separate implementations of the file system.
//	The "STUB" version just re-defines the Nachos file system
//	operations as operations on the native UNIX file system on the machine
//	running the Nachos simulation.
//
//	The other version is a "real" file system, built on top of
//	a disk simulator.  The disk is simulated using the native UNIX
//	file system (in a file named "DISK").
//
//	In the "real" implementation, there are two key data structures used
//	in the file system.  There is a single "root" directory, listing
//	all of the files in the file system; unlike UNIX, the baseline
//	system does not provide a hierarchical directory structure.
//	In addition, there is a bitmap for allocating
//	disk sectors.  Both the root directory and the bitmap are themselves
//	stored as files in the Nachos file system -- this causes an interesting
//	bootstrap problem when the simulated disk is initialized.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef FS_H
#define FS_H

#include "copyright.h"
#include "sysdep.h"
#include "openfile.h"
#include "directory.h"
#include "pbitmap.h"
#include <map>

typedef int OpenFileId;

#ifdef FILESYS_STUB // Temporarily implement file system calls as
// calls to UNIX, until the real file system
// implementation is available
class FileSystem
{
public:
	FileSystem()
	{
		for (int i = 0; i < 20; i++)
			fileDescriptorTable[i] = NULL;
	}

	bool Create(char *name)
	{
		int fileDescriptor = OpenForWrite(name);

		if (fileDescriptor == -1)
			return FALSE;
		Close(fileDescriptor);
		return TRUE;
	}

	OpenFile *Open(char *name)
	{
		int fileDescriptor = OpenForReadWrite(name, FALSE);

		if (fileDescriptor == -1)
			return NULL;
		return new OpenFile(fileDescriptor);
	}

	bool Remove(char *name) { return Unlink(name) == 0; }

	OpenFile *fileDescriptorTable[20];
};

#else // FILESYS
class FileSystem
{
public:
	FileSystem(bool format); // Initialize the file system.
							 // Must be called *after* "synchDisk"
							 // has been initialized.
							 // If "format", there is nothing on
							 // the disk, so initialize the directory
							 // and the bitmap of free blocks.
	// MP4 mod tag
	~FileSystem();

	bool Create(const char *path, int initialSize);
	// Create a file (UNIX creat)

	OpenFile * OpenAsOpenFile(const char *path); // Open a file (UNIX open)

	OpenFileId Open(const char *path); // Open a file (UNIX open)

	int Read(char *buf, int size, OpenFileId id); // Read a file into buffer

	int Write(char *buf, int size, OpenFileId id); // Read a file into buffer

	int Close(OpenFileId id); // Read a file into buffer

	bool Remove(const char *path); // Delete a file (UNIX unlink)

	void List(const char *path, bool recursively = false); // List all the files in the file system

	void Print(); // List all the files and their contents

	void PrintStructure(const char * path); // print the structure of an file

	bool Mkdir(const char * path); // make a directory

	bool RecursiveRemove(const char *path); // Delete a file (UNIX unlink)

private:
	/** 
	 * Path traverser that automatically 
	 * manage the memory usage
	 * 
	 * @param path path to traverse
	 * @param isDir whether path is for a directory
	 * @param root OpenFile of root directory
	 * @param flush_dir whether to flush information after releasing TraversePath
	 */
	class TraversePath {
	private:
		/** 
		 * Parse the path and return each path step separately
		 * Will always start from a "/" (root) element
		 * i.e. length return vector always has at least an element
		 * @param path char array represent the file/directory path
		 */
		static vector<string> ParsePath(const char * path);

		OpenFile * root; // OpenFile of root directory of traversing
		bool flush_dir; // whether should flush the info of directory
	public:
		/** 
		 * Path traverser that automatically 
		 * manage the memory usage
		 * 
		 * @param path path to traverse
		 * @param isDir whether path is for a directory
		 * @param root OpenFile of root directory
		 * @param flush_dir whether to flush information after releasing TraversePath
		 */
		TraversePath(const char * path, bool isDir, 
			OpenFile * root, bool flush_dir = true);
		~TraversePath();
		
		Directory * dir; // parent directory containing the target file/directory
		OpenFile * dirFile; // OpenFile of current (parent) directory
		char name[FileNameMaxLen + 1]; // name of target file without parent path name
		bool success; // does the traverse success
		bool exist; // is the file/directory exist
	};

	bool CreateFileDir(const char * path, bool isDir, int initialSize = 0);

	static void RemoveSingleObj(int sector, PersistentBitmap * freeMap);

	OpenFile *freeMapFile;	 // Bit map of free disk blocks,
							 // represented as a file
	OpenFile *directoryFile; // "Root" directory -- list of
							 // file names, represented as a file

	map<OpenFileId, OpenFile*> opTable; // opened file table
};

#endif // FILESYS

#endif // FS_H
