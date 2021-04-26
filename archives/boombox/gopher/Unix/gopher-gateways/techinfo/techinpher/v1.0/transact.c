/*
  This software is copyrighted by the University of Pennsylvania.
  Read COPYRIGHT for details.

  */

/*
    Assume mostly the same strategy as the TechInfo server
    and put all data into a buffer for the connection.


    However, if the transaction requires connecting to gopher, then
    fork a child, which will exec a new program with the right
    argv[], and parent will create c_helper for the connection.
    When the server receives the signal that
    the child is done, it will read the child's output file,
    translate it and write a TechInfo formatted file and call
    nio_send_file().  Then remove_helper for the connection.

    If a file containing the gopher information is already
    cached and isn't too old, use it instead of connecting to gopher.

    This is basically a complete rewrite of MIT's transact.c code.
    I *really* disliked the extensive use of variables whose scope
    was the entire module.  I like to pass parameter lists, even if
    they get cumbersome.  Less confusion in the long run about
    what gets used, what gets changed.
    
    */

#include <stdio.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <strings.h>
#include <sys/ioctl.h> /**/
#include <signal.h>

#include "gophernodes.h"  /* new for techinpher gateway */
#include "network.h"
#include "pdb.h"
#include "messages.h"
#include "node.h"


void admin(int numf, char **fields, int sock);
void change_format(int *fmt, int sock, int numf, char **fields);
void find(CONN *conn, char ch, int numf, char **fields );
char *find_abbrev(char ch);
void find_ver(CONN *conn, int numf, char **fields);
int hdl_transact(CONN *conn);
void log_trans(char *line);
char *nodeinfostr (long nodeid, struct s1 *nd, int flgsfmt);
void proc_trans(int numf, char ch, char **fields, CONN *conn);
void send_connections(int sock);
void send_empty_document(int sock);
void send_empty_menu(int sock);
void send_file(CONN *conn, char ttype, int numf, char **fields);
void send_gopher_info(struct s1 *nd, int numf, char **fields, CONN *currcon);
void send_help(int sock);
void send_node(int flgsfmt, int sock, int numf, char **transfields);
int send_reserved_node (char trans_type, int numf, char **fields, long nodeid, int flgsfmt, int cur_sock);
void send_src_info(int sock);
void send_servers_list(int sock);
int test_admin(char *passwd);
void traverse(CONN *conn, char ch, int numf, char **fields);


extern char     *msglist[]; /* the list of error messages */
extern int      debug;
extern short    todaysdate;
extern int      tables_changed;
extern int      lastused_changed;
extern void     parsefields();
extern struct s1 *getnode_bynodeid(long nid);
extern void     catch_child();
extern void     nio_send_file(int s, char *fn, long stpt, long req, int isnlist);
extern void     send_msg (char *string, int sock);
extern int      sendbuf  (char *cp, int sz, int sock);

int             max_stale_time;  /* not used, but it's in server.c */
static char     *curr_uid;  /* should get rid of this one too */
static char     in_buff[BUFSIZ];  /* holds transaction from client */
static char     nodeinfobuf[BUFSIZ];



struct gopherabbrev gopher_abbrev[] =
{                  /* leaving out ERROR and DUPSRV */
  {GOPHTYP_TEXT,        "."},
  {GOPHTYP_MENU,        "/"},
  {GOPHTYP_CSO,         " <CSO>"},
  {GOPHTYP_MACHQX,      " <HQX>"},
  {GOPHTYP_DOSBIN,      " <PC Bin>"},
  {GOPHTYP_UUENC,       " <UUENC>"},
  {GOPHTYP_SEARCH,      " <?>"},
  {GOPHTYP_TELNET,      " <TEL>"},
  {GOPHTYP_BINARY,      " <Bin>"},
  {GOPHTYP_GIF,         " <Picture>"},
  {GOPHTYP_IMAGE,       " <Picture>"},
  {GOPHTYP_TN3270,      " <3270>"},
  {GOPHTYP_SOUND,       " <)"},
  {GOPHTYP_EVENT,       " <Event>"},
  {GOPHTYP_CALENDAR,    " <Cal>"},
  {GOPHTYP_MIME,        " <MIME>"},
  {GOPHTYP_HTML,        " <HTML>"}
};
#define NUM_GOPHER_ABBREV 17
#define gopher_abbrev_unk  " <Unknown>"


