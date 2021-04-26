/*
 * node.h:  Common header file for server & client.
 */

#ifndef _NODE_H
#define _NODE_H

/* Values used in a node's flags field */
/* These flags are INTERNAL and are not saved to disk */
#define	N_ONSTACK	0x1    /* Tag Used for infinite loop protection */
#define	N_CALLEDON	0x2    /* Already been passed to a traverse func */
#define	N_RESOLVED	0x4    /* Have link node-ids been resolved to ptrs? */
#define N_SYSGEN_FN     0x20   /* File name should be maintained by server */
#define N_HIT           0x1000 /* Internal flag for WAIS search */

/* These are "real" flags that are saved to disk */
#define N_FAKE		0x8    /* Created by web_node to keep things happy */
#define N_TEXT          0x10   /* This is a document node */
#define N_IMAGE         0x40   /* The node is an image (GIF) at this time */
#define N_DONT_STAT     0x80   /* Stat the file to update modify date? */
#define N_SERVER_NODE   0x100  /* a link to another server */
#define N_MENU          0x200  /* this represents a menu item - reserved */
#define N_BINARY        0x400  /* this represents a binary item - reserved */
#define N_DONT_INDEX    0x800  /* Don't index this node for full text search */

#define N_TELNETSESSION 0x2000 /* client starts telnet Added Jan 1993 --lam */

/* Don't save the following flags when we save the web */
#define N_NOSAVE        (N_ONSTACK | N_CALLEDON | N_RESOLVED | N_SYSGEN_FN |\
                        N_HIT)


#define	setflag(n, flag)	((n)->nd_flags |= (flag))
#define	clrflag(n, flag)	((n)->nd_flags &= ~(flag))
#define	flagset(n, flag)	((n)->nd_flags & (flag)) /* is the flag set */


#define	text_node(n)		(*n_file(n) && *n_file(n) != ' ')

/* Output codes necessitated by client bugs which crashed when a node's flags 
   field contained values it didn't like.  Thus the default output format 
   zeros out the flags field, while the new and better format sends the flags 
   across.  The format is changed with the 'O' transaction, which supposedly 
   only new clients will do, to request the new format & get the flags.   
   Added 8/12/92 ark */
#define NO_FLAGS_FORMAT          1        /* Zeros out flags field */
#define SEND_FLAGS_FORMAT        2        /* Sends flags unchanged */
#define TELNET_FORMAT            3        /* "Pretty printing" to make 
					     telnet readable */
#define NUM_FORMATS              3
#define DEFAULT_OUTPUT_FORMAT    NO_FLAGS_FORMAT


/*
 * Flags for web_traverse
 */
#define	TRAV_UP		0x1
#define TRAV_DOWN	0x2
#define	TRAV_OUT	(TRAV_UP|TRAV_DOWN)
#define	TRAV_NOREPEAT	0x4

#define	NLINE_MAXLEN	4096
#define	NL_SAVE		1
#define	NL_RESTORE	2
#define NL_RESTORE_OR   3
#define NL_RESTORE_AND  4  

/* Increased NLIST_MAX_LEN from 4096 to 8192 8/3/92 ark */
#define	NLIST_MAX_LEN   8192	/* Max # of nodes in current list */


#endif ndef _NODE_H
