#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "LRU.h"

static buddyAllocator buddy;
static occupiedBlock** occupiedBlocks;
//VirMem only used to record under certain seqno, whether it is still in virtual memory.
static int** virtualMemory;
/**********************************************************
 * Quality of life helper functions / macros
 *********************************************************/
#define powOf2(E) (1 << E)


// Useful Macros and Buddy Allocator Set-up
unsigned int log2Ceiling( unsigned int N )
/**********************************************************
 * Find the smallest S, such that 2^S >= N
 * S is returned
 *********************************************************/
{
    unsigned int s = 0, pOf2 = 1;

    while( pOf2 < N){
        pOf2 <<= 1;
        s++;
    }

    return s;
}

unsigned int log2Floor( unsigned int N )
/**********************************************************
 * Find the largest S, such that 2^S <= N
 * S is returned
 *********************************************************/
{
    unsigned int s = 0, pOf2 = 1;

    while( pOf2 <= N){
        pOf2 <<= 1;
        s++;
    }

    return s-1;
}

unsigned int buddyOf( unsigned int addr, unsigned int lvl )
/**********************************************************
 * Return the buddy address of address (addr) at level (lvl)
 *********************************************************/
{
    unsigned int mask = 0xFFFFFFFF << lvl;
    unsigned int buddyBit = 0x0001 << lvl;

    return (addr & mask) ^ buddyBit;
}


// Block builders and modifiers
buddyBlock* buildBuddyBlock(unsigned int offset)
/**********************************************************
 * Allocate a new partInfo structure and initialize the fields
 *********************************************************/
{
    buddyBlock *blockPtr;

    blockPtr = (buddyBlock*) malloc(sizeof(buddyBlock));

    blockPtr->offset = offset;
	blockPtr->nextPart = NULL;

    //Buddy system's partition size is implicit
	//piPtr->size = size;

    //All available partition in buddy system is implicitly free
	//piPtr->status = FREE;

    return blockPtr;
}


void buildOccupiedBlock(int seqno, int address, int actualSize, int allocatedSize) {
    occupiedBlock* blkPtr = (occupiedBlock*) malloc(sizeof(occupiedBlock));
    blkPtr->seqno = seqno;
    blkPtr->address = address;
    blkPtr->actualSize = actualSize;
    blkPtr->allocatedSize = allocatedSize;
    // Allocate a int[10], because in the .dat file, the maximum of allocation is 10.
    blkPtr->pageOffsets = (int*) malloc(10 * sizeof(int));
    for (int i = 0; i < 10; i++) {
        if (i < allocatedSize) blkPtr->pageOffsets[i] = 1;
        else blkPtr->pageOffsets[i] = 0;
    }
    occupiedBlocks[seqno] = blkPtr;
}

void freeOccupiedBlock(int seqno, int numOfPages) {
    occupiedBlock* blk = occupiedBlocks[seqno];
    if (blk == NULL) return;
    int modifiedSize = blk->actualSize - numOfPages;
    
    if (modifiedSize <= 0) {
        buddyBlockFree(blk->address, blk->allocatedSize);
        occupiedBlocks[seqno] = NULL;
        free(blk);
    } else if (modifiedSize <= blk->allocatedSize / 2) {
        // Modify buddy allocator: Reallocate
        buddyBlockFree(blk->address, blk->actualSize);
        int newBuddyBlockAddress = buddyBlockMalloc(modifiedSize);
        // Modify occupied blocks
        blk->actualSize = modifiedSize;
        blk->allocatedSize = 1 << log2Ceiling(modifiedSize);
        blk->address = newBuddyBlockAddress;
        // Modify page offsets in occupied blocks
        int count = 0;
        int* OBPages = blk->pageOffsets;
        for (int i = 0; i < 10; i++) {
            if (OBPages[i] == 1) {
                count++;
                OBPages[i] = 0;
            }
            if (count == numOfPages) break;
        }
    } else {
        // Modify page offsets in occupied blocks
        int count = 0;
        int* OBPages = blk->pageOffsets;
        for (int i = 0; i < 10; i++) {
            if (OBPages[i] == 1) {
                count++;
                OBPages[i] = 0;
            }
            if (count == numOfPages) break;
        }
    }
}

