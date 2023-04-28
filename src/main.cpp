#include <stdio.h>
#include "include/msp.h"

unsigned char key[] = {'A', 'X', 'C', 'D'};

Node node;

int main(int argc, char* argv[])
{
    char *p;
    uint32 v;
    Node *pn;
    HASHCTL		hash_ctl;

    pn = &node;
    pn->type = T_AllocSetContext;
    if(IsA(pn, AllocSetContext)) {
        printf("Yes, it is AllocSetContext\n");
    }
    else printf("No, it is not AllocSetContext\n");
   
    MemoryContextInit();

    printf("memory pool is read!\n");

    p = (char*)palloc(13);
    if(NULL != p) {
        pfree(p);
    }

    p = (char*)palloc(15);
    if(NULL != p) {
        pfree(p);
    }

    hash_ctl.keysize = sizeof(Oid);
    hash_ctl.entrysize = sizeof(Oid);

    hash_create("Uncommitted enums",
        32,
        &hash_ctl,
        HASH_ELEM | HASH_BLOBS);

    return 0;
}