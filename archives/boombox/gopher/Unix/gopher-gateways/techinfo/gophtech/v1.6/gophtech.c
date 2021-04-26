/* 
  Gopher --> TechInfo Gateway
  Copyright November 1992, January 1993 by the University of Pennsylvania.

  Export of this software from the United States of America is assumed
  to require a specific license from the United States Government.  It
  is the responsibility of any person or organization contemplating
  export to obtain such a license before exporting.  Within that
  constraint, permission to use, distribute, or copy this software is
  granted, provided that this copyright notice appear on all copies.
  This software is provided as is, with no explicit nor implicit
  warranty.

*/

/* HISTORY:
May/June 1993: version 1.6
      Changed search capability so that ALL techinfo search functions
      are available to gophtech gateway users.  (Took out #ifdef WAIS).

      Changed SOURCES_MSGFILE to SOURCES_MSG_NODEID and added 
      send_ti_doc() call.

      Got rid of MSGFILE completely.

Mar 1993: version 1.5
      Added recognition of telnet and tn3270 file types (TI_TELNETSESS).
      Fixed bug which caused NeXT gopher to barf -- 
          Changed "\r\n.\r\n" to ".\r\n" at the end of each element sent.
      Commented out various lines which logged extra information.
      Took out the test "University of Pennsylvania" system
          from techinfo-telnet menu.

Jan 1993: version 1.4
      Added ability to point at local Sources document

Jan 1993: version 1.3 
      Added ability to point at local Gopher server.
      Added TechInfo Source to Path (informational only --
              the gateway ignores it).
      Added Full Text Searching (do #define WAIS or CFLAGS=-DWAIS)

Nov 1992: version 1.2 added GIF.
Feb 1992: Created the gateway.

*/
#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "gw.h"

#define GWVERSION     "GOPH-TIGW:1.6"
#define TERMBASEDTI   "telnet-techinfo"     /* Telnet session to techinfo */
#define WORLDWIDETI   "worldwide-techinfo"  /* List of all TechInfo servers */
#define SOURCESINFO   "sources "

int gophsock = -1;
int gophinsock = -1;

char tibuf[200000];
char gophbuf[10000];
char *gatewayhost;
char gatewayport[40];
char remotehost[500];

#define CR '\r'
#define LF '\n'
#define EOM             ".\r\n"
#define EOM_LEN         (sizeof(EOM) -1)
#define	terminator(str)	(!strncmp(str, EOM, EOM_LEN))

#ifndef RUNTIME_UID
#define RUNTIME_UID 1   /* 1 is usually the DAEMON */
#endif

/* TechInfo Protocol */
#define TICMD_GETSERVERS        "m"
#define TICMD_GETMENU           "w:2:%s:1"
#define TICMD_GETDOC            "t:%s:%d:%d"
#define TICMD_VERSION           "v:%s"
#define TICMD_SHOWFLAGS         "O:2"
#define TIOKAY                  "0:"
#define TIMAXFIELDS             40
#define TIDELIM                 ':'
#define TOTCHARSTR              "Total Characters :"
#define TI_GIFIMAGE              0x40
#define TI_TELNETSESS            0x2000

#define TICMD_KEYSRCH_NODEID       "b:%s:%s"   /* target:nodeid */
#define TICMD_FULLTEXTSRCH_NODEID  "J:%s:%s"   /* target:nodeid:max#docs */
#define TICMD_TITLESRCH_NODEID     "T:%s:%s"   /* target:nodeid */

#define TICMD_DATESRCH_NODEID      "I:%s:%s"   /* nodeid:mm:dd:yy */
#define TICMD_SOURCESRCH_NODEID    "K:%s:%s"   /* nodeid:target */




/* Gopher Protocol */
#define GO_DOC		'0'
#define GO_MENU		'1'
#define GO_CSO          '2'
#define GO_ERR          '3'
#define GO_MACHQX       '4'
#define GO_DOSBIN       '5'
#define GO_UUENCODED    '6'
#define GO_SEARCH       '7'
#define GO_TELNET       '8'
#define GO_BINARY       '9'
#define GO_GIF          'g'
#define GO_DUPLSERV     '+'
#define GO_TN3270       'T'

#define GO_DELIM        '\t'

/* Intermediary protocol */
#define I_DELIM         ' '
#define I_DOC           'D'
#define I_MENU          'M'
#define I_GIFIMAGE      'I'
#define I_SEARCHMENU    'S'
#define I_KEYSRCH       'K'
#define I_TITLESRCH     'T'
#define I_DATESRCH      'N'
#define I_SOURCESRCH    'O'
#define I_FULLTEXTSRCH   'F'


