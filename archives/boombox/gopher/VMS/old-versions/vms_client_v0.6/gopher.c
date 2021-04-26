/*Last Modified:   2-DEC-1992 14:22:35.18, By: MARK */
/* gopher.c
 *
 * Part of the Internet Gopher program, copyright (C) 1991
 * University of Minnesota Microcomputer Workstation and Networks Center
 *
 * See the README file for information about the Gopher program.
 */

#include "gopher.h"

/* Connect_to_gopher performs a connection to socket 'service' on host
 * 'host'.  Host can be a hostname or ip-address.  If 'host' is null, the
 * local host is assumed.   The parameter full_hostname will, on return,
 * contain the expanded hostname (if possible).  Note that full_hostname is a
 * pointer to a char *, and is allocated by connect_to_gopher()
 *
 * Errors:
 *
 * -1 get service failed
 *
 * -2 get host failed
 *
 * -3 socket call failed
 *
 * -4 connect call failed
 */

static struct sockaddr_in Server;
static char sBuf[16];
struct hostent *HostPtr;


int connect_to_gopher(Hostname, ThePort)
  char *Hostname;
  int  ThePort;
{
     int iSock = 0;
     
     if (Hostname == '\0') {
	  gethostname(sBuf, 16);
	  Hostname = sBuf;
     }

     if ((Server.sin_addr.s_addr = inet_addr(Hostname)) == -1) {
	  if (HostPtr = gethostbyname(Hostname)) {
	       bzero((char *) &Server, sizeof(Server));
	       bcopy(HostPtr->h_addr, (char *) &Server.sin_addr, HostPtr->h_length);
	       Server.sin_family = HostPtr->h_addrtype;
	  } else
	       return (-2);
     } else
	  Server.sin_family = AF_INET;

     Server.sin_port = (unsigned short) htons(ThePort);

     if ((iSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	  return (-3);

     setsockopt(iSock, SOL_SOCKET, ~SO_LINGER, 0, 0);
     setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, 0, 0);
     setsockopt(iSock, SOL_SOCKET, SO_KEEPALIVE, 0, 0);

     if (connect(iSock, (struct sockaddr *) &Server, sizeof(Server)) < 0) {
	  close(iSock);
	  return (-4);
     }
     return(iSock);
}


/*
** Feed a gopher takes a line of text and parses it.
** It returns a 0 when successful and -1 when it isn't
*/

int feed_a_gopher(j, sockfd)
  int j;
  int sockfd;
{
     char *cPtr = NULL;
     char foo[255];
     

     readfield(sockfd, foo, 255);

     Gopher[j].sFileType = foo[0];

     /** Get the kind of file from the first character **/
     /** Filter out files that we can't deal with **/

     switch (Gopher[j].sFileType) {
       case A_FILE:
       case A_DIRECTORY:
       case A_CSO:
       case A_INDEX:
       case A_TELNET:
       case A_SOUND:
       case A_EVENT:
/*       case A_CALENDAR:   not ready for prime time yet */
	  break;
       case A_EOI:
	  return(0);
     default:
	return(-1);  
     }


     /** Suck off the User Displayable name **/
     strncpy(Gopher[j].sTitle, foo + 1,79);
     
     /** Suck off the Pathname **/
     readfield(sockfd, Gopher[j].sPath, 150);

     /** Suck off the hostname **/
     if (readfield(sockfd, Gopher[j].sHost, 80) == 0)
	  return(-1);

     if (readline(sockfd, foo, 255)==0)
	  ; /** Panic! **/

     Gopher[j].iPort = 0;

     /** Get the port number **/
     Gopher[j].iPort = atoi(foo);

     return(0);
}


/*
**  Feed_gophers simply loops through feed_a_gopher until it reaches its
**  limit (MAXGOPHERS) or runs out of new data.
**
*/

int feed_gophers(sockfd)
  int sockfd;
{
     char ZesTmp[512];
     int  j;

     
     for (j = 0; (j < MAXGOPHERS); j++)
     {
	  /*bzero(ZesTmp, sizeof(GopherStruct));*/
	  ZesTmp[0] = '\0';
/*	  if (readline(sockfd, ZesTmp, 512) == 0)
	       break;*/

	  if (feed_a_gopher(j, sockfd)!=0) {
	       j = j -1;  /** feed a gopher failed, try again **/
	       readline(sockfd, ZesTmp, 512); /** Get rid of rest of line **/
	  }
	  if (Gopher[j].sFileType == '.')
	       break;
     }
     
     return(j);
} 

/*
** Open a connection to another host
*/

do_telnet(ZeGopher)
  GopherStruct *ZeGopher;
{
     char *sMessage1;
     char *sMessage2;
     char *sTelCmd; 
     char telcom[64];
     short int ch;
     int ret_status, row, col;

     $DESCRIPTOR(line1, "Warning!!!!!, you are about to leave the Internet");
     $DESCRIPTOR(line2, "Gopher program and connect to another host.");
     $DESCRIPTOR(line3, "If you get stuck, press the control key and the ^ ");
     $DESCRIPTOR(line4, "key, and then type q or c.");
     $DESCRIPTOR(line5, "Press return to connect: ");
     $DESCRIPTOR(d_sMessage1, sMessage1);
     $DESCRIPTOR(d_sMessage2, sMessage2);
     $DESCRIPTOR(d_sTelCmd, sTelCmd);

     /* retrieve the gopher information for the telnet command*/

     sMessage1 = (char *) malloc( 128 );
     sMessage2 = (char *) malloc( 128 );
     sTelCmd = (char *) malloc( 128 );

     strncpy( sMessage1, "\0", 128 );
     strncpy( sMessage2, "\0", 128 );
     strncpy( sTelCmd, "\0", 128 );

     d_sMessage1.dsc$a_pointer = sMessage1;
     d_sMessage2.dsc$a_pointer = sMessage2;
     d_sTelCmd.dsc$a_pointer = sTelCmd;

     ret_status = smg$erase_display( &DisplayId );

     row = 1;
     col = 17;
     ret_status = smg$put_chars( &DisplayId, &line1, &row, &col );
     row = row + 1;
     ret_status = smg$put_chars( &DisplayId, &line2, &row, &col );
     row = row + 1;
     ret_status = smg$put_chars( &DisplayId, &line3, &row, &col );
     row = row + 1;
     ret_status = smg$put_chars( &DisplayId, &line4, &row, &col );

     sprintf(sMessage1,"Now connecting to %s", ZeGopher->sHost);
     d_sMessage1.dsc$w_length = strlen( sMessage1 );

     if (ZeGopher->sPath[0] != '\0')
	  sprintf(sMessage2, "Use the account name \"%s\" to log in",ZeGopher->sPath );
     else
	  strcpy(sMessage2,"\0");

     d_sMessage2.dsc$w_length = strlen( sMessage2 );

     row = 12;
     ret_status = smg$put_chars( &DisplayId, &d_sMessage1, &row, &col);
     row = row + 1;
     ret_status = smg$put_chars( &DisplayId, &d_sMessage2, &row, &col);
     row = 23;
     ret_status = smg$put_chars( &DisplayId, &line5, &row, &col);

     while (ch != SMG$K_TRM_CR )
	ret_status = smg$read_keystroke( &KeyboardId, &ch );

     if (ZeGopher->iPort != 0)
        sprintf( sTelCmd, "%s/port=%d %s", TelnetCommand, ZeGopher->iPort,
		 ZeGopher->sHost); 
     else
        sprintf( sTelCmd, "%s %s", TelnetCommand, ZeGopher->sHost);

     ret_status = smg$erase_display( &DisplayId );
     system(sTelCmd);

}


/*
** do_index gets keywords from the user to search for.  It returns
** it to the calling process.  This storage is volotile. Callers should
** make a copy if they want to call do_index multiple times.
*/

char* do_index(ZeGopher)
  GopherStruct *ZeGopher;
{
     static char inputline[WHOLELINE];

     inputline[0] = '\0';
     GetOneOption("Index word(s) to search for: ", inputline);
     if (strlen(inputline) == 0)
	  return(NULL);
     else
	  return(inputline);
}


/*
 * fork off a sound process to siphon the data across the net.
 * So the user can listen to tunage while browsing the directories.
 */

do_sound(ZeGopher)
  GopherStruct *ZeGopher;
{
     int i=0, iLength, sockfd;
     char tmpfilename[HALFLINE];
     FILE *tmpfile;
     char inputline[512];
     char sTmp[5];
     BOOLEAN Waitforchld = FALSE;

     sTmp[0] = '\0';

     if ((sockfd = connect_to_gopher(ZeGopher->sHost, ZeGopher->iPort)) <0) {
	  check_sock(sockfd, ZeGopher->sHost);
	  return;
     }

     /*** Get rid of that darn Banner line **

     iLength = readline(sockfd, inputline, MAXLINE);
*/

     /** Send out the request **/

     writestring(sockfd, ZeGopher->sPath);
/*     writestring(sockfd, "\tfetch");*/
     writestring(sockfd, "\r\n");

     /** Check the result of the command.  See if we screwed up... **/

/*     iLength = readline(sockfd, inputline, MAXLINE);
     if (inputline[0]=='-') {
	  CursesErrorMsg(inputline + 2);
	  return;
     }

*/
     /** Okay, it's cool, we can fork off **/


     if (SOUNDCHILD != 0)
	  Waitforchld = TRUE;


/*     if ( (SOUNDCHILD = fork()) < 0) */
	  ;/* Fork Error */
     
/*     else if (SOUNDCHILD == 0) { */ /* Child Process */
/*	  wait(SIGCHLD); */
/*	  suck_sound(sockfd);
	  exit(0);
     }
*/     
     /* Parent Process */
     
     close(sockfd);
}
	       

/*
 * this procedure just retrieves binary data from the socket and
 * pumps it into a "play" process.
 */

#define BUFSIZE 1400  /* A pretty good value for ethernet */

suck_sound(sockfd)
  int sockfd;
{
     FILE *Play;
     int j;
     char buf[BUFSIZE];

     if (PlayCommand[0] == '\0') {
	  /*** Hey! no play command, bummer ***/
	  CursesErrorMsg("Sorry, this machine doesn't support sounds");

	  return(0);
     }

/*     Play = popen(PlayCommand, "w");
*/	  
     
     while(1) {
          j = read(sockfd, buf, BUFSIZE);
	  
	  if (j == 0)
	       break;
	  
	  fwrite(buf, 1, j, Play);
     }
}

/*
 *	Replacement "strncasecmp" routine
 *  copied from GNU GCC for VMS and modified for a case insensitive compare.
 */

/*
 * Compare strings (at most n bytes):  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

int
strncasecmp(s1, s2, n)
register char *s1, *s2;
register n;
{
   while (--n >= 0 && ((*s1==*s2) || ((*s1+32)==*s2) || ((*s1-32)==*s2))){
     *s2++;
     if (*s1++ == '\0')
	return(0);
   }

/*   return(n<0 ? 0 : *s1 - *--s2);  */
   return(n<0 ? 0 : 1 );

}


