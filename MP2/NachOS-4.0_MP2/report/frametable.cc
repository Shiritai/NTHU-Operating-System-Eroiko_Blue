#include "frametable.h"
#include "main.h"

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