int hdl_transact(CONN *conn)
{
	char            log_line[BUFSIZ];
	int             length;
	int             f;
	int             cur_sock;
	char            *transfields[MAX_TRANS_FIELDS];
	int             num_transfields;
	char            trans_type;

	curr_uid = conn->c_uid;
	conn->c_trans_cnt++;
	cur_sock = conn->c_socket;

	bzero(in_buff, BUFSIZ);
	length = read(cur_sock, in_buff, BUFSIZ);
	if (length <= 0) {
	  if (length)
	    perror("socket read");
	  /*                if (errno == EWOULDBLOCK)
			    return 0;
			    else 
	   This caused looping error -ST */
	  return 1;
	}
	if (in_buff[length - 1] == LF)
		length--;
	if (in_buff[length - 1] == CR)
		length--;
	in_buff[length] = '\0';
	
	trans_type = *in_buff;
	parsefields(in_buff, transfields, &num_transfields, DLM, MAX_TRANS_FIELDS-1);
	
	if (length == 0 || trans_type == T_QUIT)
	  return 1;	/* quit and close connection */
	
	if (trans_type == T_ADMIN)
	  sprintf(log_line, "%s:%d:%c", curr_uid, cur_sock, trans_type);
	else if (trans_type == T_TRYPROVIDER)
	  sprintf(log_line, "%s:%d:%c:%s", curr_uid,cur_sock,trans_type,
		  num_transfields < 2 ? "" : transfields[1]);
	else {
	  sprintf (log_line, "%s:%d", curr_uid, cur_sock);
	  for (f=0; f < num_transfields; f++) {
	    strcat (log_line, ":");
	    strcat (log_line, transfields[f]);
	  }
	}
	log_trans(log_line);	/* log the transaction */
	(void) proc_trans (num_transfields, trans_type, transfields, conn);
	return 0;		/* ok */
}



/* Switch on transaction type, calling handler procedure */
static void
  proc_trans(int num_transfields, char trans_type, char **transfields,
	     CONN *conn)
{
  int flgsfmt;
  int cur_sock;

  flgsfmt = conn->c_output_fmt;
  cur_sock = conn->c_socket;

  switch (trans_type)
    {
      /* No provider functions */
    case T_ADDLINK:
    case T_ADDNODE:
    case T_CHG_SRC_INFO:
    case T_GETFILE:
    case T_REORDER_AFTER:	
    case T_REORDER_BEFORE:	
    case T_REPLACENODE:
    case T_RMLINK:
    case T_RMNODE:
    case T_SOURCE:
      send_msg(msglist[ NOT_AUTH ], cur_sock);
      break;

    case T_TRYPROVIDER:
      send_msg(msglist[ BAD_PASSWD ], cur_sock);
      break;
      
    case T_NODE_FORMAT:
      change_format ( &(conn->c_output_fmt), cur_sock, num_transfields, transfields );
      break;
      
    case T_VERSION:	/* find the current version number */
      find_ver(conn, num_transfields, transfields);
      break;
      
    case T_SRC_INFO:
      send_src_info(cur_sock); 
      break;

    case T_SENDFILE:
      send_file(conn, trans_type, num_transfields, transfields);
      break;

    case T_SENDNODE:
      send_node(flgsfmt, cur_sock, num_transfields, transfields);
      break;
      
    case T_TRAVERSE:	/* do a traverse + return list */
      traverse(conn, trans_type, num_transfields, transfields);
      break;
      
    case T_HELP:
      send_reserved_node (trans_type, num_transfields, transfields,
			  HELP_MENU_NODE, flgsfmt, cur_sock);
      break;
      
    case T_FULL_TXT_SEARCH:     /* no difference in Gopher between the two */
    case T_FIND:		/* do a find + return list */
    case T_TITLE_SRCH:
      find(conn, trans_type, num_transfields, transfields);
      break;
      

      /* ADMIN functions */
    case T_ADMIN:
      admin(num_transfields, transfields, cur_sock);
      break;

    case T_ENDPROVIDER:
      if (test_admin(curr_uid)) { /* NOT PROVIDER, but admin */
	strcpy (curr_uid, "");
	send_msg(msglist[ OK ], cur_sock);
      } else
	send_msg(msglist[ NOT_AUTH ], cur_sock);
      break;

    case T_RELOAD: 
      if (test_admin(curr_uid)) {
	datastruct_load(1);      /* 1 == free old stuff first */
	send_msg(msglist[ OK ], cur_sock);
      } else
	send_msg(msglist[ NOT_AUTH ], cur_sock);
      break;		      

    case T_SAVEWEB: /* save the web to disk */ {
      if (test_admin(curr_uid)) {
	save_datastruct();
	send_msg(msglist[ OK ], cur_sock);
      } else
	send_msg(msglist[ NOT_AUTH ], cur_sock);
      break;
    }

    case T_SHOW_CONN:
      send_connections(cur_sock);  /* NOT an admin function */
      break;

    case T_GET_SERVER_INFO:
      send_servers_list(cur_sock);
      break;

      /* Not implementing the following */
    case T_CHG_BANNER:
    case T_SHUTDOWN:
    case T_OUTPUTFMT:
    case T_CHGD_SINCE:
    case T_FINDKEY:
    case T_SET_DATES:
    case T_SOURCE_SRCH:
    default:
      send_msg(msglist[ HUH ], cur_sock);
      break;

      } /* switch on transtype */
}