char *
strcasestr(inputline, match)
  char *inputline;
  char *match;
{
     int matchlen=0;
     int i, inlen;

     matchlen = strlen(match);
     inlen = strlen(inputline);

     for(i=0; i<inlen; i++) {
	  if (strncasecmp(inputline+i, match, matchlen)==0)
	       return(inputline+i);
     }

     return(NULL);
}


/*
 * Replace the searched words with backspaces and underline characters.
 */

void
Boldit(inputline, outputline, MungeSearchstr)
  char *inputline, *outputline, *MungeSearchstr;
{
     char words[20][40];  /** A reasonable guess **/
     char c;
     int numchars, lowwordnum, wordcount, i, j;
     char *cp, *lowword;

     bzero(outputline, 512); /** non portable!!!! ***/
     
     while (isspace(*MungeSearchstr)) /** Strip off spaces **/
	  MungeSearchstr++;
	  
     for (wordcount=0; wordcount<20; wordcount++) {

	  while (isspace(*MungeSearchstr)) /** Strip off spaces **/
	       MungeSearchstr++;
	  
	  numchars = sreadword(MungeSearchstr, words[wordcount], 40);
	  MungeSearchstr += numchars;
	  if (numchars == 0)
	       break;
	  if (strcmp(words[wordcount], "and")==0 ||
	      strcmp(words[wordcount], "or")==0 ||
	      strcmp(words[wordcount], "not")==0) {
	       words[wordcount][0] = '\0';
	       wordcount--;
	  }
     }


     /** Find the first word in the line **/

     while (*inputline!='\0') {
	  lowword = NULL;

	  for (i=0; i< wordcount; i++) {
	       cp = strcasestr(inputline, words[i]);
	       if (cp != NULL)
		    if (cp < lowword || lowword == NULL) {
			 lowword = cp;
			 lowwordnum = i;
		    }
	  }

	  if (lowword == NULL) {
	       strcpy(outputline, inputline);
	       return;
	  }
	  else {
	       strncpy(outputline, inputline, lowword - inputline);
	       outputline += (lowword - inputline);
	       inputline = lowword;
	       
	       strcpy(outputline, sGHighlighton);  /** This is dangerous!! **/
	       outputline += strlen(sGHighlighton);

	       strncpy(outputline, inputline, strlen(words[lowwordnum]));
	       inputline += strlen(words[lowwordnum]);
	       outputline += strlen(words[lowwordnum]);

	       strcpy(outputline, sGHighlightoff);
	       outputline += strlen(sGHighlightoff);

	       /*** Add the underline characters **
	       for (j=0; j < strlen(words[lowwordnum]); j++) {
		    *outputline = '_';
		    outputline++;
		    *outputline = '\010';
		    outputline++;
		    *outputline = *inputline;
		    outputline++;
		    inputline++;
	       }**/
	       
	  }
     }
}


