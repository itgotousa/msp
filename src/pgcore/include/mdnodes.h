#ifndef __MDNODES_H__
#define __MDNODES_H__

#include "c.h"
#include "nodes.h"

typedef struct MDScheme
{
    int     top_margin;
    int     bottom_margin;
    int     width;
} MDScheme;

typedef struct MDText
{
    NodeTag     type;
    Node        *sibling;
    Node        *child;
    char        *data;
} MDHeading;

/*
 *  # H1_title
 *  ## H2_title
 *  ### H3_title
 *  #### H4_title
 *  ##### H5_tile
 *  ###### H6_title
 */
typedef struct MDHeading
{
    NodeTag     type;
    Node        *sibling;
    Node        *child;
    uint8_t     level;
    char        *data;
} MDHeading;

typedef struct MDList
{
    NodeTag     type;
    Node        *sibling;
    Node        *child;
    char        *data;
} MDList;

typedef struct MDCode
{
    NodeTag     type;
    Node        *sibling;
    Node        *child;
    char        *data;
} MDCode;

typedef struct MDLink
{
    NodeTag     type;
    Node        *sibling;
    Node        *child;
    char        *url;
    uint32_t    length;
    char        *title;
} MDLink;

typedef struct MDDocument
{
    NodeTag     type;
    Node        *child;
    MDScheme    *scheme;
} MDDocument;


#endif /* __MDNODES_H__ */
