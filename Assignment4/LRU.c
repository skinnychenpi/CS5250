#include "LRU.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

static LRU sysLRU;

LRU* getLRU() {
    return &sysLRU;
}

void setupLRU(int capacity) {
    // Initialize head and tails:
    pageNode* activeHead = (pageNode*) malloc(sizeof(pageNode));
    activeHead->seqno = -1;
    activeHead->offset = -1;
    activeHead->position = 1;
    pageNode* activeTail = (pageNode*) malloc(sizeof(pageNode));
    activeTail->seqno = -1;
    activeTail->offset = -1;
    activeTail->position = 1;

    pageNode* inactiveHead = (pageNode*) malloc(sizeof(pageNode));
    inactiveHead->seqno = -1;
    inactiveHead->offset = -1;
    inactiveHead->position = 0;
    pageNode* inactiveTail = (pageNode*) malloc(sizeof(pageNode));
    inactiveTail->seqno = -1;
    inactiveTail->offset = -1;
    inactiveTail->position = 0;

    activeHead->next = activeTail;
    activeHead->prev = activeTail;
    activeTail->next = activeHead;
    activeTail->prev = activeHead;

    inactiveHead->next = inactiveTail;
    inactiveHead->prev = inactiveTail;
    inactiveTail->next = inactiveHead;
    inactiveTail->prev = inactiveHead;

    //sysLRU setup:
    sysLRU.activeHead = activeHead;
    sysLRU.activeTail = activeTail;
    sysLRU.inactiveHead = inactiveHead;
    sysLRU.inactiveTail = inactiveTail;

    sysLRU.capacity = capacity;
    sysLRU.activeSize = 0;
    sysLRU.inactiveSize = 0;
    
    pageNode*** LRUMap = (pageNode***) malloc(800 * sizeof(pageNode**));
    for (int i = 0; i < 800; i++) {
        LRUMap[i] = NULL;
    }
    sysLRU.LRUMap = LRUMap;
}

void LRUAdd(int seqno, int offset) {
    if (sysLRU.inactiveSize == sysLRU.capacity) {
        LRUEvict(); // When doing evection, the inactive size will be decremented.
    }
    sysLRU.inactiveSize++;
    pageNode*** map = sysLRU.LRUMap;
    pageNode** seqnoMap = map[seqno];
    if (seqnoMap == NULL) {
        //Initialize LRUMap's seqno entry
        map[seqno] = (pageNode**) malloc(10 * sizeof(pageNode*));
        seqnoMap = map[seqno];
        for (int i = 0; i < 10; i++) seqnoMap[i] = NULL;
    }    
    //Initialize page node
    pageNode* node = (pageNode*) malloc(sizeof(pageNode));

    pageNode* prevTail = sysLRU.inactiveTail->prev;

    node->seqno = seqno;
    node->offset = offset;
    node->position = 0;
    
    node->next = sysLRU.inactiveTail;
    prevTail->next = node;
    node->prev = prevTail;
    sysLRU.inactiveTail->prev = node;

    //Insert the pageNode into the LRUMap entry
    seqnoMap[offset] = node;
}


// Promote: page frame from any list(active or inactive), move to the tail of active list.
void LRUPromote(int seqno, int offset) {
    pageNode** seqnoMap = sysLRU.LRUMap[seqno];
    
    // for (int i = 0; i < 10; i++) {
    //     printf("Idx: %d, Seqno: %d\n", i, seqnoMap[i]->seqno);
    // }

    pageNode* node = seqnoMap[offset];

    // printf("I am here!\n");

    if (node == NULL) {
        printf("ERROR in LRU Promotion: Supposed to find the page node in the map but not found!!\n");
        return;
    }
    int position = node->position;
    // If the node is in inactive list:
    if (position == 0) {
        if (sysLRU.activeSize == sysLRU.capacity) LRUDemote();
        sysLRU.inactiveSize--;
        sysLRU.activeSize++;
        node->position = 1;
    } 
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = sysLRU.activeTail;
    node->prev = sysLRU.activeTail->prev;
    sysLRU.activeTail->prev->next = node;
    sysLRU.activeTail->prev = node;
}