getreq (char *request, int max, int insock)
{
  int idx;
  char *cp;

  readline (request, max, insock, &idx);
  cp = index (request, CR);
  if (cp) *cp = 0;
  cp = index (request, LF);
  if (cp) *cp = 0;
}


write_doc (int gophsock, int readlast, int len, int TextDoc, char *docbuf, 
	   int *totalsent)
{
  int wrote;
  int towrite;
  int pos1, pos2;

  /*
    if TextDoc is set, then we need to add CR before each LF,
    since the TI server doesn't send CR LF at the end of every line 
    Also, assuming that if TextDoc contains LF . LF, it should
    be changed to CF LF . . CF LF
    
    known problem -- if the TI server breaks up the chunks it
    sends on a line boundary, and sends a period LF, then
    this code will NOT properly quote that period with another one.
    */
  
  if (readlast)         /* if these were the last chars to be read */
    towrite = len - 4;
  else
    towrite = len;

  if (TextDoc) {
    pos1 = 0;
    do {
      for (pos2=pos1; docbuf[pos2] != LF && pos2 < towrite-1; 
	   pos2++);

      if (docbuf[pos2] == LF) {  /* found an End of Line */
	wrote = write (gophsock, docbuf+pos1, pos2-pos1);
	if (wrote < 0)
	  ErrExit("Error writing document to gopher");
	*totalsent = *totalsent + wrote;
	write (gophsock, "\r", 1); (*totalsent)++; 
	write (gophsock, "\n", 1); (*totalsent)++;
	
	if (pos2 + 2 < towrite && 
	    docbuf[pos2+1] == '.' && docbuf[pos2+2] == LF) {
	  write (gophsock, ".", 1);
	  (*totalsent)++;
	}
	pos1 = pos2 + 1;
      }
      
      else { /* No more LF left in the buffer */
	wrote = write (gophsock, docbuf+pos1, towrite-pos1);
	if (wrote < 0)
	  ErrExit ("Error sending document to gopher");
	(*totalsent) += wrote;
	pos1 = towrite;
      }
	  
    } while (pos1 < towrite);
  }
  
  else {  /* Document is not Text, so just send it as it comes from TI */
    wrote = write (gophsock, docbuf, towrite);
    if (wrote < 0)
      ErrExit ("Error sending nontext document to gopher");
    (*totalsent) += wrote;
  }
}




int send_ti_doc (int tisock, int gophsock, char *nodeid, int TextDoc)
{
  char docbuf[12200];
#define DOC_BLKSZ (sizeof(docbuf)-200)
  long startat;
  long totalchars;
  long totalsent = 0;
  char *cp;
  int len;
  int toread;
  int red;

  startat = 0;
  do {
    sprintf (tibuf, TICMD_GETDOC, nodeid, startat, DOC_BLKSZ);
    /*    sprintf (docbuf, "ToTechinfo:%s", tibuf);
      logdebug (docbuf); */
    send_msg (tisock, tibuf, strlen(tibuf));
    readline (docbuf, DOC_BLKSZ, tisock, &len);

    if (startat == 0) {
      totalchars = atoi (docbuf);
/*      sprintf (tibuf, "Document is %d bytes", totalchars);
      logdebug (tibuf); */
    }

    cp = strstr (docbuf, TOTCHARSTR);
    if (cp) {
      cp = cp + sizeof(TOTCHARSTR) - 1;
      toread = atoi(cp);
      read (tisock, docbuf, 1);  /* TI server sends 1 extra character (a LF)
				    between 1st line & the document */

      do {
	red = read (tisock, docbuf, toread);
	toread = toread - red;
	write_doc (gophsock, toread < 1, red, TextDoc, docbuf, &totalsent);
      } while (toread > 0);

      startat = startat + DOC_BLKSZ;
    }
    else {
      sprintf (tibuf, "Unable to find %s in TI server's response %s\n",
	      TOTCHARSTR, docbuf);
      ErrExit (tibuf);
    }

  } while (totalchars - startat > 0);

  sprintf (docbuf, "Sent %d bytes", totalsent);
  logdebug (docbuf);

  if (totalsent < 1) {
    sprintf (docbuf, "%s", "File does not exist!!!!!");
    send_msg (gophsock, docbuf, strlen(docbuf));
  }
  return totalsent;
}




