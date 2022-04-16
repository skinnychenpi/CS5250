#ifndef __LRU_H_	//standard protection against multiple includes
#define __LRU_H_

typedef struct PAGENODE{
    int seqno;
    int offset;
    struct PAGENODE* prev;
    struct PAGENODE* next;
    int position; // 0 means in inactive list, 1 means in active list.
} pageNode;


typedef struct {
    pageNode* inactiveHead;
    pageNode* activeHead;
    pageNode* inactiveTail;
    pageNode* activeTail;
    int capacity;
    int inactiveSize;
    int activeSize;
    pageNode*** LRUMap; //Mimic HashMap using array. LRUMap[seqno][pageOffset] returns pageNode*
} LRU; 

void setupLRU(int capacity);

void LRUAdd(int seqno, int offset);
void LRUPromote(int seqno, int offset);
void LRUEvict();
void LRUFree(int seqno, int offset);
void LRUDemote();
LRU* getLRU();
void printLRU();
#endif