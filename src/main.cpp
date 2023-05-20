
#if 0
#ifndef _WIN32

#include <stdio.h>
#include "include/msp.h"

unsigned char key[] = {'A', 'X', 'C', 'D'};

Node node;

typedef struct e {
    char      k[2];
    uint16_t  v;
} e;

int main(int argc, char* argv[])
{
    char *p;
    uint32 v;
    Node *pn;
    HASHCTL		hash_ctl;
    HTAB* ht;
    bool found = false;

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

    hash_ctl.keysize = 2;
    hash_ctl.entrysize = 4;

    ht = hash_create("dh", 32, &hash_ctl, HASH_ELEM | HASH_BLOBS);
    if(NULL != ht) {
        e he, *hi;
        he.k[0] = 'A', he.k[1] = 'B';
        hi = (e*)hash_search(ht, he.k, HASH_ENTER, &found);
        if(!found) {
            if(NULL != hi) hi->v = 16;
        }
        hi = (e*)hash_search(ht, he.k, HASH_ENTER, &found);

        hash_destroy(ht);
    }

    
    return 0;
}

#endif 
#endif 