ErrExit (char * msg)
{
  char *buf;

  write (gophsock, "\r\n.\r\n", 5);

  buf = (char *) malloc (strlen(msg)+20);
  sprintf (buf, "FatalError:%s", msg);
  logdebug (buf);
  exit(-100);
}


int parse_fields (char delim, char *line, char *fields[], int maxfields)
{
  char *cp = line;
  int argcnt = 0;

  while (argcnt < maxfields) {
    fields[argcnt] = cp;
    argcnt++;
    while (*cp && *cp != delim)
      cp++;
    if (!*cp)
      break;
    *cp = 0;
    cp++;
  }
  fields[argcnt] = (char *) 0;
  return argcnt;
}


struct hostent *
gethostbyn_or_ad (host)
     char *host;
{
  u_long ip_addrl;
  struct hostent *h;

  if (isdigit (host[0])) {     /* if name begins with a digit, assume IP adr */
    ip_addrl = inet_addr (host);
    h = gethostbyaddr (&ip_addrl, sizeof(u_long), AF_INET);
    return (h);
  }
  else {
    h = gethostbyname (host); 
    return (h);
  }
}


int getportbyn_or_num (str, port)
     int *port;
     char *str;
{
  struct servent *servent;

  if ( isdigit(str[0]) ) {   /* if its a number, convert ascii to int */
    *port = atoi(str);
    *port = htons( (u_short) *port);
  }
  else {        /* if not a number, look up name in Ultrix */
    servent = getservbyname (str, "tcp");
    if (servent == NULL)
      return (-1);
    *port = servent->s_port;
  }
  return (1);
}





int
inet_connect(char *hostname, char *service, char **errstr)
{
#define ERRORMSG_SIZE 200
  struct hostent *host;
  int toport;
  struct sockaddr_in sin;
  int sock;
  extern char *sys_errlist[];
  extern int sys_nerr;

  *errstr = (char *) malloc (ERRORMSG_SIZE);
  (*errstr)[0] = 0;

  host = gethostbyn_or_ad (hostname);

  if (host == NULL) {
    sprintf (*errstr, "%s: Unknown host", hostname);
    return -1;
  }

  else  if (getportbyn_or_num (service, &toport) < 0)  {
    sprintf (*errstr, "%s: Unknown TCP service", service);
    return -1;
  }

  else {
    sin.sin_family = host->h_addrtype;
    bcopy(host->h_addr,  &(sin.sin_addr), host->h_length);
    sin.sin_port = toport;

    sock = socket (AF_INET, SOCK_STREAM, 0, 0);
    if (sock < 0) {
      strcat (*errstr, "socket: ");
     }

    else {
      if (connect (sock, &sin, sizeof (sin)) < 0) {
	strcat (*errstr, hostname);
	strcat (*errstr, ":");
	strcat (*errstr, service);
	strcat (*errstr, ":");
	close (sock);
	sock = -1;
      }
    }
  } 

  if (errno >= 0 && errno < sys_nerr)  {
    strcat (*errstr, sys_errlist[errno]);
  }

  if (sock >= 0)
    free(*errstr);
  return (sock);
}

int 
send_msg(sock,str,len) /* BEWARE! this routine writes into str beyond len! */
     int sock;
     char *str;
     int len;
{
  int wrote;
  char msg[100];

  if (sock < 0) return 0;
  if (len < 1) return 0;

  str[len] = CR;
  str[len+1] = LF;
  wrote = write(sock, str, len+2);
  if (wrote < 1) {
    sprintf (msg, "Unable to write %d chars to sock %d, errno %d",
	     len+2, sock, errno);
    ErrExit (msg);
  }
  else
    return 1;
}


int readline (char *line, int sz, int sock, int *idx)
{
  int rc;
  char msg[100];

  *idx = 0;
  line[*idx] = 0;
  do {
    rc = read(sock, line + *idx, 1);
    if (rc < 0) {
      sprintf (msg, "Error reading line from socket %d, errno %d",
	       sock, errno);
      ErrExit (msg);
    }
    (*idx)++;
  } while (*idx < sz-1 && line[*idx-1] != LF);
  line[*idx] = 0;
}


get_msg(sock, buff, buffsize)   /* read message off socket */
     int             sock;
     char           *buff;
     int             buffsize;
{
  int rc, len;
  char errmsg[100];

  len = 0;
  bzero (buff, buffsize);

  do {
    rc = read(sock, &buff[len], buffsize - len);
    if (rc < 0 ) {
      sprintf (errmsg, "get_msg: unable to read %d chars from sock %d, errno %d", buffsize-len, sock, errno);
      ErrExit(errmsg);
    }
    len = rc + len;
  } while (rc >= 0 && len <= buffsize && !terminator(&buff[len - EOM_LEN]));

  if (terminator (&buff[len - EOM_LEN]))
    len -= EOM_LEN;

  buff[len] = '\0';
  return (len);
}