static void
send_servers_list(int cur_sock)
{
  FILE           *fopen(), *srvfile;
  char           buf[BUFSIZ],*cp;
  int            num = 0;
  
  srvfile = fopen(SERVER_FILE, "r");
  if (!srvfile) {
    perror(SERVER_FILE);
    send_msg(msglist[ CANT_FIND_SERVER_FILE ], cur_sock);
    return;
  }

  /* how many lines in the file? */
  for (num = 0; fgets(buf,BUFSIZ-1,srvfile) != NULL; num++);
  rewind (srvfile);

  cp = (char *) domalloc ((num+1) * BUFSIZ);  /* a little extra room */
  sprintf (cp, "%d:servers\n", num);

  while (fgets(buf, BUFSIZ-1, srvfile) != NULL) 
    strcat (cp, buf);
  strcat (cp, ".\r\n");

  fclose(srvfile);
  sendbuf (cp, strlen(cp), cur_sock);
  return;
}


/*
  send_nlistmsg_trav should only be used when the TI client
  is expecting an nlist as the response to T_TRAVERSE.
  TI client is expecting:
  #nodes:
  0:parent
  1:child1
  1:child2
  .
  */

static void
  send_nlistmsg_trav (long par, long child, int repeatpar, int flgsfmt, int cur_sock)
{
  char *nlistptr;

  if (repeatpar) {
    nlistptr = (char *) domalloc (3*BUFSIZ);
    sprintf (nlistptr, "3:\n0:");
  }
  else {
    nlistptr = (char *) domalloc (2*BUFSIZ);
    sprintf (nlistptr, "2:\n0:");
  }

  strcat (nlistptr, nodeinfostr(par, (struct s1 *)NULL, flgsfmt));
  strcat (nlistptr, "\n1:");
  if (repeatpar) {
    strcat (nlistptr, nodeinfostr(par, (struct s1 *)NULL, flgsfmt));
    strcat (nlistptr, "\n1:");
  }
  strcat (nlistptr, nodeinfostr(child, (struct s1 *) NULL, flgsfmt));
  strcat (nlistptr, "\n.\r\n");
  sendbuf (nlistptr, strlen(nlistptr), cur_sock);

  /* Assumption: sendbuf will free the nlistptr allocated memory */
}



/* send_nlistmsg_find should only be used when the TI client
   is expecting an nlist as the response to T_FIND.
   TI client is expecting:
   #nodes:
   0:node1
   0:node2
   .
*/

static void send_nlistmsg_find (long par, long child, int flgsfmt, int cur_sock)
{
  char *nlistptr;

  nlistptr = (char *) domalloc (2*BUFSIZ);

  sprintf (nlistptr, "2:\n0:");
  strcat (nlistptr, nodeinfostr(par, (struct s1 *)NULL, flgsfmt));
  strcat (nlistptr, "\n0:");
  strcat (nlistptr, nodeinfostr(child, (struct s1 *) NULL, flgsfmt) );
  strcat (nlistptr, "\n.\r\n");
  sendbuf (nlistptr, strlen(nlistptr), cur_sock);

  /* Assumption: sendbuf will free the nlistptr allocated memory */
}




/* send the file associated with a certain node to the client. */
static void send_file(CONN *conn, char trans_type, int num_transfields,
		    char **transfields)
{
  int nodeid;
  struct s1 *nd;
  int cur_sock = conn->c_socket;
  int flgsfmt = conn->c_output_fmt;

  if (num_transfields < 2)
    nodeid = 0;
  else 
    nodeid = atoi (transfields[1]);

  if (!send_reserved_node(trans_type, num_transfields, transfields, nodeid, flgsfmt, cur_sock)) {
    nd = getnode_bynodeid(nodeid);

    if (nd == NULL) {
      send_empty_document(cur_sock);
      return;
    }

    switch (nd->gophertype) {
    case GOPHTYP_GIF:
    case GOPHTYP_IMAGE:
    case GOPHTYP_TEXT:
      send_gopher_info(nd, num_transfields, transfields, conn);
      break;

    case GOPHTYP_CSO: 
    case GOPHTYP_MACHQX:
    case GOPHTYP_DOSBIN:
    case GOPHTYP_UUENC:
    case GOPHTYP_BINARY:
    case GOPHTYP_SOUND:
    case GOPHTYP_EVENT:
    case GOPHTYP_CALENDAR:
    case GOPHTYP_HTML:
    case GOPHTYP_MIME:
      send_reserved_node (trans_type, num_transfields, transfields,
			  GOPHERFILETYPES_UNAVAILABLE, flgsfmt, cur_sock);
      break;

    case GOPHTYP_TELNET:
    case GOPHTYP_TN3270:
      send_reserved_node (trans_type, num_transfields, transfields, CLIENT_NEEDS_TELNET, flgsfmt, cur_sock);
      break;

    case GOPHTYP_MENU:
    case GOPHTYP_SEARCH:
      send_msg (msglist[ NOT_DOCUMENT ], cur_sock);
      break;
      
    default:
      send_reserved_node (trans_type, num_transfields, transfields, 
			  UNKNOWN_DOC_TYPE, flgsfmt, cur_sock);
      break;
    }
  }
}




