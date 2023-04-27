#include <stdio.h>
#include <intrin.h>
#include "include/msp.h"

unsigned char key[] = {'A', 'X', 'C', 'D'};

Node node;

int main(int argc, char* argv[])
{
    char *p;
    uint32 v;
    Node *pn;


    printf("size of(void*) is %d bytes\n", (int)sizeof(void*));

    uint32 h = hash_bytes((unsigned char*)key, 4);
    p = (char*)&h;
    printf("Hash is:%8X\n", h);
    v = 0x40000000;
    h = __lzcnt(v);

    printf("%8X has %d zeros\n", v, h);

    pn = &node;
    pn->type = T_AllocSetContext;
    if(IsA(pn, AllocSetContext)) {
        printf("Yes, it is AllocSetContext\n");
    }
    else printf("No, it is not AllocSetContext\n");
    
    ereport(ERROR,
            (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg("out of memory"),
                errdetail("Failed while creating memory context \"%s\".",
                        name)));

    return 0;
}