logdebug (char *string)
{
  FILE *debugfp;
  long now;

  debugfp = fopen (DEBUGLOG, "a");
  if (debugfp != NULL) {
    now = time(0);
    fprintf (debugfp, "%d:%s:%s:%s", getpid(),string, remotehost,ctime(&now));
    fclose(debugfp);
  }
}

make_menu_line (char *line, char typ, char *title, char ityp,
		char *tiserver, char *tiport, char *nodeid, char *source,
		char *keys, char *filename)
{
  char *portnum;
  char *loginname;
  char *host;
  char *cp;
  
  if (typ == GO_TELNET || typ == GO_TN3270) {
    /* if it's a telnet/tn3270 type of thing, then
       need to change it to gopher style information */
    
    /* From TI server: keys format: <type><space><port>  E.g.: 8 23  */
    cp = index (keys, ' ');
    if (cp) portnum = cp+1;
    else portnum = "0";         /* DEFAULT TELNET PORT ACCORDING TO GOPHER */

    /* From TI server, for Telnet or TN3270 objects:
       filename format: <login><space><host>  E.g.: gopher dccs.upenn.edu */
    cp = rindex (filename, ' ');
    if (cp) {
      host = cp+1;
      *cp = 0;
      loginname = filename;
    }
    else {
      host = filename;
      loginname = "";
    }

    sprintf (line, "%c%s%c%s%c%s%c%s",
	     typ, title, GO_DELIM, loginname, GO_DELIM,
	     host, GO_DELIM, portnum);
  }
  else {
    sprintf (line, "%c%s%c%c%c%s%c%s%c%s%c%s%c%s%c%s",
	     typ, title, GO_DELIM,
	     ityp, I_DELIM, tiserver, I_DELIM, tiport, I_DELIM, nodeid,
	     I_DELIM, source,
	     GO_DELIM, gatewayhost, GO_DELIM, gatewayport);
  }
}


static void send_server_line(char *line)
{
  char *fields[TIMAXFIELDS];
  char *nodeid, *server, *port, *title;
  char *Title;
  
  parse_fields (TIDELIM, line, fields, TIMAXFIELDS);
  nodeid = fields[0];
  port = fields[1];
  title = fields[5];
  server = fields[6];

  Title = (char *) malloc (strlen(title) +  strlen (" TechInfo") + 1);
  strcpy (Title, title);
  strcat (Title, " TechInfo");

  make_menu_line (gophbuf, GO_MENU, Title, I_MENU, server, port, nodeid, "",
		  "", "");
  send_msg (gophsock, gophbuf, strlen(gophbuf));
}

send_ti_server_list()
{
  char *errstr;
  int length;
  int toread, numlines;
  int tisock;

  tisock = inet_connect (TISERVERS_HOST, TISERVERS_PORT, &errstr);
  if (tisock < 0) {
    sprintf(gophbuf, "Couldn't get list of TechInfo servers. %s (port %s)",
	    errstr, TISERVERS_PORT);
    ErrExit (gophbuf);
  }
  else {
    init_connect (tisock);

    sprintf (tibuf, "%s", TICMD_GETSERVERS);
    /*    sprintf (gophbuf, "ToTechinfo:%s", tibuf);
	  logdebug (gophbuf); */

    send_msg (tisock, tibuf, strlen(tibuf));
    readline (tibuf, sizeof(tibuf), tisock, &length);

    toread = atoi (tibuf);
    numlines = 0;
  
    while (numlines < toread) {
      readline (tibuf, sizeof(tibuf), tisock, &length);
      numlines++;
      send_server_line(tibuf);
    }
    sprintf (gophbuf, "Sent %d server lines", numlines);
    logdebug (gophbuf);
  }
}

/*
  Disgusting hack:
  The TI nodetype is NOT entirely in one place in the nodes' info string.
  Telnet session type files may actually be TN3270 things.
  Look in keys to get the scoop.
  */