void send_empty_document(int sock)
{
  nio_send_file (sock, "/dev/null", 0, 32000, 0);
}


void send_empty_menu (int sock)
{
  send_msg ("0:", sock);
}



static int compute_nodeflags (struct s1 *nd, int format_type)
{
  if (format_type == NO_FLAGS_FORMAT) 
    return 0;
  
  else if (nd == NULL)
    return N_FAKE;  /* Not a real node */
  
  else {
    switch (nd->gophertype) {
      /*
	Most of the N_ flags don't appear here even though MIT has
	put them into the node.h file in the source distribution.
	None of their code handles the N_ flags yet (except for N_IMAGE).
	N_TELNETSESSION is my own creation. --lam
	*/
      
    case GOPHTYP_GIF:
    case GOPHTYP_IMAGE:
      return N_IMAGE;
      break;
      
    case GOPHTYP_TELNET:
    case GOPHTYP_TN3270:
      return N_TELNETSESSION; 
      break;
      
    default:
      return 0;
      /* N_TEXT and N_MENU flags are ignored by TI clients
	 at the moment (Feb 1993), so I'm leaving them out. */
      break;
    }
  }
}



static char *reserved_nodeinfostr(long nid, int flgfmt)
     /* ASSUMPTION: the values in reserved nodes don't contain any extraneous
	DLM characters */
{
  int flags;
  extern struct resvnode *get_resvnode();
  struct resvnode *rnode;

  rnode = get_resvnode (nid);

  if (rnode == NULL) {
    rnode = get_resvnode(DUMMY_NODE);
  }

  /*
    MIT's code doesn't seem to handle these N_ flags...so just
    use 0 as the flags and ignore flgsfmt

    if (cur_format_type == NO_FLAGS_FORMAT)
    flags = 0;
    else if (*(rnode->file))
    flags = N_TEXT;
    else 
    flags = N_MENU;
    */
  
  flags = 0;
  /*nodeid:flags:date:keyword list:title:source:locker:file:parents:children*/

  if (*(rnode->file)) 
    sprintf (nodeinfobuf, "%d:%d:%d::%s.:%s:none:%s::",
	     nid, flags, todaysdate, rnode->title,
	     GW_SOURCENAME, rnode->file);
  else
    sprintf (nodeinfobuf, "%d:%d:%d::%s/:%s::::", 
	     nid, flags, todaysdate, rnode->title,
	     GW_SOURCENAME);

  return (nodeinfobuf);
}


char *nodeinfostr (long nodeid, struct s1 *nd, int flgsfmt)
{
  long nodeflags;
  char gophertype;
  char gophertitle[BUFSIZ];
  char gopherpath[BUFSIZ];
  char gopherserver[BUFSIZ];
  char gopherport[BUFSIZ];

  if (is_reserved_node (nodeid))
    return (reserved_nodeinfostr(nodeid, flgsfmt));

  if (nd == NULL) { /* wasn't already resolved, so find it */
    nd = getnode_bynodeid(nodeid);
  }

  nodeflags = compute_nodeflags (nd, flgsfmt);

  if (nd == (struct s1 *) NULL) {  /* still not found; it's a bogus nodeid */
    return (reserved_nodeinfostr(DUMMY_NODE, flgsfmt));
  }

  else { 
    /* there is no way to quote DLM character in the TechInfo
       protocol at this time.  So convert it to something
       else and let's hope it all works out */

    substi_char (nd->gophertitle,   gophertitle,  DLM, ';');
    substi_char (nd->gopherpath,    gopherpath,   DLM, ';');
    substi_char (nd->gopherserver,  gopherserver, DLM, ';');
    substi_char (nd->gopherport,    gopherport,   DLM, ';');
    if (nd->gophertype == DLM) gophertype = ';';
    else gophertype = nd->gophertype;

    switch (nd->gophertype)
      {
      case GOPHTYP_MENU:
      case GOPHTYP_SEARCH:
	sprintf (nodeinfobuf, "%d:%d:%d:%c %s %s %s:%s%s:%s::::",
		 nd->nodeid, nodeflags, todaysdate, 
		 
		 gophertype, gopherpath, gopherserver, gopherport,
		 /* YUCK! use keywords fields for gopher info */
		 
		 gophertitle, find_abbrev(nd->gophertype),
		 GW_SOURCENAME);
	
	/* no locker, no filename, no parents, no children */
	break;
	
      case GOPHTYP_CSO:
      case GOPHTYP_TEXT:
      case GOPHTYP_GIF:
      case GOPHTYP_IMAGE:
      case GOPHTYP_MACHQX:
      case GOPHTYP_DOSBIN:
      case GOPHTYP_BINARY:
      case GOPHTYP_UUENC:
      case GOPHTYP_TELNET:
      default:
	sprintf (nodeinfobuf, "%d:%d:%d:%c %s:%s%s:%s:%s:%s %s::",
		 nd->nodeid, nodeflags, todaysdate,
		 
		 gophertype, gopherport,
		 /* YUCK!!! use keywords field for some gopher info */
		 
		 gophertitle, find_abbrev(nd->gophertype),
		 GW_SOURCENAME,
		 "none",
		 /* put rest of gopher info in filename */
		 gopherpath, gopherserver);
	/* no parents, no children */
	break;
      }
  } /* not a bogus nodeid */
  
  return (nodeinfobuf);  /* static buf */
}