void allocateVirtualMemory(int seqno, int actualSize) {
    int* pageOffsetList = (int*) malloc(10 * sizeof(int));
    for (int i = 0; i < 10; i++) {
        if (i < actualSize) pageOffsetList[i] = 1;
        else pageOffsetList[i] = 0;
    }
    virtualMemory[seqno] = pageOffsetList;
}

void freeVirtualMemory(int seqno, int numOfPages) {
    int* pageOffsetList = virtualMemory[seqno];
    if (pageOffsetList == NULL) return;
    int count = 0;
    for (int i = 0; i < 10; i++) {
        if (pageOffsetList[i] == 1) {
            count++;
            pageOffsetList[i] = 0;
        }
        if (count == numOfPages) break;
    }
    if (count < numOfPages) {
        virtualMemory[seqno] = NULL;
        free(pageOffsetList);
    }
}

void freeOccupiedBlockOnEvict(int seqno, int offset) {
    occupiedBlock* blk = occupiedBlocks[seqno];
    if (blk == NULL) return;
    int modifiedSize = blk->actualSize - 1;
    
    if (modifiedSize <= 0) {
        buddyBlockFree(blk->address, blk->allocatedSize);
        occupiedBlocks[seqno] = NULL;
        free(blk);
    } else if (modifiedSize <= blk->allocatedSize / 2) {
        // Modify buddy allocator: Reallocate
        buddyBlockFree(blk->address, blk->actualSize);
        int newBuddyBlockAddress = buddyBlockMalloc(modifiedSize);
        // Modify occupied blocks
        blk->actualSize = modifiedSize;
        blk->allocatedSize = 1 << log2Ceiling(modifiedSize);
        blk->address = newBuddyBlockAddress;
        // Modify page offsets in occupied blocks
        int* OBPages = blk->pageOffsets;
        OBPages[offset] = 0;
    } else {
        blk->actualSize = modifiedSize;
        // Modify page offsets in occupied blocks
        int* OBPages = blk->pageOffsets;
        OBPages[offset] = 0;
    }
}


// Debug use printers
void printBuddyBlockList(buddyBlock* piPtr)
/**********************************************************
 * Print a buddy allocator block linked list
 *********************************************************/
{
	buddyBlock* current;
    int count = 1;
	
	for ( current = piPtr; current != NULL; 
		current = current->nextPart){
        if (count % 8 == 0){
            printf("\t");
        }
		printf("[+%5d] ", current->offset);
        count++;
        if (count % 8 == 0){
            printf("\n");
        }
	}
    printf("\n");
}

void printBuddyAllocatorMetaInfo()
/**********************************************************
 * Print Heap Internal Bookkeeping Information
 *********************************************************/
{
    int i;

	printf("\nBuddy Allocator Info:\n");
	printf("===============\n");
	printf("Total Size = %d pages\n", buddy.totalSize);
	printf("Start Address = %d\n", 0);

    for (i = buddy.maxIdx; i >=0; i--){
        printf("A[%d]: ", i);
        printBuddyBlockList(buddy.A[i] );
    }

}

void printBuddyAllocatorStatistic()
/**********************************************************
 * Print Heap Usage Statistics
 *********************************************************/
{
    //TODO: Task 4. Calculate and report the various statistics

    printf("\nAllocator Statistics:\n");
    printf("======================\n");

   //Remember to preserve the message format!

    printf("Total Space: %d pages\n", buddy.totalSize);

    int totalFreePartition = 0;
    int totalFreeSize = 0;
    for (int lvl = 1; lvl <= buddy.maxIdx; lvl++) {
        int levelSize = 1;
        levelSize <<= lvl;
        buddyBlock* levelCursor = buddy.A[lvl];
        while (levelCursor != NULL) {
            totalFreePartition++;
            totalFreeSize += levelSize;
            levelCursor = levelCursor->nextPart;
        }
    }
    
    printf("Total Free Partitions: %d\n", totalFreePartition);
    printf("Total Free Size: %d pages\n", totalFreeSize);

    printf("Total Internal Fragmentation: %d pages\n", buddy.internalFragTotal);
}