whatis_type (char *flags, char *filename, char *intermedtype, char *gophertype,
	     char *keys)
{
  int iflags;

  iflags = atoi(flags);

  if (iflags & TI_GIFIMAGE) {
    *gophertype = GO_GIF;
    *intermedtype = I_GIFIMAGE;
  }
  else if (iflags & TI_TELNETSESS) {
    /* ASSUMPTION: TI Server uses the Gopher characters to indicate
       whether it's a Telnet or a TN3270 type of file */
    *gophertype = *keys;
    *intermedtype = *gophertype;
  }

  else if (strlen(filename) > 0) {
    *gophertype = GO_DOC;
    *intermedtype = I_DOC;
  }
  else {
    *intermedtype = I_MENU;
    *gophertype = GO_MENU;
  }

}



static int send_menu_item (char *line, char *tiserver, char *tiport, int minlevel)
{
  char *fields[TIMAXFIELDS];
  char selector[500];
  char *title, *filename, *nodeid, *level, *flags, *source, *keys;
  char ityp, gtyp;

  parse_fields (TIDELIM, line, fields, TIMAXFIELDS);
  level = fields[0];
  nodeid = fields[1];
  flags = fields[2];
  keys = fields[4];
  title = fields[5];
  filename = fields[8];
  source = fields[6];
  
  if (atoi(level) < minlevel)
    return 0;
  
  whatis_type (flags, filename, &ityp, &gtyp, keys);
  make_menu_line (gophbuf, gtyp, title, ityp, tiserver, tiport, nodeid,source,
		  keys, filename);
  send_msg (gophsock, gophbuf, strlen(gophbuf));
  return 1;
}

send_searchmenu_item (char *title, char *tiserver, char *tiport, 
		      char *nodeid)
{
  sprintf (gophbuf, "%cSearch %s%c%c%c%s%c%s%c%s%c%s%c%s",
	   GO_MENU, title, GO_DELIM,
	   I_SEARCHMENU, I_DELIM, tiserver, I_DELIM, tiport, I_DELIM, nodeid,
	   GO_DELIM, gatewayhost, GO_DELIM, gatewayport);
  send_msg (gophsock,gophbuf, strlen(gophbuf));
}

static int
send_searches (char *tiserver, char *tiport, char *nodeid)
{
  char *thename;

  if (strcasecmp(tiserver, LOCALTI_SERVER) == 0) 
    thename = LOCALTI_NAME;
  else
    thename = "TechInfo";

  if (strcmp(nodeid, "0") != 0 && strcmp(nodeid, "") != 0) {
    send_searchmenu_item ("current menu and below", tiserver,tiport,nodeid);
    send_searchmenu_item (thename, tiserver,tiport, "");
    return 2;
  }
  else {
    send_searchmenu_item (thename, tiserver,tiport, "");
    return 1;
  }
}


static void
send_search_item (char itype, char *tiserver, char *tiport, char *nodeid)
{
  char title [100];

  switch  (itype)
    {
    case I_KEYSRCH: strcpy (title, "Keyword Search"); break;
    case I_TITLESRCH: strcpy (title, "Title Search"); break;
    case I_DATESRCH: strcpy (title, "What's New Search (specify date as mm/dd/yy)"); break;
    case I_FULLTEXTSRCH: strcpy (title, "Full Text Search"); break;
    case I_SOURCESRCH: strcpy (title, "Source (owner) Search"); break;
      default : return;
    }

  sprintf (gophbuf, "%c%s%c%c%c%s%c%s%c%s%c%s%c%s",
	   GO_SEARCH, title, GO_DELIM,
	   itype, I_DELIM, tiserver,
	   I_DELIM, tiport, I_DELIM, nodeid,
	   GO_DELIM, gatewayhost, GO_DELIM, gatewayport);
  send_msg (gophsock, gophbuf, strlen(gophbuf));
}



static void
send_ti_nodelist (int tisock, char *tiserver, char *tiport, int *sent, int minlevel)
{
  int toread, numlines;
  int length;
  char *cp, *cp2;

  *sent = 0;
  readline (tibuf, sizeof(tibuf), tisock, &length);
  cp = index (tibuf, TIDELIM);
  if (cp) {
    cp++;
    for (cp2 = cp; *cp2 && *cp2 != '\n'; cp2++);
    *cp2 = 0;
    sprintf (gophbuf, "%s", cp); logdebug (gophbuf);
    if (cp2-cp > 0) 
      return;
  }
  toread = atoi (tibuf);
  numlines = 0;
  while (numlines < toread) {
    readline (tibuf, sizeof(tibuf), tisock, &length);
    numlines++;
    *sent += send_menu_item (tibuf, tiserver, tiport, minlevel);
  }
  readline (tibuf, sizeof(tibuf), tisock, &length);
}


