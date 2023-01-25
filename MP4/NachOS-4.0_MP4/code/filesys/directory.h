// directory.h
//	Data structures to manage a UNIX-like directory of file names.
//
//      A directory is a table of pairs: <file name, sector #>,
//	giving the name of each file in the directory, and
//	where to find its file header (the data structure describing
//	where to find the file's data blocks) on disk.
//
//      We assume mutual exclusion is provided by the caller.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"
#include <vector> // for Print

#define IS_FILE FALSE // this is a file
#define IS_DIR  TRUE  // this is a directory
#define NumDirEntries 64 // max number of directory entry
#define FileNameMaxLen 9 // for simplicity, we assume
                         // file names are <= 9 characters long


// The following class defines a "directory entry", representing a file
// in the directory.  Each entry gives the name of the file, and where
// the file's header is to be found on disk.
//
// Internal data structures kept public so that Directory operations can
// access them directly.
class DirectoryEntry
{
public:
    bool isDir;                    // Is this entry is a subdirectory?
    bool inUse;                    // Is this entry in use?
    int sector;                    // Location on disk to find the
                                   //   FileHeader for this file
    char name[FileNameMaxLen + 1]; // Text name for file, with +1 for
                                   // the trailing '\0'
};

// The following class defines a UNIX-like "directory".  Each entry in
// the directory describes a file, and where to find it on disk.
//
// The directory data structure can be stored in memory, or on disk.
// When it is on disk, it is stored as a regular Nachos file.
//
// The constructor initializes a directory structure in memory; the
// FetchFrom/WriteBack operations shuffle the directory information
// from/to disk.
class Directory
{
public:
    Directory(int size = NumDirEntries); // Initialize an empty directory
                         // with space for "size" files
    ~Directory();        // De-allocate the directory

    void FetchFrom(OpenFile *file); // Init directory contents from disk
    void WriteBack(OpenFile *file); // Write modifications to
                                    // directory contents back to disk

    int Find(const char *name, bool isDir = IS_FILE); // Find the sector number of the
                          // FileHeader for file: "name"

    bool Add(const char *name, int newSector, bool isDir = IS_FILE); // Add a file name into the directory

    bool Remove(const char *name, bool isDir = IS_FILE); // Remove a file from the directory

    void List();  // Print the names of all the files
                  //  in the directory
    void Print(); // Verbose print of the contents
                  //  of the directory -- all the file
                  //  names and their contents.

    void SubDirectory(char * name, Directory * fresh_dir);

    void Apply(void (* callbackFile)(int, void*), 
        void (* callbackDir)(int, void*), void* object = NULL);

private:
    int FindIndex(const char *name, bool isDir); // Find the index into the directory
                               //  table corresponding to "name"

    void SubDirectory(int index, Directory * fresh_dir);

    void Print(vector<bool> indent_list);

    /*
		MP4 Hint:
		Directory is actually a "file", be careful of how it works with OpenFile and FileHdr.
		Disk part: table
		In-core part: tableSize
	*/
    //* MP4 in-disk part
    int tableSize;         // Number of directory entries
    DirectoryEntry *table; // Table of pairs:
                           // <file name, file header location>

    //+ MP4 in-core part
    int count; // # of element in this directory
};

#endif // DIRECTORY_H
