
#include "include/pgcore.h"

/*
 * Support for newNode() macro
 *
 * In a GCC build there is no need for the global variable newNodeMacroHolder.
 * However, we create it anyway, to support the case of a non-GCC-built
 * loadable module being loaded into a GCC-built backend.
 */

Node	   *newNodeMacroHolder;
