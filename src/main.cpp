#include <stdio.h>
#include <intrin.h>
#include "include/msp.h"

unsigned char key[] = {'A', 'X', 'C', 'D'};

int main(int argc, char* argv[])
{
    char *p;
    uint32 v;


    printf("size of(void*) is %d bytes\n", (int)sizeof(void*));

    uint32 h = hash_bytes((unsigned char*)key, 4);
    p = (char*)&h;
    printf("Hash is:%8X\n", h);
    v = 0x40000000;
    h = __lzcnt(v);

    printf("%8X has %d zeros\n", v, h);

    return 0;
}