static void send_sources_info (char *srvr)
{
  int tisock;
  char *errstr;

  if (*LOCAL_SOURCES_NODEID && *LOCALTI_SERVER &&
      strcmp (LOCALTI_SERVER, srvr) == 0) {

    /* connect to localti server, retrieve the document associated
       with local sources nodeid, translate it, send it to gopher client */

    tisock = inet_connect (LOCALTI_SERVER, LOCALTI_PORT, &errstr);
    if (tisock < 0) {
      sprintf(gophbuf, "Couldn't connect to Techinfo:%s (port %s)",
	    errstr, LOCALTI_PORT);
      ErrExit (gophbuf);
    }
    else {
      init_connect (tisock);
      if (*SOURCES_MSG_NODEID)
	send_ti_doc (tisock, gophsock, SOURCES_MSG_NODEID, 1);

      send_ti_doc (tisock, gophsock, LOCAL_SOURCES_NODEID, 1);
    }
  }
}




int send_sources_item (char *tiserver)
{
  /* Only offer this item if
     the server is the local one and there's a LOCAL SOURCES NODEID 
     */

  if ((strcasecmp(tiserver, LOCALTI_SERVER) == 0) && *LOCAL_SOURCES_NODEID) {
    sprintf (gophbuf, "%cSources (Providers) of Information\t%s%s\t%s\t%s",
	     GO_DOC, SOURCESINFO, tiserver, gatewayhost, gatewayport);
    send_msg (gophsock, gophbuf, strlen(gophbuf));
    return 1;
  }
  else
    return 0;
}


init_connect (int tisock)  /* call this after successful inet_estab */
{
  get_msg (tisock, tibuf, sizeof(tibuf)); /* get the banner */
  send_version(tisock);
  turnon_tiflags (tisock);
}

static void
create_tisearchcmd (char srchtyp, char *target, char *nodeid, char *tibuf)
{
  char *nodeidtouse;
  
  if (strlen(nodeid) > 0)
    nodeidtouse = nodeid;
  else
    nodeidtouse = "0"; /* if there was no nodeid given, then use 0 because
			  that is SUPPOSED to be the main menu */
  
  switch (srchtyp) {
  case I_KEYSRCH:
    sprintf (tibuf, TICMD_KEYSRCH_NODEID, target, nodeidtouse);
    break;
  case I_FULLTEXTSRCH:
    sprintf (tibuf, TICMD_FULLTEXTSRCH_NODEID, target, nodeidtouse);
    break;
  case I_TITLESRCH:
    sprintf (tibuf, TICMD_TITLESRCH_NODEID, target, nodeidtouse);
    break;

    /* TechInfo protocol:
       Change the order of target & nodeid for the SOURCE & DATE search! */

  case I_DATESRCH:
    sprintf (tibuf, TICMD_DATESRCH_NODEID, nodeidtouse, target);
    break;

  case I_SOURCESRCH:
    sprintf (tibuf, TICMD_SOURCESRCH_NODEID, nodeidtouse, target);
    break;
  }
}