void printOccupiedBlocks()
/**********************************************************
 * Print the content of the occupied blocks.
 *********************************************************/
{
    //Included as last debugging mechanism.
    //Print the entire heap regions as integer values.
    printf("\nOccpuied Blocks:\n");
    printf("======================\n");
    for (int i = 0; i < 800; i++){
        if (i % 4 == 0){
            printf("[+%5d] |", i);
        }
        occupiedBlock* blk = occupiedBlocks[i];
        if (blk == NULL) printf("NULL; ");
        else {
            printf("seq = %d ",blk->seqno);
            printf("addr = %d ",blk->address);
            printf("actusize = %d ",blk->actualSize);
            printf("allosize = %d; ",blk->allocatedSize);
        }
        if ((i+1) % 4 == 0){
            printf("\n");
        }
    }
}

void printVirtualMemory()
/**********************************************************
 * Print the content of VM.
 *********************************************************/
{
    printf("\nVirtual Memory:\n");
    printf("======================\n");
    for (int i = 0; i < 800; i++){
        printf("seqno:[+%5d] |", i);
        int* pageOffsets = virtualMemory[i];
        if (pageOffsets == NULL) printf("NULL ;");
        else {
            for (int j = 0; j < 10; j++) {
            printf("idx = %d signal = %d ;",j, pageOffsets[j]);
        }
        }
        printf("\n");
    }
    printf("\n");
}



// Setup
int setupBuddy(int initialSize)
/**********************************************************
 * Setup a heap with "initialSize" pages
 *********************************************************/
{
    buddy.base = 0;
	buddy.totalSize = initialSize;
    buddy.internalFragTotal = 0;
	
    //TODO: Task 1. Setup the rest of the bookkeeping info:
    //       hmi.A <= an array of partition linked list
    //       hmi.maxIdx <= the largest index for hmi.A[]
    int numOfLevels = log2Floor(initialSize);
    buddy.A = (buddyBlock**) malloc((numOfLevels + 1) * sizeof(buddyBlock*));
    buddy.maxIdx = numOfLevels;
    for (int i = 0; i < numOfLevels + 1; i++) {
        buddy.A[i] = NULL;
    }
    buddyBlock* start = malloc(sizeof(buddyBlock));
    start->offset = 0;
    start->nextPart = NULL;
    buddy.A[numOfLevels] = start;

    return 1;
}

int setupOccupiedBlocks() {
    // set the size to 800 because the maximum seqno is less than 800.
    int size = 800;
    occupiedBlocks = (occupiedBlock**) malloc(size * sizeof(occupiedBlock*));
    for (int i = 0; i < size; i++) {
       occupiedBlocks[i] = NULL;
    }
    return 1;
}

int setupVirtualMemory() {
    // set the size to 800 because the maximum seqno is less than 800.
    int size = 800;
    virtualMemory = (int**) malloc(size * sizeof(int*));
    for (int i = 0; i < size; i++) {
       virtualMemory[i] = NULL;
    }
    return 1;
}

// Buddy Allocator algorithms:
void addBlockAtLevel( unsigned int lvl, unsigned int offset )
/**********************************************************
 *    This function adds a new free partition with "offset" at hmi.A[lvl]
 *    If buddy is found, recursively (or repeatedly) perform merging and insert
 *      at higher level
 *********************************************************/
{   
    if (lvl > buddy.maxIdx) return;

    buddyBlock* levelHead = buddy.A[lvl];
    // Find Buddy
    int buddyOffset = buddyOf(offset, lvl);
    // printf("The buddy OFFSET IS : %d \n",buddyOffset);
    buddyBlock* findBuddyCursor = levelHead;
    buddyBlock* findBuddyPrevCursor = NULL;
    // Do linear search to find the buddy.
    while (findBuddyCursor != NULL){
        if (findBuddyCursor->offset == buddyOffset) {
            break;
        }
        findBuddyPrevCursor = findBuddyCursor;
        findBuddyCursor = findBuddyCursor->nextPart;
    }
    
    // If we find a buddy:
    if (findBuddyCursor != NULL) {
        int nextLevelOffset = (buddyOffset < offset) ? buddyOffset : offset;
        // remove the buddy
        if (findBuddyPrevCursor == NULL) {
            levelHead = findBuddyCursor->nextPart;
            findBuddyCursor->nextPart = NULL;
            buddy.A[lvl] = levelHead;
        } else {
            findBuddyPrevCursor->nextPart = findBuddyCursor->nextPart;
            findBuddyCursor->nextPart = NULL;
        }
        addBlockAtLevel(lvl + 1, nextLevelOffset);
    } else {
        // Else just do insertion
        buddyBlock* cursor = levelHead;
        buddyBlock* prevCursor = NULL;

        buddyBlock* toAdd = malloc(sizeof(buddyBlock));
        toAdd->offset = offset;
        toAdd->nextPart = NULL;

        if (levelHead == NULL) {
            levelHead = toAdd;
        }
        while (cursor != NULL && cursor->offset < offset) {
            prevCursor = cursor;
            cursor = cursor->nextPart;
        }
        // If the new one becomes the head.
        if (prevCursor == NULL) {
            levelHead = toAdd;
            toAdd->nextPart = cursor;
        } else {
            prevCursor->nextPart = toAdd;
            toAdd->nextPart = cursor;
        }
        buddy.A[lvl] = levelHead;
    }

}

