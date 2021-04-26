/*
  This software is copyrighted by the University of Pennsylvania.
  Read COPYRIGHT for details.
  */

#define HELPER                   "techinpher-helper"
#define CACHEDIR                 "cache/"
#define CACHETIMEOUT             1200  /* 20 min in seconds */
#define GW_SOURCENAME            "gateway"
#define GW_LONGSRCNAME           "PennInfo ==> Gopher Gateway"
#define GW_EMAILADDR             "penninfo-gopher@dccs.upenn.edu"

#define GW_NODE_UNUSED(lastu)    ( (todaysdate-lastu) > 5 )

/* reserved nodes (see nodes.reserved) */
#define MAINMENUNODE                  0
#define DUMMY_NODE                    2
#define HELP_MENU_NODE               10

#define GOPHERFILETYPES_UNAVAILABLE  20
#define CLIENT_NEEDS_TELNET          21
#define UNKNOWN_DOC_TYPE             22
#define FILE_NOT_SEARCHABLE          23
#define NO_SHOW_PATH                 24
#define NO_OUTLINE                   25
#define TRY_SEARCHCMD                26
#define FILE_NOT_AMENU               27
#define MENU_NOT_AVAILABLE           28
#define GLOBGOPH_SRCH_NODE         1000 /* Used for veronica */

#define FIRST_UNRESERVED_NODE      1000
#define MAX_RESV_NODES             FIRST_UNRESERVED_NODE
#define is_reserved_node(nid)      (nid < FIRST_UNRESERVED_NODE)
#define NUMFIELDS_RESVNODES        4

/* Which node to use for global gopher search -- this nodeid
   must appear in gw_nodes_file */

#define NUMFIELDS_GOPHER       4 /* title,path,server,port */
#define NUMFIELDS_GOPHNODES    3 /* nodeid,lastu,gopherinfo */

struct s1 {
  char gophertype;
  char *gophertitle;
  char *gopherserver;
  char *gopherport;
  char *gopherpath;
  long nodeid;
  short lastused;    /* last date item was used: either it was
			used in a menu, OR selected (i.e. w:2:N:1, t:N) */
  struct s1 *nextnode;
};

/* lastused, count, initcount should be used to decide which
   gopher nodes to delete from the datastructures.  */

struct s2 {
  long nodeid;
  struct s1 *node;
  struct s2 *nextnode;
};

struct resvnode {
  long nodeid;
  short numchildren;
  long *children;
  char *title;
  char *file;
};

struct gopherabbrev  {
  char gophtyp;
  char *gophabbrev;
};

#define GOPHER_DLM        '\t'
/* Got the following types from gopher v1.11, Jan 1993 */
#define GOPHTYP_TEXT      '0'
#define GOPHTYP_MENU      '1'
#define GOPHTYP_CSO       '2'
#define GOPHTYP_ERROR     '3'
#define GOPHTYP_MACHQX    '4'
#define GOPHTYP_DOSBIN    '5'
#define GOPHTYP_UUENC     '6'
#define GOPHTYP_SEARCH    '7'
#define GOPHTYP_TELNET    '8'
#define GOPHTYP_BINARY    '9'
#define GOPHTYP_SOUND     's'
#define GOPHTYP_EVENT     'e'
#define GOPHTYP_CALENDAR  'c'
#define GOPHTYP_GIF       'g'
#define GOPHTYP_HTML      'h'
#define GOPHTYP_TN3270    'T'
#define GOPHTYP_MIME      'M'
#define GOPHTYP_IMAGE     'I'

#define GOPHTYP_DUPSRV  '+'


