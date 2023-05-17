#include <stdbool.h>
/*
 * Check for validity of a single UTF-8 encoded character
 *
 * This directly implements the rules in RFC3629.  The bizarre-looking
 * restrictions on the second byte are meant to ensure that there isn't
 * more than one encoding of a given Unicode character point; that is,
 * you may not use a longer-than-necessary byte sequence with high order
 * zero bits to represent a character that would fit in fewer bytes.
 * To do otherwise is to create security hazards (eg, create an apparent
 * non-ASCII character that decodes to plain ASCII).
 *
 * length is assumed to have been obtained by pg_utf_mblen(), and the
 * caller must have checked that that many bytes are present in the buffer.
 */
bool
pg_utf8_islegal(const unsigned char *source, int length)
{
	unsigned char a;

	switch (length)
	{
		default:
			/* reject lengths 5 and 6 for now */
			return false;
		case 4:
			a = source[3];
			if (a < 0x80 || a > 0xBF)
				return false;
			/* FALL THRU */
		case 3:
			a = source[2];
			if (a < 0x80 || a > 0xBF)
				return false;
			/* FALL THRU */
		case 2:
			a = source[1];
			switch (*source)
			{
				case 0xE0:
					if (a < 0xA0 || a > 0xBF)
						return false;
					break;
				case 0xED:
					if (a < 0x80 || a > 0x9F)
						return false;
					break;
				case 0xF0:
					if (a < 0x90 || a > 0xBF)
						return false;
					break;
				case 0xF4:
					if (a < 0x80 || a > 0x8F)
						return false;
					break;
				default:
					if (a < 0x80 || a > 0xBF)
						return false;
					break;
			}
			/* FALL THRU */
		case 1:
			a = *source;
			if (a >= 0x80 && a < 0xC2)
				return false;
			if (a > 0xF4)
				return false;
			break;
	}
	return true;
}
