/*
 * a header for Techinfo network stuff
 */
/* The top half of this file will need to be modified before compiling the 
   server for a new installation. Also there are other file locations 
   in glob.c */
/* 
 * NOTE:  By default clients do NOT see the a node's flags; in the default
 *  format a zero is sent in place of the flags.  To receive the flags, a 
 *  client must use the 'O' transaction (see below).
 */

#define PIPS_PORT	9000
#define PORT            "9000"
#define PIPS_SERVER     "penninfo-srv.upenn.edu"

#define SERVER_UID       20061 
#define TI_VERSION      "UNIX:3.1"
#define HELP_PHONE      "(215) 898-2728"
#define HELP_ID          10210



#define BANNER_MSG  101
#define BANNER     "0:TechInpherServer, version 1.0  .\n"
#define NAK_BANNER     "100: The TechInpher Server is temporarily unavailable, please try again later.\n"
#ifndef TEST
/* production locations */
#define  TRANS_LOG	"/var/ti/logs/pips.trans.log"
#define  NEXTID_FILE	"/var/ti/admin/pips.next.id"
#define  PROV_FILE	"/var/ti/admin/pips.providers"
#define  VER_FILE	"/var/ti/admin/pips.versions"
#define  LOCAL_WEB_FILE "/var/ti/admin/pips.web"
#define  PROV_LN_SZ	45
#define  ADMIN_FILE	"/var/ti/admin/pips.admin"
#define  ADMIN_LN_SZ	80
#define  SERVER_FILE       "/var/ti/admin/pips.servers"
#define  SOURCE_FILE       "/var/ti/admin/pips.sources"
#define  SOURCE_FILE_BACKUP       "/var/ti/admin/pips.sources.bak"
#define  TMP_SOURCE_FILE   "/var/ti/admin/pips.sources.tmp"
#define	PIPS_LOCKER	"ti_data"
#else
/* test locations */
#define  TRANS_LOG	"pips.trans.log"
#define  NEXTID_FILE	"pips.next.id"
#define  PROV_FILE	"pips.providers"
#define  VER_FILE	"pips.versions"
#define  LOCAL_WEB_FILE "pips.web"
#define  PROV_LN_SZ	45
#define  ADMIN_FILE	"pips.admin"
#define  ADMIN_LN_SZ	80
#define  SERVER_FILE       "pips.servers"
#define  SOURCE_FILE       "pips.sources"
#define  SOURCE_FILE_BACKUP     "pips.sources.bak"
#define  TMP_SOURCE_FILE   "pips.sources.tmp"
#define	 PIPS_LOCKER	"tidata"
#endif

/*++++++++  BELOW HERE SHOULD NOT NEED MODIFING +++++++++++++++++++*/

#define	CRLF		"\015\012"
#define	EOL		"\012"		/* End of Line */
#define	EOL_LEN		1
#define EOM		".\015\012"		/* End of Message */
#define EOM_LEN		3
#define EOLM		"\012.\015\012"	/* EOL + EOM */
#define EOLM_LEN	(EOL_LEN + EOM_LEN)

#define  MAX_TRANS      500   /* maximum # of trans per session */
#define  SRV_LN_SZ      256
#define  SRC_LN_SZ      256

#define  ADV_HELP_ID    22558     /*  old way -- not used */
#define  HELP_MENU_ID   8041     /*  THIS is the one! */

#define  NOBODY  -1

typedef struct pending_sends TOSEND;
typedef struct pending_recvs TOREC;
typedef struct pending_helper WAITFORHELPER;

struct pending_helper
{
  long pid;               /* id of child process */
  char *file;             /* name of child's output file */

  /*
    nodeid and gophtype needed because we will move the translated results
    to cachedir/nodeid.gophtype.
    nodeid is also needed because it is the first node when an nlist
    is sent back to the client.
    gophtype is also needed because we need to know HOW to interpret
    the results in the child's output file.
    */
  struct s1 *node;            /* nodeid and gophtype for this send */

  /* needed after child exits to decide what to send client */
  long startpoint;        /* what byte to start at --for documents */
  long requested;         /* how much to send --for documents */

  /* child needs to know where to put the converted results */
  char *cachefile;
};


