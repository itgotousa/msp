#include <stdio.h>
#include "include/msp.h"

unsigned char key[2] = {'A', 'B'};

int main(int argc, char* argv[])
{
    char *p;
    printf("Hello, MSP!\n");

    printf("size of(void*) is %d bytes\n", (int)sizeof(void*));

    if(1 < argc) {
        uint32 h = hash_bytes((unsigned char*)argv[1], 4);
        p = (char*)&h;
        printf("Hash is:%2X%2X%2X%2X!\n", *p, *(p+1), *(p+2), *(p+3));
    }

    return 0;
}