buddyBlock* removeBlockAtLevel(unsigned int lvl)
/**********************************************************
 *    This function remove a free partition at hmi.A[lvl]
 *    Perform the "upstream" search if this lvl is empty AND perform
 *    the repeated split from higher level back to this level.
 * 
 * Return NULL if cannot find such partition (i.e. cannot sastify request)
 * Return the Partition Structure if found.
 *********************************************************/
{
    buddyBlock* levelPart = buddy.A[lvl];
    if (levelPart == NULL) {
        return NULL;
    } else {
        buddyBlock* newHead = levelPart->nextPart;
        buddy.A[lvl] = newHead;
        levelPart->nextPart = NULL;
        return levelPart;
    }
}

int buddyBlockMalloc(int size)
/**********************************************************
 * Mimic the normal "malloc()":
 *    Attempt to allocate a piece of free heap of (size) bytes
 *    Return the memory addres of this FREE memory if successful
 *    Return NULL otherwise 
 *********************************************************/
{
    //TODO: Task 2. Implement the allocation using buddy allocator
    int S = log2Ceiling(size);
    int levelSSize = 1;
    levelSSize <<= S;
    int internalFrag = levelSSize - size;
    // printf("IN MYMALLOC, THE INTERNAL FRAG IS: %d\n", internalFrag);

    buddyBlock *levelSPart = removeBlockAtLevel(S);

    if (levelSPart != NULL) {
        buddy.internalFragTotal += internalFrag;
        return levelSPart->offset;
    } else {
        int R = S;
        buddyBlock *levelRPart = removeBlockAtLevel(R);
        // printf("BEFORE THE CHECK THE R VALUE IS %d\n",R);
        while (levelRPart == NULL) {
            R++;
            if (R > buddy.maxIdx && levelRPart == NULL) {
                return -1;
            }
            levelRPart = removeBlockAtLevel(R);
        }
        int K = R - 1; 
        while (K >= S) {
            // printf("I am here! AND THE K VALUE IS %d\n",K);
            buddyBlock *newPart = malloc(sizeof(buddyBlock));
            newPart->nextPart = NULL;
            int size = 1;
            size <<= K;
            newPart->offset = levelRPart->offset + size;
            buddy.A[K] = newPart;
            K--;
        }
        buddy.internalFragTotal += internalFrag;

        return levelRPart->offset;
    }

}

void pageFree(int seqno, int numOfPages){
    if (numOfPages == 0) return;
    freeVirtualMemory(seqno, numOfPages);
    //modify occupied blocks (if necessary, reallocate buddy blocks)
    freeOccupiedBlock(seqno, numOfPages);
    // TODO: NEED TO CHECK WHETHER THE FRAME IS IN RAM OR NOT!!! + modify active/inactive list
    
    // If there are still frames in RAM for given seqno:
    LRU sysLRU = *(getLRU());
    if (sysLRU.LRUMap[seqno] != NULL) {
        pageNode** nodes = sysLRU.LRUMap[seqno];
        int count = 0;
        for (int i = 0; i < 10; i++) {
            if (nodes[i] != NULL) {
                LRUFree(seqno, i);
                count++;
            }
            if (count == numOfPages) break;
        }
        if (count < numOfPages) {
            sysLRU.LRUMap[seqno] = NULL;
            free(nodes);
        }
    }




}