struct pending_sends
{
  char *buf;  /* buffer to free when through */
  char *ptr;  /* pointer to start sending from */
  int  size;  /* how much left to send */
  int  sock;  /* socket to send to */
};

struct pending_recvs
{
  char *filename; /* the file name of the file being recvd */
  int  the_fd;    /* the file desc of the open file */
  int  sock;      /* the socket to receive from */
};

typedef struct connection	CONN;

struct connection {
	int	c_socket;
	char	*c_hostname;
	int	c_portnum;
	int	c_flags;
	char    *c_uid;
        char    *c_type;
	time_t	c_made;
	time_t	c_last;
	TOSEND  *c_ptr;
	TOREC   *c_recptr;
	WAITFORHELPER *c_hptr;     /* Added Jan 1993 LAM */
	int     c_output_fmt;      /* Added 8/12/92 ark */
	int     c_trans_cnt;      /* added 9/118/92 ST */
};

#define C_FULLFMT	0x1	/* Using full format output */
#define C_BUSY		0x2	/* connection is already being serviced */
#define C_TIMEOUT	0x4	/* Connection has timed out */
#define C_PROVIDER	0x8	/* Connection is a provider */

/* Added 1/93 LAM:
  c_childpid = process id of child getting Gopher data.
  c_gophertype = gopher file type -- so parent knows what 
      output format to expect from its child.
  c_state tells whether child is done getting gopher data.
  */

#define MAX_TRANS_FIELDS 31  /* number of DLM separated fields in transaction*/


/*
 * Transaction codes.
 */

#define	T_ADDNODE	  'a'	/* add */
#define T_FIND            'b'	/* fnd, find */
#define	T_ENDPROVIDER	  'c'	/* epv, */
#define	T_DUMPWEB	  'd'	/* dmp, dump */
#define T_EXPAND          'e'	/* exp, expand */
#define	T_GETFILE	  'f'	/* fil, file, getfile  */
#define T_REORDER_BEFORE  'g'   /* reorder the child links */
#define T_SOURCE          'h'   /* test of valid source for an id */
#define T_RELOAD          'i'   /* reload the web from disk */
#define T_REORDER_AFTER   'j'   /* reorder the child links */
#define T_FINDKEY         'k'   /* find keyword */
#define	T_ADDLINK	  'l'	/* lnk, link */
#define	T_GET_SERVER_INFO 'm'   /* get info on other servers */
#define T_SRC_INFO        'n'   /* get full source info on a node */
#define	T_OUTPUTFMT	  'o'	/* fmt -- undocumented */
#define	T_TRYPROVIDER	  'p'	/* prv, provider */
#define T_QUIT            'q'   /* quit */
#define	T_REPLACENODE	  'r'	/* rpl, replace */
#define	T_SENDNODE	  's'	/* get, show? node? */
#define T_SENDFILE        't'	/* sfl, sendfile */
#define	T_RMLINK	  'u'	/* ulk, unlink */ 
#define T_VERSION         'v'   /* find current version */
#define T_TRAVERSE        'w'	/* trv, web? trav, traverse */
#define	T_RMNODE	  'x'	/* del, delete */
#define T_SAVEWEB         'y'   /* save the web to disk */
#define T_ADMIN           'z'	/* adm, admin */
#define T_CHG_SRC_INFO    'A'   /* change the info about a source 
				   format is A:<source_info_line> */
#define T_SHOW_CONN       'B'	/* show the current connections, an admin cmd*/
#define T_SHUTDOWN        'C'	/* shutdown the server, save web , dont 
				   accept connections, and shutdown when 
				   connections left have been inactive
				   for n minutes */
#define T_CHG_BANNER      'D'	/* change the banner msg */
#define T_SET_DATES       'E'	/* stat all files except those flagged
				 and set the last modified date (admin cmd)*/
#define T_HELP            'H'   /* send the help menu */
#define T_CHGD_SINCE      'I'   /* send the non-menu nodes which have changed
				 since a certain date*/
#define T_FULL_TXT_SEARCH 'J'   /* Perform full-text WAIS search */
#define T_SOURCE_SRCH     'K'   /* Return all nodes of a given source*/
#define T_NODE_FORMAT     'O'   /* Choose nodelist format */
#define T_TITLE_SRCH      'T'   /* return nodes based on title */

/*
 * For/From netio.c
 */

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