/**
*** Show file takes an open socket connection, writes it to a file
*** and passes it to your favorite pager.
**/

showfile(ZeGopher)
  GopherStruct *ZeGopher;
{
     int i=0, iLength, sockfd;
     char tmpfilename[HALFLINE];
     FILE *tmpfile;
     char inputline[512];
     char outputline[512];
     char sTmp[5];

     sTmp[0] = '\0';

     if ((sockfd = connect_to_gopher(ZeGopher->sHost, ZeGopher->iPort)) <0) {
	  check_sock(sockfd, ZeGopher->sHost);
	  return;
     }

     /*** Get rid of that darn Banner line ***/

/*     iLength = readline(sockfd, inputline, MAXLINE);
*/

     /** Send out the request **/

     writestring(sockfd, ZeGopher->sPath);
     writestring(sockfd, "\r\n");

     /** Check the result of the command.  See if we screwed up... **/

/*     iLength = readline(sockfd, inputline, MAXLINE);
     if (inputline[0]=='-') {
	  CursesErrorMsg(inputline + 2);
	  return;
     }
  */   

     /** Open a temporary file **/

     sprintf(tmpfilename, "sys$scratch:gopher.%d",getpid());

     if ((tmpfile = fopen(tmpfilename, "w")) == NULL)
	  fprintf(stderr, "Couldn't make a tmp file!\n"), exit(-1);

     for(;;) {
	  iLength = readline(sockfd, inputline, 512);
	  outputline[0] = '\0';
	  if (iLength == 0)
	       break;

	  ZapCRLF(inputline);
	  
	  
	  /*** Ugly hack ahead..... ***/

          if (ZeGopher->sFileType == A_CSO) {
	       if (inputline[0] == '2')
		    break;

	       if ((inputline[0] >= '3') && (inputline[0] <= '9'))  {
		    fprintf(tmpfile, "%s\n", ZeGopher->sPath);
		    fprintf(tmpfile, "%s\n", inputline+4);
		    break;
	       }
	       if (inputline[0] == '-') {
		    if (inputline[5] != i)
			 fprintf(tmpfile, "-------------------------------------------------------\n");
		    i = inputline[5];
		    fprintf(tmpfile, "%s\n", inputline+7);
	       }
	  }

          if (ZeGopher->sFileType == A_FILE || 
	      ZeGopher->sFileType == A_EVENT) {
	       if ((inputline[0] == '.') && (inputline[1] == '\0'))
		    break;
	       else {
		    /*** Underline searched words, except and, or and not ***/
		    if (Searchstring != NULL) {
			 Boldit(inputline, outputline, Searchstring);
		    }
		    else
			 strcpy(outputline, inputline);
		    fprintf(tmpfile, "%s\n", outputline);
	       }
	  }
     }

     fprintf(tmpfile, "\012 \n\n");  /** Work around a bug in xterm n' curses*/
     (void)fclose(tmpfile);

     display_file(tmpfilename);

     /** Good little clients clean up after themselves..**/

/*     if (remove(tmpfilename)!=0)
	  fprintf(stderr, "Couldn't remove %s!!!\n");
*/

}