static void
send_node(int flgsfmt, int cur_sock, int num_transfields, char **transfields)
     /* send all information about a particular node */
{
  long nodeid;
  /* send node information:
     Unfortunately, we don't have a record of the parents & children 
     because we don't really have a web */
  
  if (num_transfields < 2)
    nodeid = MAINMENUNODE;
  else
    nodeid = atoi (transfields[1]);

  send_msg (nodeinfostr(nodeid, (struct s1 *) NULL, flgsfmt), cur_sock);
}


/* write the transaction to the log file */
void log_trans(char *line)
{
	char            logline[BUFSIZ];
	time_t          secs;
	int             dfd;

	time(&secs);
	sprintf(logline, "%s:%s", line, ctime(&secs));
	if ((dfd = open(TRANS_LOG, O_APPEND | O_CREAT | O_WRONLY, 0644)) == 0)
		printf("error logging transaction\n");
	write(dfd, logline, strlen(logline));
	close(dfd);

	if (debug)
	  fprintf(stderr, "Transaction: %s", logline);
	return;
}


/*
  search--search for target.  If no node
  is given, use global Gopher search node(veronica).
  Called for keyword search, wais-index text search,
  title search.
  */
static void
 find(CONN *conn, char trans_type, int num_transfields, char **transfields)
{
  long nodeid;
  struct s1 *nd;
  char tmpstr[BUFSIZ];
  int cur_sock = conn->c_socket;
  int flgsfmt = conn->c_output_fmt;
  
  /* blank target string? send empty menu */
  if (num_transfields < 2 || strlen(transfields[1]) < 1) {
    send_empty_menu(cur_sock);
    return;
  }

  if (num_transfields > 2) {
    nodeid = atoi(transfields[2]);
    if (nodeid == MAINMENUNODE)
      nodeid = GLOBGOPH_SRCH_NODE;
  }
  else
    nodeid = GLOBGOPH_SRCH_NODE;

  if (is_reserved_node(nodeid)) {
    send_nlistmsg_find (nodeid, FILE_NOT_SEARCHABLE, flgsfmt, cur_sock);
    return;
  }

  nd = getnode_bynodeid (nodeid);
  if (nd == NULL)
    send_empty_menu(cur_sock);

  else {
    switch (nd->gophertype) {
    case GOPHTYP_SEARCH:
      send_gopher_info(nd, num_transfields, transfields, conn);
      break;
    default:
      send_nlistmsg_find (nodeid, FILE_NOT_SEARCHABLE, flgsfmt, cur_sock);
      break;
    }
  }
}



/* Change output format */
static void 
change_format(int *fmt, int cur_sock, int num_transfields, char **transfields)
{
  int format;

  if (num_transfields < 2) {
    send_msg(msglist[ UNKNOWN_FORMAT ], cur_sock);
    return;
  }

  format = atoi(transfields[1]);
  if ((format < 1) || (format > NUM_FORMATS))
    {
      send_msg(msglist[ UNKNOWN_FORMAT ], cur_sock);
      return;
    }
  *fmt = format;
  send_msg(msglist[ OK ], cur_sock);
}



static void
  traverse(CONN *conn, char trans_type, int num_transfields, char **transfields) 
{
  int direction;
  long nodeid;
  int levels;
  struct s1 *nd;
  int cur_sock = conn->c_socket;
  int flgsfmt = conn->c_output_fmt;
  
  if (num_transfields < 4) { /* if not enough fields, send empty list */
    send_empty_menu (cur_sock);
    return;
  }
  
  direction = atoi(transfields[1]);
  nodeid = atoi(transfields[2]);
  levels = atoi(transfields[3]);

  if (direction == TRAV_UP)
    send_nlistmsg_trav (nodeid, NO_SHOW_PATH, 0, flgsfmt, cur_sock);

  else if (direction == TRAV_OUT)
    send_nlistmsg_trav (nodeid, NO_SHOW_PATH, 0, flgsfmt, cur_sock); 

  else if (levels > 1)
    send_nlistmsg_trav (nodeid, NO_OUTLINE, 0, flgsfmt, cur_sock);

  else { /* traverse down ONE level */
    if (!send_reserved_node(trans_type, num_transfields, transfields,
			    nodeid, flgsfmt, cur_sock)) {
      nd = getnode_bynodeid (nodeid);
      if (nd == NULL) /* client gave unknown nodeid, it loses */
	send_empty_menu(cur_sock);
      else {
	switch (nd->gophertype)
	  {
	  case GOPHTYP_MENU:
	    send_gopher_info (nd, num_transfields, transfields, conn);
	    break;

	  case GOPHTYP_SEARCH:
	    send_nlistmsg_trav (nodeid,TRY_SEARCHCMD, 1, flgsfmt, cur_sock);
	    break;

	  default:
	    send_nlistmsg_trav (nodeid, FILE_NOT_AMENU, 1, flgsfmt, cur_sock);
	    break;
	  }
      } /* not unknown */
    } /* handle reserved */
  } /* traverse down one level */
}



