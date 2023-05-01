#ifndef __SVGNODES_H__
#define __SVGNODES_H__

#include "c.h"
#include "nodes.h"

typedef struct SVGText
{
    NodeTag     type;
    Node        *sibling;
    Node        *child;
    char        *data;
} SVGHeading;

/*
 *  # H1_title
 *  ## H2_title
 *  ### H3_title
 *  #### H4_title
 *  ##### H5_tile
 *  ###### H6_title
 */
typedef struct SVGHeading
{
    NodeTag     type;
    Node        *sibling;
    Node        *child;
    uint8_t     level;
    char        *data;
} SVGHeading;

typedef struct SVGList
{
    NodeTag     type;
    Node        *sibling;
    Node        *child;
    char        *data;
} SVGList;

typedef struct SVGCode
{
    NodeTag     type;
    Node        *sibling;
    Node        *child;
    char        *data;
} SVGCode;

typedef struct SVGPath
{
    NodeTag     type;
    Node        *sibling;
    Node        *child;
    char        *url;
    uint32_t    length;
    char        *title;
} SVGPath;

typedef struct SVGDocument
{
    NodeTag     type;
    Node        *child;
} SVGDocument;


#endif /* __SVGNODES_H__ */
