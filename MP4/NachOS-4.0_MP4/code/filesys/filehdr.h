// filehdr.h
//	Data structures for managing a disk file header.
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "pbitmap.h"
#include <vector> // for debug message!!!

#define NumDirect ((SectorSize - 2 * sizeof(int)) / sizeof(int))
#define MaxFileSize (NumDirect * SectorSize)

// The following class defines the Nachos "file header" (in UNIX terms,
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks.
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.
class FileHeader
{
public:
	// MP4 mod tag
	FileHeader(int numBytes = -1, int numSectors = -1);
	~FileHeader();

	bool Allocate(PersistentBitmap *freeMap, int fileSize); // Initialize a file header,
														   //  including allocating space
														   //  on disk for the file data
	void Deallocate(PersistentBitmap *freeMap);			   // De-allocate this file's
														   //  data blocks

	void FetchFrom(int sectorNumber); // Initialize file header from disk
	void WriteBack(int sectorNumber); // Write modifications to file header
									  //  back to disk

	int ByteToSector(int offset); // Convert a byte offset into the file
								  // to the disk sector containing
								  // the byte

	int FileLength(); // Return the length of the file
					  // in bytes

	void Print(bool with_content = false); // Print the contents of the file.

private:
	void ResetStackMemory();
	void ResetHeapMemory();
	void ResetArrays();


	int Allocate(PersistentBitmap *freeMap, int totalSectors, int currentSector, int totalByte); // Initialize a file header,
														   //  including allocating space
														   //  on disk for the file data

	int FetchFrom(int sectorNumber, int currentSector); // Initialize file header from disk

	void Print(bool with_content, vector<bool> indent_list); // Print the contents of the file without header part

	// static constant members
	static const int FILEHDR_SZ = SectorSize / sizeof(int); // size of FileHeader
	static const int MEMBER_SZ = 3; // # of non-array members
	static const int N_SECTORS = FILEHDR_SZ - MEMBER_SZ; // available # of index to record sector
	static const bool LEAF = FALSE; // LEAF node mark
	static const bool NON_LEAF = TRUE; // NON_LEAF node mark
	static const int INVALID_SECTOR = -1; // represent invalid sector

	/*
		MP4 hint:
		You will need a data structure to store more information in a header.
		Fields in a class can be separated into disk part and in-core part.
		Disk part are data that will be written into disk.
		In-core part are data only lies in memory, and are used to maintain the data structure of this class.
		In order to implement a data structure, you will need to add some "in-core" data
		to maintain data structure.
		
		Disk Part - numBytes, numSectors, dataSectors occupy exactly 128 bytes and will be
		written to a sector on disk.
		In-core part - none
		
	*/

	//* MP4 in-disk part
	int numBytes;	 // Number of bytes in the file
	int numSectors;	 // Number of data sectors in the file

	/** 
	 * Map that determine whether the column 
	 * is a node sector (TRUE) or data sector (FALSE)
	 * 
	 * The reason of the existence of this member 
	 * is to identify whether the content of dataSectors 
	 * represents a leaf data or a header node
	 */
	Bitmap * map;
	
	/** 
	 * Disk sector numbers for each data 
	 * block in the file
	 */
	int dataSectors[N_SECTORS];	

	//+ MP4 in-core part
	
	/** 
	 * leading sector count of current file header 
	 * among the whole tree, the additional one position 
	 * is for traversing convenience
	 */
	int startSector[N_SECTORS + 1];
	FileHeader * children[N_SECTORS];
	int additionalSector;
};

#endif // FILEHDR_H
