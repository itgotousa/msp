#ifndef __NODES_H__
#define __NODES_H__


/*
 * The first field of every node is NodeTag. Each node created (with makeNode)
 * will have one of the following tags as the value of its first field.
 *
 * Note that inserting or deleting node types changes the numbers of other
 * node types later in the list.  This is no problem during development, since
 * the node numbers are never stored on disk.  But don't do it in a released
 * branch, because that would represent an ABI break for extensions.
 */

typedef enum NodeTag
{
    T_Invalid = 0,
    T_AllocSetContext
} NodeTag;


#endif /* __NODES_H__ */