/* find the version number of the client associated with a certain machine */
static void
 find_ver(CONN *conn, int num_transfields, char **transfields) 
{
	FILE           *fopen(), *verfile;
	char            verline[ADMIN_LN_SZ];
	int             len;
	int             cur_sock = conn->c_socket;

	verfile = fopen(VER_FILE, "r");
	if (!verfile) {
	  send_msg(msglist[ VERSION_NOT_FOUND ], cur_sock);
	  return;
	}

	if (num_transfields < 2 || strlen(transfields[1]) == 0) {
	  conn->c_type = "";
	  send_msg(msglist[ VERSION_NOT_FOUND ], cur_sock);
	  return;
	}

	len = strlen(transfields[1]);
	/* set c_type in conntab */
	conn->c_type = domalloc((unsigned int) len+1);
	strcpy(conn->c_type, transfields[1]);
	
	while (fgets(verline, ADMIN_LN_SZ, verfile) != '\0') {
		verline[strlen(verline) - 1] = '\0';
		if (strncasecmp(transfields[1], verline, len) == 0) {
			fclose(verfile);
			send_msg(verline, cur_sock);
			return;
		}
	}
	fclose(verfile);
	send_msg(msglist[ VERSION_NOT_FOUND ], cur_sock);
	return;		/* close file */
}




static void send_src_info(int cur_sock)
{
  char sourceline[BUFSIZ];

  /* source:longsrc:name:phone:email */
  sprintf (sourceline, "%s:%s:::%s",
	   GW_SOURCENAME, GW_LONGSRCNAME, GW_EMAILADDR);
  send_msg (sourceline, cur_sock);
}


  /* BUFFERSIZE must be big enough to handle all possible connections
     + a header */
#define BUFFERSIZE (200 + FD_SETSIZE*110)

/* Send a description of the current connections */
void
send_connections(int cur_sock) 
{
  int i;
  char *buf, *prov, *cp;
  time_t  secs;
  extern CONN     conntab[];
  
  buf = (char *) domalloc((unsigned) BUFFERSIZE);
  bzero(buf, BUFFERSIZE);

  sprintf(buf,"\
%-5s %-5s  %-7s %-8s  %-25s  %-15s %s\n",
"Conn#", "Sock", "Sesslen", "Inactive", "Host", "ClientType", "Prov");
  
  time(&secs);
  for (i = 0; i < FD_SETSIZE; i++) 
    {
      if (conntab[i].c_socket != -1)
	{
	  cp = index(buf,'\0');
	  if (conntab[i].c_flags & C_PROVIDER)
	    prov = "PROV";
	  else
	    prov = "";
	  sprintf(cp,"\
%-5d %-5d  %-7.1f %-8.1f  %-25s  %-15s %s\n"
		  ,i,conntab[i].c_socket,
		  (float) ((secs - conntab[i].c_made) /60.0),
		  (float) ((secs - conntab[i].c_last) /60.0),
		  conntab[i].c_hostname, 
		  conntab[i].c_type,
		  prov);
	}
    }
  /* 
   * Put End-of-Message on buffer & send it out.  sendbuf() frees the
   *  buffer for us.
   */
  strcat(buf, EOM);
  sendbuf(buf, strlen(buf), cur_sock);
}
#undef BUFFERSIZE




/* test to see if a uid is an admin */
static int test_admin(char *passwd)
{
  FILE           *fopen(), *adminfile;
  char            adminline[ADMIN_LN_SZ];

  adminfile = fopen(ADMIN_FILE, "r");
  if (!adminfile) {
    perror(ADMIN_FILE);
    return 0;
  }
  while (fgets(adminline, ADMIN_LN_SZ, adminfile) != '\0') {
    adminline[strlen(adminline) - 1] = '\0';
    if (strcasecmp(passwd, adminline) == 0) {
      fclose(adminfile);
      return 1;
    }
  }
  fclose(adminfile);
  return 0;		/* close file */
}

/* If the client's uid is ok, establish him as an administrator. */
static void admin(int num_transfields, char **transfields, int cur_sock)
{
  if (num_transfields < 2 || *(transfields[1]) == 0)
    send_msg(msglist[ NOT_AUTH ], cur_sock);
  else if (test_admin(transfields[1])) {
    strcpy(curr_uid, transfields[1]);
    send_msg(msglist[ OK ], cur_sock);
  }
  else
    send_msg(msglist[ NOT_AUTH ], cur_sock);
}