/*
** Pushgopher takes a GopherThing pointer and adds it to it's stack.
**
** Ick this must be fixed!
*/

pushgopher(ZeGopher)
  GopherStruct *ZeGopher;
{

     OldGopher[iLevel].sFileType      = ZeGopher->sFileType;

     strcpy(OldGopher[iLevel].sTitle, ZeGopher->sTitle);
     strcpy(OldGopher[iLevel].sHost,  ZeGopher->sHost);
     strcpy(OldGopher[iLevel].sPath,  ZeGopher->sPath);
     OldGopher[iLevel].iPort = ZeGopher->iPort;

     iLevel ++;
}

/*
** PopGopher fills in a GopherThing structure with the head of
** the stack.  If the stack is empty, popgopher returns a -1
*/

int
popgopher(ZeGopher)
  GopherStruct *ZeGopher;
{

     if (iLevel == 0)
	  return(-1);

     iLevel --;

     ZeGopher->sFileType    = OldGopher[iLevel].sFileType;
     strcpy(ZeGopher->sTitle, OldGopher[iLevel].sTitle);
     strcpy(ZeGopher->sHost, OldGopher[iLevel].sHost);
     strcpy(ZeGopher->sPath, OldGopher[iLevel].sPath);
     ZeGopher->iPort = OldGopher[iLevel].iPort;

     return(0);
}


