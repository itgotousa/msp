#ifndef __ELOG_H__
#define __ELOG_H__

/* Error level codes */
#define DEBUG5		10			/* Debugging messages, in categories of
								 * decreasing detail. */
#define DEBUG4		11
#define DEBUG3		12
#define DEBUG2		13
#define DEBUG1		14			/* used by GUC debug_* variables */
#define LOG			15			/* Server operational messages; sent only to
								 * server log by default. */
#define LOG_SERVER_ONLY 16		/* Same as LOG for server reporting, but never
								 * sent to client. */
#define COMMERROR	LOG_SERVER_ONLY /* Client communication problems; same as
									 * LOG for server reporting, but never
									 * sent to client. */
#define INFO		17			/* Messages specifically requested by user (eg
								 * VACUUM VERBOSE output); always sent to
								 * client regardless of client_min_messages,
								 * but by default not sent to server log. */
#define NOTICE		18			/* Helpful messages to users about query
								 * operation; sent to client and not to server
								 * log by default. */
#define WARNING		19			/* Warnings.  NOTICE is for expected messages
								 * like implicit sequence creation by SERIAL.
								 * WARNING is for unexpected messages. */
#define PGWARNING	19			/* Must equal WARNING; see NOTE below. */
#define WARNING_CLIENT_ONLY	20	/* Warnings to be sent to client as usual, but
								 * never to the server log. */
#define ERROR		21			/* user error - abort transaction; return to
								 * known state */
#define PGERROR		21			/* Must equal ERROR; see NOTE below. */
#define FATAL		22			/* fatal error - abort process */
#define PANIC		23			/* take down the other backends with me */

#define elog(elevel, ...)  do {} while(0)
#define ereport(elevel, ...)  do {} while(0)

#endif  /* __ELOG_H__ */
