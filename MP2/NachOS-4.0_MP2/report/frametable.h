#ifndef FRAME_TABLE_H
#define FRAME_TABLE_H

#include "machine.h"
#include "list.h"

/** 
 * Data structure of Virtual Memory
 */
typedef TranslationEntry* PageTable;

/** 
 * Data structure of Physical Memory
 */
class FrameTable {
public:
    /** 
     * Initialize a frame table
     */
    FrameTable();
    ~FrameTable();

    /** 
     * Allocate pageNum of frames (pages) and collect
     * corresponding translation informations into a page table.
     * 
     * @param pageNum numbers of pages
     * @return a new Page table, NULL if not enough memory space
     */
    PageTable Allocate(uint pageNum);

    /** 
     * Release the physical memory frame 
     * which the info stored in PageTable
     */
    void Release(PageTable ptb, int pageNum);

    /** 
     * @return the remaining numbers of entry of the frame table
     */
    uint RemainSize();

private:
    List<int> available;
};

FrameTable::FrameTable() {
    for (int i = 0; i < NumPhysPages; ++i)
        available.Append(i);
}

FrameTable::~FrameTable() {}

uint FrameTable::RemainSize() { return available.NumInList(); }

PageTable FrameTable::Allocate(uint pageNum)
{
    // if not enough memory
    if (RemainSize() < pageNum) 
        return NULL;

    PageTable ptb = new TranslationEntry[pageNum];

    for (int i = 0; i < pageNum; ++i) {
        ptb[i].virtualPage = i;
        int f = available.RemoveFront(); // frame number
        ptb[i].physicalPage = f;

        ptb[i].valid = TRUE;
        ptb[i].use = FALSE;
        ptb[i].dirty = FALSE;
        ptb[i].readOnly = FALSE;

        // zero out the entire address space
        bzero(kernel->machine->mainMemory + f * PageSize, PageSize);
    }
    return ptb;
}

void FrameTable::Release(PageTable ptb, int pageNum) {
    if (!ptb)
        return; // nothing to release
    for (int i = 0; i < pageNum; ++i)
        available.Append(ptb[i].physicalPage);
    delete [] ptb;
}

#endif /* FRAME_TABLE_H */