void check_sock(sockfd, host)
  int sockfd;
  char *host;
{
     char DispString[WHOLELINE];
     char Response[HALFLINE];
     int ret_status;

     Response[0] = '\0';
     
#ifdef DEBUG
     switch(sockfd)  {
     case -4:
	  fprintf(stderr, "connect call failed ");
	  break;
     case -3:
	  (void)fprintf(stderr, "socket call failed");
	  break;
     case -2:
	  (void)fprintf(stderr, "get host failed");
	  break;
     case -1:
	  (void)fprintf(stderr, "get service failed");
	  break;
     }
     
     fprintf(stderr, "to host %s, port %d\n",
	     Gopher[iNum].sHost, Gopher[iNum].iPort);
     exit(-1);
#endif

     if (sockfd <0) {
	  sprintf(DispString, "Cannot connect to host %s: ", host);
	  CursesErrorMsg(DispString);
	  ret_status = smg$erase_display( &DisplayId );
	  exit( 1 );
     }
}


/**************
** This bit of code catches control-c's, it cleans up the curses stuff.
*/
void*
controlc()
{
     char buf[HALFLINE];

     sprintf(buf, "sys$scratch:gopher.%d", getpid());
     if (remove(buf) < 0)
	  fprintf(stderr, "could not remove %s\n", buf);

     exit(0);
}


/**************
** This bit of code catches window size change signals
**/

void*
sizechange()
{
#ifdef  TIOCGWINSZ
     static struct      winsize zewinsize;        /* 4.3 BSD window sizing */
#endif
     
#ifdef  TIOCGWINSZ
     if (ioctl(0, TIOCGWINSZ, (char *) &zewinsize) == 0) {
	  LINES = zewinsize.ws_row;
	  COLS = zewinsize.ws_col;
     } else {
#endif
	  /* code here to use sizes from termcap */
#ifdef  TIOCGWINSZ
     }
/*     MainWindow = initscr(); */
     
     scline(iMenuLines, -1, 1, Gopher, SavedTitle);

#endif
}



/**********
**
** Set up all the global variables.
**
***********/