static void do_ti_trans(char *request)
{
  char *fields[4];
  int tisock;
  char *tiserver, *tiport, *typ, *nodeid;
  char *errstr;
  char *target = NULL;
  char *cp;
  int numitems, numslash;
  
  switch (*request) {
  case I_KEYSRCH:
  case I_FULLTEXTSRCH:
  case I_TITLESRCH:
  case I_DATESRCH:
  case I_SOURCESRCH:
    /* expect selector-string TAB search-string */
    target = index (request, GO_DELIM);
    if (!target) {
      sprintf (gophbuf, "Didn't understand search request %s", request);
      ErrExit (gophbuf);
      return;
    }
    /* Replace the delimiter with end-of-string marker */
    *target = '\0';
    target++;

    if (*request == I_DATESRCH) {  /* target should be mm/dd/yy */
      for (numslash=0, cp = target; *cp; cp++) {
	if (*cp == '/') {
	  numslash++;
	  *cp = ':';
	}
      }
      if (numslash != 2) {
	sprintf (gophbuf, "Didn't understand date %s, expecting mm/dd/yy",
		target);
	ErrExit (gophbuf);
	return;
      }
    }

    /* fall through to parse_fields intentionally */

  case I_DOC:
  case I_MENU:
  case I_SEARCHMENU:
  case I_GIFIMAGE:
    
    parse_fields (I_DELIM, request, fields, 4);
    typ = fields[0];
    tiserver = fields[1];
    tiport = fields[2];
    nodeid = fields[3];
    break;
    
  default:
    sprintf (gophbuf, "Didn't understand file type %c", *request);
    ErrExit (gophbuf);
    return;
  }

/* okay, we've parsed the Intermed. token... */
/*  fprintf (stderr, "serv=%s, port=%s, nodeid=%s, target=%s.\n",
	  tiserver, tiport, nodeid, target);  */

  if (*typ == I_SEARCHMENU) {
    send_search_item (I_KEYSRCH, tiserver, tiport, nodeid);
    send_search_item (I_DATESRCH, tiserver, tiport, nodeid);
    send_search_item (I_TITLESRCH, tiserver, tiport, nodeid);
    send_search_item (I_FULLTEXTSRCH, tiserver, tiport, nodeid);
    send_search_item (I_SOURCESRCH, tiserver, tiport, nodeid);
  }
  
  else {
    tisock = inet_connect (tiserver, tiport, &errstr);
    if (tisock < 0) {
      sprintf(gophbuf, "Couldn't connect to Techinfo:%s (port %s)",
	      errstr, tiport);
      ErrExit (gophbuf);
    }
    else {
      init_connect (tisock);
      
      switch (*typ) {
      case I_KEYSRCH:
      case I_FULLTEXTSRCH:
      case I_DATESRCH:
      case I_SOURCESRCH:
      case I_TITLESRCH:
	create_tisearchcmd (*typ, target, nodeid, tibuf);
	sprintf (gophbuf, "ToTechinfo:%s", tibuf); logdebug (gophbuf);
	
	send_msg (tisock, tibuf, strlen(tibuf));
	send_ti_nodelist(tisock, tiserver, tiport, &numitems, 0);
	sprintf (tibuf, "Sent %d items", numitems);
	logdebug (tibuf);
	break;
	
      case I_DOC:
      case I_GIFIMAGE:
	send_ti_doc (tisock, gophsock, nodeid, *typ != I_GIFIMAGE);
	break;
	
      case I_MENU:
	sprintf (tibuf, TICMD_GETMENU, nodeid);
	/*      sprintf (gophbuf, "ToTechinfo:%s", tibuf); logdebug (gophbuf); */
	send_msg (tisock, tibuf, strlen(tibuf));
	
	send_ti_nodelist (tisock, tiserver, tiport, &numitems, 1);
	numitems += send_searches (tiserver, tiport, nodeid);
	
	if (send_sources_item (tiserver))
	  numitems = numitems + 1;
	
	sprintf (tibuf, "Sent %d items", numitems);
	logdebug (tibuf);
	break;
      }
    }
  }
}


terminalbased_ti()
{
  sprintf (gophbuf, "%cMassachusetts Institute of Technology%c%ctechinfo.mit.edu%c0",
	   GO_TELNET, GO_DELIM, GO_DELIM,GO_DELIM);
  send_msg (gophsock, gophbuf, strlen(gophbuf));

  sprintf (gophbuf, "%cUniversity of Pennsylvania%c%cpenninfo.upenn.edu%c0",
	   GO_TELNET, GO_DELIM, GO_DELIM,GO_DELIM);
  send_msg (gophsock, gophbuf, strlen(gophbuf));

  sprintf (gophbuf, "%cMississippi State%cMSUINFO%cmsuinfo.msstate.edu%c0",
	   GO_TELNET, GO_DELIM, GO_DELIM,GO_DELIM);
  send_msg (gophsock, gophbuf, strlen(gophbuf));

}




send_message_file (char *filename)
{
  FILE *fp;
  char line[100], *cp;

  fp = fopen (filename, "r");
  if (fp == NULL) {
    sprintf (gophbuf, "Unable to read msg file %s", filename);
    ErrExit (gophbuf);
  }

  do {
    if (fgets (line,sizeof(line)-1,fp) == NULL)
      break;
    else {
      cp = index (line, '\n');
      if (cp) {
	write (gophsock, line, cp-line);
	write (gophsock, "\r\n", 2);
      }
      else
	write (gophsock, line, strlen(line));
    }
  } while (1);
}


send_version(int tisock)
{
  sprintf (tibuf, TICMD_VERSION, GWVERSION);
  send_msg (tisock, tibuf, strlen(tibuf));
  get_msg(tisock, gophbuf, sizeof(gophbuf));
}