void pageAccess(int seqno, int offset) {
    //Step1: Check whether it is in the VM. Yes then preceed, else return.
    if (virtualMemory[seqno] == NULL) return;
    if (virtualMemory[seqno][offset] == 1) {
        //Step2: Check whether it is in the RAM. (Use occupied Block to check). 
        
        //If Not in RAM, then treated as a page fault: allocate the frame to buddy allocater + modify occupied Block 
        //+ add the frame to LRU inactive tail + (if full) LRU eviction.
        if (occupiedBlocks[seqno] == NULL || occupiedBlocks[seqno]->pageOffsets[offset] == 0) {
            
            if (occupiedBlocks[seqno] != NULL) {
                occupiedBlock* blk = occupiedBlocks[seqno];
                int modifiedSize = blk->actualSize + 1;
                
                // If the required size is larger than allocated, we need to reallocate.
                if (modifiedSize > blk->allocatedSize) {
                    int allocatedAddress = buddyBlockMalloc(modifiedSize);
                    if (allocatedAddress == -1) {
                        printf("Page Access(Page Fault Reallocate): At seqno:%d, allocate offset of pages: %d, the memory is not enough.\n",seqno, offset);
                        return;
                    }
                    blk->actualSize = modifiedSize;
                    blk->address = allocatedAddress;
                    blk->allocatedSize = 1 << log2Ceiling(modifiedSize);
                }
                blk->pageOffsets[offset] = 1;
            }
            else {
                int allocatedAddress = buddyBlockMalloc(1);
                if (allocatedAddress == -1) {
                    printf("Page Access(Page Fault Reallocate): At seqno:%d, allocate offset of pages: %d, the memory is not enough.\n",seqno, offset);
                    return;
                }
                occupiedBlock* newBlk= (occupiedBlock*) malloc(sizeof(occupiedBlock));
                newBlk->actualSize = 1;
                newBlk->allocatedSize = 1;
                newBlk->address = allocatedAddress;
                newBlk->seqno = seqno;
                newBlk->pageOffsets = (int*) malloc(10 * sizeof(int));
                for (int i = 0; i < 10; i++) {
                    if (i == offset) newBlk->pageOffsets[i] = 1;
                    else newBlk->pageOffsets[i] = 0;
                }
                occupiedBlocks[seqno] = newBlk;
            }
            LRUAdd(seqno, offset);

            //Potential BUG: when LRUAdd, one page frame might be evicted. The evicted page frame should also
            //change its occupied block status. Hint: Modify LRUEvict() function, let it automatically change
            //occupied block. (FIXED! See LRUEvict() in LRU.c)
        }
        if (occupiedBlocks[seqno]->pageOffsets[offset] == 1) LRUPromote(seqno, offset); //In RAM, then LRU promotion.
        
    }


    
    

}


void pageAllocate(int seqno, int numOfPages) {
        int allocatedAddress = buddyBlockMalloc(numOfPages);
        if (allocatedAddress == -1) {
            printf("Page Allocate: At seqno:%d, allocate number of pages: %d, the memory is not enough.\n",seqno, numOfPages);
            return;
        }

        int actualSize = numOfPages;
        int allocatedSize = 1 << log2Ceiling(actualSize);
        //record occupied blocks:
        buildOccupiedBlock(seqno,allocatedAddress,actualSize,allocatedSize);
        //allocate VM:
        allocateVirtualMemory(seqno, actualSize);
        //record in LRU list:
        for (int i = 0; i < numOfPages; i++) LRUAdd(seqno, i);
}


void buddyBlockFree(int address, int size)
/**********************************************************
 * Mimic the normal "free()":
 *    Attempt to free a previously allocated memory space
 *********************************************************/
{
    //Implement the de allocation using buddy allocator
    int levelToSearch = log2Ceiling(size);
    
    int levelSize = 1;
    levelSize <<= levelToSearch;
    int internalFrag = levelSize - size;
    
    int offset = address - buddy.base;
    addBlockAtLevel(levelToSearch, offset);
    buddy.internalFragTotal -= internalFrag;
}