void
Initialize()
{
     int err, ret_status;
     char *cp;
     char termname[HALFLINE];
     static char terminal[1024];
     static char capabilities[1024];   /* String for cursor motion */
     
     static char *ptr = capabilities;  /* for buffering         */

     /** Get the pager command **/
     
     if (getenv("PAGER") == NULL)
	  strcpy(PagerCommand, PAGER_COMMAND);
     else
	  strcpy(PagerCommand, getenv("PAGER"));

     strcpy(PrinterCommand, PRINTER_COMMAND);
     strcpy(TelnetCommand, TELNET_COMMAND);
     strcpy(PlayCommand, PLAY_COMMAND);

     if (getenv("EDITOR") == NULL)
	  strcpy(EditorCommand, "eve");
     else
	  strcpy(EditorCommand, getenv("EDITOR"));

     /*** Get the terminal type ***/

     if (getenv("TERM") == NULL)
	  fprintf(stderr, "I don't understand your terminal type\n"), exit(-1);
     
     if (strcpy(termname, getenv("TERM")) == NULL)
	  fprintf(stderr, "I don't understand your terminal type\n"),exit(-1);

     strcpy(sGClearscreen, "");

     strcpy(sGAudibleBell,"\007");

     strncpy(sGHighlighton, "\0", 20 );
     strncpy(sGHighlightoff, "\0", 20 );
     strcpy(sGHighlighton, "\033\133\067\155" );
     strcpy(sGHighlightoff, "\033\133\155" );

     /*** Init MainWindow ****/

     ret_status = smg$erase_display( &DisplayId,0,0,0,0);

     Gopher = (GopherStruct *) malloc(500 * sizeof(GopherStruct));

     /*** Make a signal handler for window size changes ***/

#ifdef SIGWINCH
     if (signal(SIGWINCH, sizechange)==(void*)-1)
	  perror("signal died:\n"), exit(-1);

#endif

     if (signal(SIGINT, controlc) == (void*)-1)
	  perror("signal died:\n"), exit(-1);

}


static char *GlobalOptions[] =  
{"Pager Command", "Print Command", "Telnet Command", "Sound Command", "Editor"};

void
SetOptions()
{
     static char Response[MAXRESP][MAXSTR];
     
     strcpy(Response[0], PagerCommand);
     strcpy(Response[1], PrinterCommand);
     strcpy(Response[2], TelnetCommand);
     strcpy(Response[3], PlayCommand);
     strcpy(Response[4], EditorCommand);

     Get_Options("Internet Gopher Options", "", 5, GlobalOptions, Response);

     strcpy(PagerCommand, Response[0]);
     strcpy(PrinterCommand, Response[1]);
     strcpy(TelnetCommand, Response[2]);
     strcpy(PlayCommand, Response[3]);
     strcpy(EditorCommand, Response[4]);
}


/* This should be a generalized stack type.  This is icky for now... */
static int SavedLinenum[MAXGOPHERS];
static int SavedLinePtr = 0;

