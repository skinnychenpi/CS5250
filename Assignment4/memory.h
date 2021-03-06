#ifndef __MEMORY_H_	//standard protection against multiple includes
#define __MEMORY_H_


#define FREE 0
#define OCCUPIED 1


typedef struct BUDDYBLOCK {
    unsigned int offset;
	struct BUDDYBLOCK *nextPart;
} buddyBlock;

typedef struct {
	int totalSize;
    int base;
	buddyBlock** A; //The array of partition lists in buddy system
    int maxIdx;   //For convenience. Keep the max index for A[].
    int internalFragTotal;  //Keep track of internal fragmentation.
} buddyAllocator;

typedef struct {
    int address;
    int allocatedSize;
    int actualSize;
    
    // page info:
    int seqno;
    int* pageOffsets;
} occupiedBlock;






/**********************************************************
 * Do NOT modify any of the following functions as they 
 *   are used by the test drivers.
 *********************************************************/
int setupBuddy(int);
int setupOccupiedBlocks();
int setupVirtualMemory();
void setupLRU();


void printBuddyAllocatorMetaInfo();
void printBuddyAllocatorStatistic();
void printOccupiedBlocks();
void printVirtualMemory();

int buddyBlockMalloc(int);

void pageAllocate(int, int);
void pageFree(int, int);
void pageAccess(int, int);
//For simplicity, the free() takes in the size of partition
void buddyBlockFree(int, int);
void freeOccupiedBlockOnEvict(int seqno, int offset);

#endif