// Demote: page frame from active list head to inactive list tail.
void LRUDemote(){
    sysLRU.activeSize--;
    sysLRU.inactiveSize++;
    pageNode* node = sysLRU.activeHead->next;
    node->position = 0;
    sysLRU.activeHead->next = node->next;
    node->next->prev = sysLRU.activeHead;
    sysLRU.inactiveTail->prev->next = node;
    node->prev = sysLRU.inactiveTail->prev;
    node->next = sysLRU.inactiveTail;
    sysLRU.inactiveTail->prev = node;
}

// Free: when free is called, remove the page node from list(inactive or active).
void LRUFree(int seqno, int offset){
    pageNode* node = sysLRU.LRUMap[seqno][offset];
    if (node == NULL) {
        printf("ERROR in LRU Free: Supposed to find the page node in the map but not found!!\n");
        return;
    }
    int position = node->position;
    if (position == 0) sysLRU.inactiveSize--;
    else if (position == 1) sysLRU.activeSize--;

    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;
    sysLRU.LRUMap[seqno][offset] = NULL;
    free(node);
}

void LRUEvict(){
    pageNode* node = sysLRU.inactiveHead->next;
    sysLRU.inactiveSize--;
    sysLRU.inactiveHead->next = node->next;
    node->next->prev = sysLRU.inactiveHead;
    node->next = NULL;
    node->prev = NULL;

    sysLRU.LRUMap[node->seqno][node->offset] = NULL;
    freeOccupiedBlockOnEvict(node->seqno, node->offset);
    free(node);
}

void printLRU() {
    printf("\nCapacity: %d", sysLRU.capacity);
    printf("\nInactive List:\n");
    printf("======================\n");
    printf("Inactive list size: %d\n", sysLRU.inactiveSize);
    pageNode* node = sysLRU.inactiveHead->next;
    int count = 0;

    while (node != sysLRU.inactiveTail) {
        if (count % 4 == 0){
            printf("[+%5d] |", count);
        }
        printf("(seq = %d, ",node->seqno);
        printf("offset = %d) ;",node->offset);

        if ((count+1) % 4 == 0){
            printf("\n");
        }
        node = node->next;
        count++;
    }

    printf("\nActive List:\n");
    printf("======================\n");
    printf("Active list size: %d\n", sysLRU.activeSize);
    node = sysLRU.activeHead->next;
    count = 0;

    while (node != sysLRU.activeTail) {
        if (count % 4 == 0){
            printf("[+%5d] |", count);
        }
        printf("(seq = %d, ",node->seqno);
        printf("offset = %d); ",node->offset);

        if ((count+1) % 4 == 0){
            printf("\n");
        }
        node = node->next;
        count++;
    }

    printf("\n");

}


// int main (int argc, char** argv){
//     setupLRU(10);
//     // A 0 10
//     for (int i = 0; i < 10; i++) {
//         LRUAdd(0, i);
//     }

//     LRUPromote(0,0);
//     LRUPromote(0,1);
//     LRUPromote(0,5);
//     LRUPromote(0,1);

//     for (int i = 0; i < 3; i++) {
//         LRUAdd(1, i);
//     }


//     LRUAdd(2,0);

//     printLRU();

//     LRUPromote(0,3);
//     LRUPromote(0,4);
//     LRUPromote(0,6);
//     LRUPromote(0,7);
//     LRUPromote(0,8);
//     LRUPromote(2,0);
//     LRUPromote(1,2);
//     LRUPromote(1,0);
//     LRUPromote(0,5);
//     LRUPromote(0,1);

//     printLRU();

//     LRUFree(1,1);
//     LRUFree(0,9);
//     LRUFree(0,0);
//     LRUFree(0,9);
//     LRUFree(0,1);

//     printLRU();
//     return 0;
// }