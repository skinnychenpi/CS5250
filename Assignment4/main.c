#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include <sys/stat.h>
#include <string.h>
#include "LRU.h"



int main(int argc, char** argv)
{

    char* fileName = "A0229929L-assign4-input.dat";
    // char* fileName = "test.dat";
    FILE * fp = fopen(fileName, "rb");

    struct stat sb;
    stat(fileName, &sb);

    char *file_contents = malloc(sb.st_size);
    char* seqnoStr = malloc(3 * sizeof(char));
    char* numStr = malloc(2 * sizeof(char));
    char* cmd = malloc(sizeof(char));

    setupBuddy(512);
    setupOccupiedBlocks();
    setupVirtualMemory();
    setupLRU(250);



    // pageAllocate(0,10);
    // pageAllocate(1,8);

    // printBuddyAllocatorMetaInfo();
    // printBuddyAllocatorStatistic();
    // printOccupiedBlocks();

    // pageFree(0,2);

    // printBuddyAllocatorMetaInfo();
    // printBuddyAllocatorStatistic();
    // printOccupiedBlocks();

    
    while (fscanf(fp, "%[^\n] ", file_contents) != EOF) {
        *cmd = file_contents[0];
        printf("cmd>%c\n",*cmd);

        int i = 2;
        int j = 0;
        
        while (file_contents[i] - '0' != -39 && i < 5) {
            seqnoStr[j++] = file_contents[i++];
        }
        while (file_contents[i] - '0' == -39) i++;
        j = 0;

        while (file_contents[i] - '0' != -48 && i < 9) {
            numStr[j++] = file_contents[i++];            
        }

        int seqno = atoi(seqnoStr);
        int num = atoi(numStr);
        printf("seqno>%d\n",seqno);
        printf("num>%d\n\n",num);

        seqnoStr[1] = '\0';
        seqnoStr[2] = '\0';
        numStr[1] = '\0';

        if (*cmd == 'A') pageAllocate(seqno, num);
        else if (*cmd == 'F') pageFree(seqno, num);
        else if (*cmd == 'X') pageAccess(seqno, num);
 
    }
    
    printBuddyAllocatorMetaInfo();
    printBuddyAllocatorStatistic();
    printOccupiedBlocks();
    printVirtualMemory();
    printLRU();

    fclose(fp);
    return 0;
}