main(argc, argv)
  int argc;
  char *argv[];
{
     int iLine=0;
     int iNum=0;
     BOOLEAN bDone = FALSE;
     char sTmp[40];
     GopherStruct TmpGopher, *Gopherp;
     short int TypedChar;
     /*** for getopt processing ***/
     int c, tmplen;
     extern char *optarg;
     extern int optind;
     int errflag, ret_status, disp_row, disp_col, p_row, p_col;

     disp_row = 24;
     disp_col = 80;
     p_row = 1;
     p_col = 1;
     SavedLinenum[SavedLinePtr] = 1;

     TmpGopher.iPort = GOPHER_PORT;
     TmpGopher.sFileType = A_DIRECTORY;   
     strcpy(TmpGopher.sTitle, ROOT_DIRECTORY);
     TmpGopher.sPath[0] = '\0';
     strcpy(TmpGopher.sHost, DEFAULT_HOST);
     sTmp[0] = '\0';
     errflag = 0;

/*   check the command line for a hostname and port number */

     if ( ( argc != 1 ) && ( argc != 3 ) ) {
	printf("Usage: Gopher [hostname] [port number]\n");
	exit(1);
     }

     if ( argc == 3 ) {
       tmplen = strlen( TmpGopher.sHost );
       strncpy( TmpGopher.sHost, "\0", tmplen );
       strcpy( TmpGopher.sHost, argv[1] ); /** copy the hostname **/
       TmpGopher.iPort = atoi( argv[2] );  /** get the port number **/
     }

/*
 * set up the SMG stuff
 */

     ret_status = smg$create_pasteboard( &PasteId, 0, 0, 0, 0);
     ret_status = smg$create_virtual_display( &disp_row, &disp_col, 
                   &DisplayId, 0, 0, 0);
     ret_status = smg$create_virtual_keyboard( &KeyboardId );
     ret_status = smg$paste_virtual_display( &DisplayId, &PasteId, 
                    &p_row, &p_col );
     if ( ret_status != 1 )
       printf("oops! %d\n",ret_status);

     /*** Set up global variables, etc. ***/

     Initialize();

     iLine = 1;

     process_request(&TmpGopher);

     while (bDone == FALSE)
     {

	  iLine = GetMenu(iMenuLines, Gopher, SavedTitle, &TypedChar, iLine);

	  switch(TypedChar)
	  {
	  case SMG$K_TRM_CR:
	  case SMG$K_TRM_LF:
	  case SMG$K_TRM_RIGHT:
	       /*** Select the designated item ***/
	       iNum = iLine - 1;
	       if (Gopher[iNum].sFileType == A_DIRECTORY || 
		   Gopher[iNum].sFileType == A_INDEX) {
		    SavedLinenum[++SavedLinePtr] = iLine;
		    iLine=1;
	       }
	       process_request(&(Gopher[iNum]));
	       break;
	       
	  case SMG$K_TRM_NULL_CHAR:
	       /*** What the heck? ***/
	       popgopher(&TmpGopher);
	       popgopher(&TmpGopher);
	       CursesErrorMsg("Strange Error occurred!");
	       process_request(&TmpGopher);
	       break;
	       
	  case SMG$K_TRM_LOWERCASE_U:
	  case SMG$K_TRM_UPPERCASE_U:
	  case SMG$K_TRM_LEFT:
	       /*** Go up a directory level ***/
	       iNum=0;
	       popgopher(&TmpGopher);
	       popgopher(&TmpGopher);
	       iLine = SavedLinenum[(SavedLinePtr ==0) ? 0:SavedLinePtr--];
	       process_request(&TmpGopher);
	       break;
	       
	  case SMG$K_TRM_LOWERCASE_S:
	  case SMG$K_TRM_UPPERCASE_S:
	       /*** Save the thing in a file ***/
	       break;
	       
	  case SMG$K_TRM_LOWERCASE_N:
	  case SMG$K_TRM_UPPERCASE_N:
	       /*** Create a new file ***/
/*	       createobject();
	       popgopher(&TmpGopher);  /** Get the altered directory **/
/*	       process_request(&TmpGopher);*/
	       break;

	  case SMG$K_TRM_LOWERCASE_D:
	  case SMG$K_TRM_UPPERCASE_D:
/*	       deleteobject(&(Gopher[iLine-1]));*/
/*	       popgopher(&TmpGopher);  /** Get the altered directory **/
/*	       process_request(&TmpGopher);*/
	       break;

	  case SMG$K_TRM_UPPERCASE_M:
	  case SMG$K_TRM_LOWERCASE_M:
	       iNum = 0;
	       while (popgopher(&TmpGopher) != -1)
		    ;
	       process_request(&TmpGopher);
	       break;

	  case SMG$K_TRM_LOWERCASE_Q:
	  case SMG$K_TRM_UPPERCASE_Q:
	       /*** Quit the program ***/
	       bDone = TRUE;
	       ret_status = smg$erase_display( &DisplayId );
	       break;
	       
	  case SMG$K_TRM_LOWERCASE_O:
	  case SMG$K_TRM_UPPERCASE_O:
	       /*** Change various program things ***/
	       SetOptions();
	       break;
		      
	  case SMG$K_TRM_EQUAL:
	       iNum = iLine - 1;
	       describe_gopher(&(Gopher[iNum]));
	       break;

	  case SMG$K_TRM_QUESTION_MARK:
	       /*** Display help file ***/
 	       display_file(GOPHERHELP);
	       break;
	  default :
	       break;

	  }
     }
}     


