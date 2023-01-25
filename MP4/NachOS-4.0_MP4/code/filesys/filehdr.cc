// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"
#include <cmath>
#include <string> // for debug message!!!

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::ResetArrays
//	 reset all members
//----------------------------------------------------------------------
void FileHeader::ResetArrays() {
	ResetStackMemory();
	ResetHeapMemory();
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::ResetHeapMemory
//	 reset heap memory usage
//----------------------------------------------------------------------
void FileHeader::ResetHeapMemory() {
	// map
	if (map) {
		delete map;
		map = NULL;
	}
	// children[]
	for (int i = 0; i < N_SECTORS; ++i) {
		if (children[i]) {
			delete children[i];
			children[i] = NULL;
		}
	}
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::ResetStackMemory
//	 reset all arrays in stack memory
//----------------------------------------------------------------------
void FileHeader::ResetStackMemory() {
	memset(dataSectors, INVALID_SECTOR, sizeof(dataSectors)); 
	memset(startSector, INVALID_SECTOR, sizeof(startSector));
	memset(children, NULL, sizeof(children));
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::FileHeader
//	There is no need to initialize a fileheader,
//	since all the information should be initialized by Allocate or FetchFrom.
//	The purpose of this function is to keep valgrind happy.
//----------------------------------------------------------------------
FileHeader::FileHeader(int numBytes, int numSectors): 
	numBytes(numBytes), numSectors(numSectors), map(NULL), additionalSector(1)
{
	ResetStackMemory();
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::~FileHeader
//	Currently, there is not need to do anything in destructor function.
//	However, if you decide to add some "in-core" data in header
//	Always remember to deallocate their space or you will leak memory
//----------------------------------------------------------------------
FileHeader::~FileHeader()
{
	ResetHeapMemory();
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------
bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{
	int totalSectors = divRoundUp(fileSize, SectorSize);

	DEBUG(dbgFile, "Ready to allocate " << fileSize 
		<< " bytes in total " << totalSectors << " sectors");
		
	Allocate(freeMap, totalSectors, 0, fileSize);
	
	DEBUG(dbgFile, "End of allocation, " 
		<< "additional sectors of allocation is " << additionalSector);
	return TRUE;
}

/** 
 * Helper private recursive function to allocate tree structure of file header
 * @param freeMap free space map (bit vector) of disk
 * @param totalSector total number of sectors managed by this file header
 * @param currentSector leading sector count of current file header in the whole file header tree structure
 * @param totalBytes total bytes manages by this file header
 * @return additional sector count of file header
 */
int FileHeader::Allocate(
	PersistentBitmap *freeMap, int totalSector, 
	int currentSector, int totalBytes)
{
	// record file header size
	additionalSector = 1;
	
	// initialize members
	numSectors = totalSector;
	numBytes = totalBytes;

	ASSERT_MSG(freeMap->NumClear() > numSectors, 
		"Not enough space for allocation");

	// assert is NULL, or abort and print warning message!
	ASSERT_MSG(!map, 
		"Warning: attempt to allocate with fileSectors: " << totalSector 
		<< " into an FileHeader which the map (of value: " 
		<< map << ") member is not in the initial status!"
	);

	// initialize members for maintaining tree structure
	map = new Bitmap(N_SECTORS);

	// algorithm to build the header tree
	int i = 0; // index
	int remainSector = totalSector; // remain sectors to be allocate
	int remainBytes = totalBytes; // remain bytes to be allocate
	while (remainSector >= 1) {
		// 1. Calculate and adjust the bytes to be allocated in index i
		int curSector, curBytes; // attribute of current child
		if (i + 1 < N_SECTORS) {
			// curSector = N_SECTORS ^ floor(log_remainSector ln N_SECTORS)
			curSector = static_cast<int>(
				pow(N_SECTORS, floor(log(remainSector) / log(N_SECTORS))));
		}
		else 
			curSector = remainSector;
		if (curSector == totalSector) // exception of the former "if" statement
			curSector /= N_SECTORS;
		if (remainSector == 1)
			curSector = 1;
		
		curBytes = curSector * SectorSize;
		if (curBytes > remainBytes) 
			curBytes = remainBytes;

		// 2. Build tree node
		// record current byte position
		startSector[i] = currentSector + totalSector - remainSector;

		// allocation
		ASSERT_MSG((dataSectors[i] = freeMap->FindAndSet()) >= 0, 
			"Not enough space for single sector allocation!");

		if (curSector == 1) { 
			// only one -> place it directly
			children[i] = NULL;
		}
		else { 
			// more than one, allocate a new 
			// 		file header to contain the sectors
			children[i] = new FileHeader();
			map->Mark(i); // since non leaf
			additionalSector += children[i]->Allocate(freeMap, curSector, 
				currentSector + totalSector - remainSector, curBytes);
		}

		// 3. update remain data info
		remainSector -= curSector;
		remainBytes -= curBytes;
		++i;
	}

	ASSERT_MSG(!remainSector && !remainBytes, 
		"Error: remainSector: " << remainSector 
		<< " or remainBytes: " << remainBytes 
		<< " is not zero after allocated all file header space!");

	startSector[i] = currentSector + totalSector; // (additional) last position

	return additionalSector;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------
void FileHeader::Deallocate(PersistentBitmap *freeMap)
{
	for (int i = 0; i < N_SECTORS; i++) {
		// deallocate sectors directly managed in this file header
		if (map->Test(i) == LEAF) {
			if (dataSectors[i] != INVALID_SECTOR) {
				ASSERT_MSG(freeMap->Test(dataSectors[i]), 
					"Error occurs in deallocation part: "
					<< "the sector managed directly is corrupt!"); // ought to be marked!
				freeMap->Clear((int)dataSectors[i]);
			}
			else
				break;
		}
		// deallocate sectors managed by child nodes
		else {
			ASSERT_MSG(children[i], "Error occurs in deallocation part: "
				<< "the children pointer hasn't be set correctly "
				<< "when allocating / fetching");
			children[i]->Deallocate(freeMap);
			delete children[i];
			children[i] = NULL; // reset this child
			map->Clear(i); // reset child map
		}
	}
	// reset children info map after deallocation
	ResetArrays();
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------
void FileHeader::FetchFrom(int sector)
{
	DEBUG(dbgFile, "Start fetching whole file header from sector " << sector);
	FetchFrom(sector, 0);
	DEBUG(dbgFile, "End fetching whole file header with header size " 
		<< (additionalSector * SectorSize) << " bytes (" 
		<< additionalSector << " sectors)");
}

int FileHeader::FetchFrom(int sector, int currentSector)
{
	int cache[FILEHDR_SZ]; // cache to catch the data from disk
	kernel->synchDisk->ReadSector(sector, (char *) cache);

	/*
		MP4 Hint:
		After you add some in-core informations, you will need to rebuild the header's structure
	*/

	// assert is NULL, or abort and print warning message!
	ASSERT_MSG(!map, 
		"Warning: attempt to allocate with sector number: " << sector 
		<< " into an FileHeader which the map (of value: " 
		<< map << ") member is not in the initial status!"
	);

	// initialize members for maintaining tree structure
	map = new Bitmap(N_SECTORS);

	// initialize members of file header
	numBytes = cache[0];
	numSectors = cache[1];
	map->Import((unsigned int *) cache + MEMBER_SZ - 1);
	memcpy(dataSectors, cache + MEMBER_SZ, sizeof(dataSectors));

	// rebuild the tree structure use DFS
	additionalSector = 1;
	int childSector = 0; // count child sector
	int i;
	for (i = 0; i < N_SECTORS; ++i) {
		startSector[i] = currentSector + childSector;
		if (map->Test(i) == NON_LEAF) { // has children
			children[i] = new FileHeader();
			additionalSector += children[i]->FetchFrom(dataSectors[i], startSector[i]);
			childSector += children[i]->numSectors;
		}
		else if (dataSectors[i] == INVALID_SECTOR) // no more sectors
			break;
		else // leaf node
			++childSector;
	}

	startSector[i] = currentSector + childSector; // (additional) last position

	ASSERT_MSG(childSector == numSectors, 
		"The total sector count [" << numSectors 
		<< "] does not match with [" << childSector
		<< "] when re-building header tree use FetchFrom"
		<< ", where sector number is " << sector 
		<< " and currentSector is " << currentSector);

	return additionalSector;
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------
void FileHeader::WriteBack(int sector)
{
	/*
		MP4 Hint:
		After you add some in-core informations, you may not want to write all fields into disk.
		Use this instead:
		char buf[SectorSize];
		memcpy(buf + offset, &dataToBeWritten, sizeof(dataToBeWritten));
		...
	*/
	int cache[FILEHDR_SZ]; // cache of the data to be store into disk
	cache[0] = numBytes;
	cache[1] = numSectors;
	map->Export((unsigned int *) cache + MEMBER_SZ - 1);
	memcpy(cache + MEMBER_SZ, dataSectors, sizeof(dataSectors));

	// write back the information of the current file header
	kernel->synchDisk->WriteSector(sector, (char *) cache);

	// store the children file headers recursively
	for (int i = 0; i < N_SECTORS; ++i) {
		if (map->Test(i) == NON_LEAF) {
			ASSERT_MSG(children[i], "The children [" << i 
				<< "] is invalid now but attempt to write back...");
			children[i]->WriteBack(dataSectors[i]); // DFS
		}
	}
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------
int FileHeader::ByteToSector(int offset)
{
	// traverse to leaf node containing this file offset
	int offsetSector = offset / SectorSize;
	for (int i = 0; i < N_SECTORS; ++i) {
		if (offsetSector >= startSector[i] && offsetSector < startSector[i + 1]) {
			if (map->Test(i)) {// header node
				ASSERT_MSG(children[i], 
					"Invalid children sector found while fetching ByteToSector");
				return children[i]->ByteToSector(offset);
			}
			else // desired leaf node
				return dataSectors[i];
		}
	}
	// never reach
	ASSERT_MSG(FALSE, "Error: the file doesn't contain the offset: " << offset);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------
int FileHeader::FileLength()
{
	return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------
void FileHeader::Print(bool with_content)
{
	cout << "FileHeader contents:" << endl 
		<< "1. File size: " << numBytes << " bytes ("
		<< numSectors << " sectors)" << endl 
		<< "2. FileHeader Size in disk: " << (additionalSector * SectorSize) 
		<< " bytes (" << additionalSector << " sectors)" << endl
		<< "3. Memory Usage: " << sizeof(FileHeader) * additionalSector
		<< " bytes" << endl;
	cout << "4. File header structure:\n";
	Print(with_content, vector<bool>());
}

void FileHeader::Print(bool with_content, vector<bool> indent_list) {
	string front_proto = "", front = "";
	for (unsigned int i = 0; i < indent_list.size(); ++i) 
		front_proto += indent_list[i] ? "      " : "│     ";
	
	vector< vector<bool> > next_indent_list;
	next_indent_list.push_back(indent_list);
	next_indent_list.push_back(indent_list);
	next_indent_list[0].push_back(FALSE);
	next_indent_list[1].push_back(TRUE);

	char * buf = new char[N_SECTORS * SectorSize];
	
	bool last_one = false;
	bool last = NON_LEAF;
	for (int i = 0, k = 0; !last_one; i++) {
		last_one = i == N_SECTORS - 1 || dataSectors[i + 1] == -1;
		front = front_proto + (last_one ? "└" : "├") + "───";

		if (map->Test(i) == NON_LEAF) { // print in DFS order
			ASSERT_MSG(children[i], "The children [" << i 
				<< "] is invalid now but attempt to read and print...");

			if (last == LEAF)
				cout << front_proto << "│" << endl;
			cout << front << " [T] start: " << startSector[i] 
				<< ", sector: " << dataSectors[i] << ", ind: " << i << endl;
			
			children[i]->Print(with_content, next_indent_list[last_one]);
			if (!last_one)
				cout << front_proto << "│" << endl;
				
			last = NON_LEAF;
		}
		else {
			ASSERT_MSG(!children[i], 
				"Attempt to print a node sector at index: " << i);
			cout << front << " [E] start: " << startSector[i] 
				<< ", sector: " << dataSectors[i] << ", ind: " << i;

			if (with_content) {
				cout << ", content: { ";
				kernel->synchDisk->ReadSector(dataSectors[i], buf);
				for (int j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
					if ('\040' <= buf[j] && buf[j] <= '\176') // isprint(buf[j])
						printf("%c", buf[j]);
					else
						printf("\\%x", (unsigned char) buf[j]);
				}
				cout << " }";
			}

			cout << endl;
			
			last = LEAF;
		}
	}

	delete buf;
}