turnon_tiflags (int tisock)
{
  sprintf (tibuf, TICMD_SHOWFLAGS);
  send_msg (tisock, tibuf, strlen(tibuf));
  get_msg (tisock, gophbuf, sizeof(gophbuf));
}

set_remote_hostname ()
{
  struct sockaddr_in sname;
  struct hostent *host;
  int namelen;

  namelen = sizeof(sname);

  if (getpeername (gophinsock, &sname, &namelen)) {
    if (errno == ENOTSOCK) 
      sprintf (remotehost, "STDIN");
    else
      sprintf (remotehost, "getpeername-errno-%d", errno);
  }

  else {
    host = gethostbyaddr(&sname.sin_addr, sizeof(sname.sin_addr), AF_INET);
    if (!host)
      sprintf (remotehost, "gethostbyaddr-errno-%d", errno);
    else
      sprintf (remotehost, "%s", host->h_name);
  }
}

main (int argc, char *argv[])
{
  char request[1000];
  char *ptr;
  int port;
  
  setreuid (RUNTIME_UID, RUNTIME_UID);
  if (getuid() == 0 || geteuid() == 0) {
    fprintf (stderr, "\r\n.\r\n");
    /* should send a message to syslog daemon */
    exit (-1);
  }
  
  gophsock =  fileno(stdout);
  gophinsock = fileno(stdin);
  set_remote_hostname ();
  
/*  logdebug ("Start"); */
  
  if (argc < 3) {
    sprintf (gophbuf, "Usage: %s <hostname> <port>", argv[0]);
    ErrExit(gophbuf);
  }
  gatewayhost = argv[1];
  if (getportbyn_or_num (argv[2], &port) < 0) {
    sprintf (gophbuf, "Unknown service port argv2: %s", argv[2]);
    ErrExit (gophbuf);
  }
  port = ntohs(port);
  sprintf (gatewayport, "%d", port);
  
  getreq (request, sizeof(request), gophinsock);
  sprintf (gophbuf, "Request:%s", request);
  logdebug (gophbuf);
  
  for (ptr = request; *ptr && isspace(*ptr) ; ptr++);
  if (*ptr == 0) {                 /* is it a blank line? */

    if (strlen(LOCALTI_SERVER) > 0) {
      sprintf (request, "%c%c%s%c%s%c%s",
	       I_MENU, I_DELIM,  LOCALTI_SERVER, I_DELIM, LOCALTI_PORT, I_DELIM, 
	       LOCALTI_MAINMENU);
      do_ti_trans(request);
    }

    sprintf (gophbuf, "%cWorld Wide TechInfo%c%s%c%s%c%s",
	     GO_MENU, GO_DELIM, WORLDWIDETI, GO_DELIM,
	     gatewayhost, GO_DELIM, gatewayport);
    send_msg (gophsock, gophbuf, strlen(gophbuf));
    
    if (strlen(LOCALGOPHTITLE) > 0) {
      sprintf (gophbuf, "%c%s%c%s%c%s%c%s",
	       GO_MENU, LOCALGOPHTITLE, GO_DELIM, LOCALGOPHERPATH, GO_DELIM,
	       LOCALGOPHERSERVER, GO_DELIM, LOCALGOPHERPORT);
      send_msg (gophsock, gophbuf, strlen(gophbuf));
    }
    
    sprintf (gophbuf, "%cWorld Wide Gopher%c1/Other Gopher and Information Servers%cgopher.tc.umn.edu%c70",
	     GO_MENU, GO_DELIM, GO_DELIM,GO_DELIM);
    send_msg (gophsock, gophbuf, strlen(gophbuf));
  }
  
  else if (!strcmp(request, WORLDWIDETI)) {
    send_ti_server_list();
    sprintf (gophbuf, "%cTerminal-based TechInfo services%c%s%c%s%c%s",
	     GO_MENU, GO_DELIM, TERMBASEDTI, GO_DELIM,
	     gatewayhost, GO_DELIM, gatewayport);
    send_msg (gophsock, gophbuf, strlen(gophbuf));
  }


  else if (!strncmp(request, SOURCESINFO, strlen(SOURCESINFO)))
    send_sources_info(request + strlen(SOURCESINFO));
  
  else if (!strcmp(request, TERMBASEDTI))
    terminalbased_ti();
  
  else                            /* should be a techinfo menu request */
    do_ti_trans(request);
  
  /* this is the normal exit */
/*  write (gophsock, "\r\n.\r\n", 5); */
  write (gophsock, ".\r\n", 3);
/*  logdebug ("End"); */
  exit (0);
}