/* nd is the parent or the search node.
   if nd's gophertype is menu, write numnodes+1 into the buf 
   because the parent node itself should be in there.
   nlist is the list of children (or a results list).
   */
static void
send_nlist (struct s1 *nd, long *nlist, int numnodes, int flgsfmt, int cur_sock)
{
  char *buf;
  char *levelstr;
  int n;
  struct s1 *node;

  if (nd == NULL) {
    fprintf (stderr, "send_nlist() called with null nd parameter\n");
    return;
  }

  if (nd->lastused != todaysdate) {
    nd->lastused = todaysdate; /* node was selected */
    tables_changed++;
    lastused_changed++;
  }

  /* assumption: average length of nodeinfo is less than 500 chars */
  buf = (char *) malloc (500 * (numnodes+1));

  if (nd->gophertype == GOPHTYP_MENU) {
    sprintf (buf, "%d:\n0:%s\n", numnodes+1,
	     nodeinfostr(nd->nodeid, nd, flgsfmt));
    levelstr = "1:";
  }
  else {
    sprintf (buf, "%d:\n", numnodes);
    levelstr = "0:";
  }

  for (n = 0; n < numnodes; n++) {
    strcat (buf, levelstr);

    node = getnode_bynodeid (nlist[n]);

    /* item appeared on a menu, so update its "usedness" */
    if (node != NULL) {
      if (node->lastused != todaysdate) {
	node->lastused = todaysdate; /* node is used as part of a menu */
	tables_changed++;
	lastused_changed++;
      }
    }
    
    strcat (buf, nodeinfostr(nlist[n], node, flgsfmt));
    strcat (buf, "\n");
  }
  strcat (buf, ".\r\n");
  sendbuf(buf, strlen(buf), cur_sock);
  /* assuming sendbuf will free the allocated buf */
}


send_cached_nlist (struct s1 *nd, int cur_sock, char *cachefilename, int flgsfmt)
{
  int numnodes;
  FILE *cafile;
  char line[100];
  long *nlist;
  int n;

  cafile = fopen(cachefilename, "r");
  if (cafile == NULL) {  /* not sure what to do at this point */
    fprintf (stderr, "Unable to read cache file %s!\n", cachefilename);
    return;
  }
  for (numnodes=0; fgets(line, sizeof(line)-1, cafile) != NULL; )
    numnodes++;

  nlist = (long *) domalloc ((unsigned int) numnodes * sizeof (long));
  n = 0;
  for (rewind(cafile); fgets(line, sizeof(line)-1, cafile) != NULL; ) {
    nlist[n] = atol(line);
    n++;
  }

  send_nlist (nd, nlist, numnodes, flgsfmt, cur_sock);
  free(nlist);
  fclose (cafile);
}




/*  handles 5 types: MENU, TEXT, GIF, IMAGE, SEARCH */
/* side effect: forks child, sets conn->c_hptr  */
static void send_gopher_info(struct s1 *nd, 
		      int num_transfields, char **transfields,
		      CONN *curr_conn)
{
  struct stat stbuf;
  char cachefilename[MAXPATHLEN];
  long startpoint = 0, requested = 32000;
  WAITFORHELPER *hptr;
  pid_t pid;
  char *path;
  int cur_sock = curr_conn->c_socket;
  int flgsfmt = curr_conn->c_output_fmt;

  if (nd == NULL)
    return;   /* should probably send a message or something */

  if (nd->lastused != todaysdate) {
    nd->lastused = todaysdate;  /* node was selected */
    tables_changed++;
    lastused_changed++;
  }

  if (nd->gophertype == GOPHTYP_TEXT ||
      nd->gophertype == GOPHTYP_GIF ||
      nd->gophertype == GOPHTYP_IMAGE) { /* not for menus & searches */
    /* get startpt & length from transaction */
    if (num_transfields > 2) startpoint = atoi(transfields[2]);
    if (num_transfields > 3) requested = atoi(transfields[3]);
  }
  
  if (nd->gophertype == GOPHTYP_SEARCH) {
    /* assuming calling code already checked for blank search string */
    path = domalloc(strlen(transfields[1]) + 1 +
		    strlen(nd->gopherpath) + 1);
    sprintf (path, "%s\t%s", nd->gopherpath, transfields[1]);
    sprintf (cachefilename, "%s%d.%c.%s", CACHEDIR, nd->nodeid, 
	     nd->gophertype, transfields[1]);
  }

  else { /* if type is something other than SEARCH */
    path = domalloc(strlen(nd->gopherpath) + 1);
    sprintf (path, "%s", nd->gopherpath);
    sprintf (cachefilename, "%s%d.%c", CACHEDIR, nd->nodeid, 
	     nd->gophertype);
  }
  
  /* Can we use cached file? */
  
  if (stat(cachefilename, &stbuf) == 0 &&
      ((time(0) - stbuf.st_mtime) < CACHETIMEOUT))  {   /* use cached file */
    
    if (nd->gophertype == GOPHTYP_MENU ||
	nd->gophertype == GOPHTYP_SEARCH) 
      send_cached_nlist (nd, cur_sock, cachefilename, flgsfmt);
    
    else /* TEXT, GIF, or IMAGE */
      nio_send_file (cur_sock, cachefilename, startpoint, requested, 0);
    
    return;                       /* DONE ! */
  }  /* use cache file */


  /* if we got here, we couldn't use the cache file.
     Need to call a helper. Create c_hptr for the connection */

  /* Create and initialize c_hptr for curr_conn */
  hptr = (WAITFORHELPER *) domalloc ((unsigned int)sizeof (WAITFORHELPER));
  curr_conn->c_hptr = hptr;

  hptr->file = tempnam(CACHEDIR, "GOPHER");
  hptr->node = nd;
  /* ASSUMING that nothing will remove nd from the tables between
     now and the time that the help returns ! */
  hptr->startpoint = startpoint;
  hptr->requested = requested;
  hptr->cachefile = (char *) domalloc (strlen(cachefilename) + 1);
  strcpy (hptr->cachefile, cachefilename);

  
  pid = fork ();
  if (pid == 0) {    /* child */
    execlp (HELPER, HELPER, nd->gopherserver, nd->gopherport, path,
	    hptr->file, (char *)0);
    fprintf (stderr, "EXECLP FAILED!!!\n");
    perror (HELPER);
    exit (-1);  /* child had a problem */
  }

  else if (pid < 0) {  /* parent, but no child */

    /* what to send to TI client ???*/

    fprintf (stderr, "Unable to fork child\n");
    perror ("fork");
  }

  else {  /* parent */
    (curr_conn->c_hptr)->pid = pid;
    signal (SIGCHLD, catch_child);
    fprintf (stderr, "\nT=%d.  Set up helper pid %d for sock %d\n",
	     time(0), (curr_conn->c_hptr)->pid, curr_conn->c_socket);
    do_free(path);
  }
}



