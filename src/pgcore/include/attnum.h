#ifndef __ATTNUM_H__
#define __ATTNUM_H__

#include "c.h"

/*
 * user defined attribute numbers start at 1.   -ay 2/95
 */
typedef int16 AttrNumber;

#define InvalidAttrNumber		0
#define MaxAttrNumber			32767

/* ----------------
 *		support macros
 * ----------------
 */
/*
 * AttributeNumberIsValid
 *		True iff the attribute number is valid.
 */
#define AttributeNumberIsValid(attributeNumber) \
	((bool) ((attributeNumber) != InvalidAttrNumber))

/*
 * AttrNumberIsForUserDefinedAttr
 *		True iff the attribute number corresponds to a user defined attribute.
 */
#define AttrNumberIsForUserDefinedAttr(attributeNumber) \
	((bool) ((attributeNumber) > 0))

/*
 * AttrNumberGetAttrOffset
 *		Returns the attribute offset for an attribute number.
 *
 * Note:
 *		Assumes the attribute number is for a user defined attribute.
 */
#define AttrNumberGetAttrOffset(attNum) \
( \
	AssertMacro(AttrNumberIsForUserDefinedAttr(attNum)), \
	((attNum) - 1) \
)

/*
 * AttrOffsetGetAttrNumber
 *		Returns the attribute number for an attribute offset.
 */
#define AttrOffsetGetAttrNumber(attributeOffset) \
	 ((AttrNumber) (1 + (attributeOffset)))


#endif /* __ATTNUM_H__ */