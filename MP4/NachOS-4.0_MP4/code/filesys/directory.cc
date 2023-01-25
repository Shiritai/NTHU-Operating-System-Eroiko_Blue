// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "debug.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------
Directory::Directory(int size): count(0)
{
    table = new DirectoryEntry[size];

    // MP4 mod tag
    memset(table, 0, sizeof(DirectoryEntry) * size); // dummy operation to keep valgrind happy

    tableSize = size;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------
Directory::~Directory()
{
    delete[] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------
void Directory::FetchFrom(OpenFile *file)
{
    (void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
    count = 0; // reset count
    // count used elements after fetching
    for (int i = 0; i < tableSize; ++i)
        count += table[i].inUse;
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------
void Directory::WriteBack(OpenFile *file)
{
    (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------
int Directory::FindIndex(const char *name, bool isDir)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && table[i].isDir == isDir && 
            !strncmp(table[i].name, name, FileNameMaxLen))
            return i;
    return -1; // name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------
int Directory::Find(const char *name, bool isDir)
{
    int i = FindIndex(name, isDir);

    if (i != -1)
        return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//  "isDir" -- is the name represent a directory of file
//----------------------------------------------------------------------
bool Directory::Add(const char *name, int newSector, bool isDir)
{
    if (FindIndex(name, isDir) != -1) // file/directory exist
        return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) { // available position
            table[i].inUse = TRUE;
            table[i].isDir = isDir;
            strncpy(table[i].name, name, FileNameMaxLen);
            table[i].sector = newSector;
            DEBUG(dbgFile, "Add a new " << (isDir ? "directory" : "file") 
                << " with " << "name=" << name << ", sector=" << newSector);
            ++count;
            return TRUE;
        }
    return FALSE; // no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory.
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------
bool Directory::Remove(const char *name, bool isDir)
{
    int i = FindIndex(name, isDir);

    if (i == -1)
        return FALSE; // name not in directory
    table[i].inUse = FALSE;
    --count;
    return TRUE;
}

//----------------------------------------------------------------------
// Directory::SubDirectory
// 	    fetch the subdirectory instance according to given directory
//   the memory space is NOT managed by this directory!
//
//  Usage:
//  Directory * sub = new Directory(); // instance to obtain subdirectory
//  parentDir->SubDirectory(name, sub); // "sub" subdirectory is ready to be used
//  ---[DO_SOMETHING]---
//  delete sub;
//----------------------------------------------------------------------
void Directory::SubDirectory(char * name, Directory * fresh_dir)
{
    int i = FindIndex(name, IS_DIR);
    SubDirectory(i, fresh_dir);
}

void Directory::SubDirectory(int index, Directory * fresh_dir)
{
    OpenFile * sub_f = new OpenFile(table[index].sector);
    fresh_dir->FetchFrom(sub_f);
    delete sub_f;
}

//----------------------------------------------------------------------
// Directory::Apply
// 	Apply callback function to all the file/directory
//      in this directory recursively in DFS order
// 
// "callbackFile" -- callback function apply to all files
// "callbackDir" -- callback function apply to all directory
// "object" -- the additional object to 
//             operate with callbacks, default NULL
//----------------------------------------------------------------------
void Directory::Apply(
    void (* callbackFile)(int, void*), 
    void (* callbackDir)(int, void*), void* object)
{
    for (int i = 0, _cnt = count; _cnt; ++i) {
        if (table[i].inUse) {
            --_cnt;
            if (table[i].isDir) { // DFS
                Directory * sub = new Directory(NumDirEntries);
                SubDirectory(i, sub);
                sub->Apply(callbackFile, callbackDir, object);
                callbackDir(table[i].sector, object);
                delete sub;
            }
            else 
                callbackFile(table[i].sector, object);
        }
    }
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory.
//----------------------------------------------------------------------
void Directory::List()
{
    for (int i = 0, _cnt = count; _cnt; i++)
        if (table[i].inUse) {
            cout << (--_cnt ? "├" : "└") << "───["
                 << (table[i].isDir ? "D" : "F") 
                 << "] " << table[i].name << endl;
        }
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------
void Directory::Print()
{
    cout << "Directory structure:" << endl;
    Print(vector<bool>());
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------
void Directory::Print(vector<bool> indent_list)
{
    string front_proto = "", front = "";
	for (unsigned int i = 0; i < indent_list.size(); ++i) 
		front_proto += indent_list[i] ? "      " : "│     ";
        
    vector< vector<bool> > next_indent_list;
	next_indent_list.push_back(indent_list);
	next_indent_list.push_back(indent_list);
	next_indent_list[0].push_back(FALSE);
	next_indent_list[1].push_back(TRUE);

    int _cnt = count;
    bool last = IS_DIR;
    bool last_one = _cnt == 0;
    for (int i = 0; !last_one; ++i) {
        if (table[i].inUse) {
            last_one = !--_cnt;
            front = front_proto + (last_one ? "└" : "├") + "───";
            if (table[i].isDir) {
                if (last == IS_FILE)
                    cout << front_proto << "│" << endl;
                cout << front << " [D] name: " << table[i].name 
                    << ", first sector: " << table[i].sector << ", ind: " << i << endl;
                
                Directory * sub = new Directory(NumDirEntries);
                SubDirectory(i, sub);
                sub->Print(next_indent_list[last_one]);
                delete sub;

                if (!last_one)
                    cout << front_proto << "│" << endl;
                
                last = IS_DIR;
            }
            else {
                cout << front << " [F] name: " << table[i].name 
                    << ", first sector: " << table[i].sector 
                    << ", ind: " << i << endl;
                
                last = IS_FILE;
            }
        }
    }
}