static char *find_abbrev(char ch)
{
  int x;

  for (x = 0; x < NUM_GOPHER_ABBREV && ch != gopher_abbrev[x].gophtyp; x++);
  if (ch == gopher_abbrev[x].gophtyp)
    return (gopher_abbrev[x].gophabbrev);
  else
    return (gopher_abbrev_unk);
}


/* ASSUMPTION: T_SENDFILE, T_TRAVERSE, and T_HELP call send_reserved_node  */
static int
send_reserved_node (char trans_type, int num_transfields, char **transfields, long nodeid, int flgsfmt, int cur_sock)
{
  struct resvnode *rnode;
  int child;
  int starting = 0;
  int requested = 32000;
  char *nlistbuf;

  if (!is_reserved_node(nodeid))
    return 0;

  if (trans_type == T_SENDFILE) { /* t:nodeid:st:en */
    if (num_transfields > 2)
      starting = atoi (transfields[2]);
    if (num_transfields > 3)
      requested = atoi (transfields[3]);
  }

  rnode = get_resvnode(nodeid);
  if (rnode == NULL) {
    if (trans_type == T_SENDFILE)
      send_empty_document(cur_sock);
    else if (trans_type == T_TRAVERSE)
      send_empty_menu(cur_sock);
    return 1;
  }
  
  else { /* found reserved node */
    if (trans_type == T_SENDFILE) {
      if (*(rnode->file)) {
	nio_send_file(cur_sock, rnode->file, starting, requested, 0);
      }
      else
	send_msg (msglist[ NOT_DOCUMENT ], cur_sock);
    }

/* add code around here to dynamically generate an nlist
   of all the gopher nodes which are GOPHTYP_SEARCH */

    else if (trans_type == T_TRAVERSE || trans_type == T_HELP) {
      nlistbuf = (char *) domalloc ((rnode->numchildren + 1)* BUFSIZ);
      
      /*ASSUMPTION: the average length of the nodeinfo for the
	reserved node and its children is less than BUFSIZ*/
      
      /*Assumption: we aren't automatically adding HelpMenu and
	Veronica to reserved node menus */
      
      sprintf (nlistbuf, "%d:\n", rnode->numchildren + 1);
      strcat (nlistbuf, "0:");
      strcat (nlistbuf, reserved_nodeinfostr(rnode->nodeid, flgsfmt));
      strcat (nlistbuf, "\n");
      for (child=0; child < rnode->numchildren; child++) {
	strcat (nlistbuf, "1:");
	strcat (nlistbuf, nodeinfostr(rnode->children[child],
				      (struct s1 *) NULL, flgsfmt));
	strcat (nlistbuf, "\n");
      }
      strcat (nlistbuf, ".\r\n");
      sendbuf (nlistbuf, strlen(nlistbuf), cur_sock);
      /* Assumption: sendbuf & its cronies will free the allocated
	 space for nlistbuf */
    }
  }
  return 1;
}