void
Load_Index(ZeGopher)
  GopherStruct *ZeGopher;
{
     Draw_Status("Searching Text...");

     Load_Index_or_Dir(ZeGopher, Searchstring);
}

void
Load_Dir(ZeGopher)
  GopherStruct *ZeGopher;
{
     Searchstring= NULL;
     Load_Index_or_Dir(ZeGopher, NULL);
}


void
Load_Index_or_Dir(ZeGopher, Searchmungestr)
  GopherStruct *ZeGopher;
  char *Searchmungestr;
{
     int sockfd;
     int i, length;
     char sTmp[10];
     char sOldTitle[WHOLELINE];
     static char inputline[MAXLINE];


     sTmp[0]= '\0';

     strcpy(sOldTitle, SavedTitle);

     if ((sockfd = connect_to_gopher(ZeGopher->sHost, ZeGopher->iPort)) <0)
	  check_sock(sockfd, ZeGopher->sHost);
     else {
	  if (ZeGopher->sFileType == A_DIRECTORY) {
	       writestring(sockfd, ZeGopher->sPath);
	       writestring(sockfd, "\r\n");
	  }
	  else if (ZeGopher->sFileType == A_INDEX) {
	       writestring(sockfd, ZeGopher->sPath);
	       writestring(sockfd, "\t");
	       writestring(sockfd, Searchmungestr);
	       writestring(sockfd, "\r\n");
	  }

	  if (ZeGopher->sFileType == A_INDEX)
	     sprintf(SavedTitle, "%s: %s", ZeGopher->sTitle, Searchmungestr);
          else
	     strncpy(SavedTitle, ZeGopher->sTitle, WHOLELINE);

          SavedTitle[WHOLELINE-1] = '\0';

	  pushgopher(ZeGopher); /* Risky; sometimes they push back */
	  
	  i = feed_gophers(sockfd); /* Get next level's gophers*/
	  if (i <= 0) {
	       CursesErrorMsg("Nothing available.");
	       popgopher(ZeGopher);
	       strcpy(SavedTitle, sOldTitle);
	       SavedLinePtr--;
	  }
	  else
	       iMenuLines = i;
	  
     }
     i = close(sockfd);
}


void
process_request(ZeGopher)
  GopherStruct *ZeGopher;
{
     int sockfd;
     char *cp;

     switch(ZeGopher->sFileType) {
     case -1:
	  break;

     case A_EVENT:
/*	  HandleEvent(ZeGopher);*/
	  break;

     case A_FILE:
	  Draw_Status("Receiving Text...");
	  showfile(ZeGopher);
	  break;
	  
     case A_DIRECTORY:
	  Draw_Status("Receiving Directory...");
	  Load_Dir(ZeGopher);
	  break;
		    
     case A_TELNET:
	  do_telnet(ZeGopher);
	  break;
	  
     case A_INDEX:
	  Draw_Status("Searching Text...");
	  Searchstring = do_index(ZeGopher);
	  if (Searchstring != NULL)
	     Load_Index(ZeGopher);
	  break;
	  
     case A_CSO:
	  do_cso(ZeGopher);
	  break;

     case A_SOUND:
	  Draw_Status("Receiving Sound...");
	  do_sound(ZeGopher);
	  break;
	  
     }
}

int
describe_gopher(ZeGopher)
  GopherStruct *ZeGopher;
{
     char tmpfilename[40];
     FILE *tmpfile;

     sprintf(tmpfilename,"sys$scratch:gopher.%d", getpid());
     if ((tmpfile = fopen(tmpfilename, "w")) == NULL)
	   fprintf(stderr, "Couldn't make a tmp file!\n"), exit(-1);

     fprintf(tmpfile,"Name=%s\nType=%c\nPort=%d\nPath=%s\nHost=%s\n\n",
	     ZeGopher->sTitle,
	     ZeGopher->sFileType,
	     ZeGopher->iPort,
	     ZeGopher->sPath,
	     ZeGopher->sHost);

     fclose(tmpfile);

     display_file(tmpfilename);
/*     if (remove(tmpfilename) != 0)
	  fprintf(stderr, "Couldn't remove %s!!!\n",tmpfilename);
*/

}
