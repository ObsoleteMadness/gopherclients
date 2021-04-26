/*Last Modified:   8-MAR-1993 10:46:18.40, By: MARK */
/* Derived from an 
 * Example of server using TCP protocol
 * pp 284-5 Stevens UNIX Network Programming
 */


#include "vmsgopherd.h"
void LOGGopher();

char Zehostname[128];
int GopherPort = GOPHER_PORT;

extern int vmserrno;
extern int socket_errno;

main(argc, argv)
  int 	argc;
  char 	*argv[];
{
     int sockfd;
     int newsockfd;
     int clilen;
     int childpid;
     int i, argcounter;
     char *vms_errno_string();
     int ret_status, write_len, equiv_len, lnm_index;
     unsigned int lnm_attrib;
     unsigned char acmode;
     char *LOGFile, *SecurityFile, *equiv_string, *datadir_def;

     struct sockaddr_in cli_addr;
     struct sockaddr_in serv_addr;

     /*** for getopt processing ***/
     int c;
     extern char *optarg;
     extern int optind;
     int errflag =0;
     char *colon_pos;

     $DESCRIPTOR( d_gopherroot, "GOPHER_ROOT" );
     $DESCRIPTOR( d_datadef, "DATA_DIRECTORY_DEF" );
     $DESCRIPTOR( d_logfile, "GOPHER_LOGFILE" );
     $DESCRIPTOR( d_SecurityFile, "GOPHER_SecurityFile" );
     $DESCRIPTOR( d_datadir, "GOPHER_DATADIR" );
     $DESCRIPTOR( d_tcpport, "GOPHER_TCPPORT" );
     $DESCRIPTOR( d_stable, "LNM$SYSTEM_TABLE" );
     $DESCRIPTOR( d_jtable, "LNM$JOB" );
     $DESCRIPTOR( d_equiv, equiv_string );

     acmode = PSL$C_EXEC;
     lnm_attrib = LNM$M_CONCEALED + LNM$M_TERMINAL;
     pname = argv[0];
     strcpy(Data_Dir, DATA_DIRECTORY);

/*      do logical name translations here to get port, data_dir, logfile,
 *      and security file.  How do I determine if I got here from inetd?
 */

     LOGFile = ( char * ) malloc( 255 );
     SecurityFile = ( char * ) malloc( 255 );
     equiv_string = ( char * ) malloc( 255 );
     datadir_def = ( char * ) malloc( 255 );

/*
 * Now we set up to translate the logical names if they exist for the logfile,
 * the security file, the data directory, and the port.
 */

     strncpy( LOGFile, "\0", 255 );
     strncpy( SecurityFile, "\0", 255 );
     strncpy( equiv_string, "\0", 255 );
     strncpy( datadir_def, "\0", 255 );
 
     lnmlst[0].length = 255;
     lnmlst[0].code = LNM$_STRING;
     lnmlst[0].bufadr = equiv_string;
     lnmlst[0].retlen = &equiv_len;

     lnmlst[1].length = 4;
     lnmlst[1].code = LNM$_INDEX;
     lnmlst[1].bufadr = &lnm_index;
     lnmlst[1].retlen = 0;

     lnmlst[2].length = 0;
     lnmlst[2].code = 0;
     lnmlst[2].bufadr = 0;
     lnmlst[2].retlen = 0;

     lnm_index = 0;
     equiv_len = 0;
/*
 * Translate the logical name for the log file if it exists
 */
     ret_status = sys$trnlnm( 0, &d_stable, &d_logfile, &acmode, &lnmlst );

     if ( ret_status == SS$_NORMAL )
	strncpy( LOGFile, equiv_string, equiv_len );

     strncpy( equiv_string, "\0", 255 );
     equiv_len = 0;
/*
 * Translate the logical name for the security file if it exists
 */
     ret_status = sys$trnlnm( 0, &d_stable, &d_SecurityFile, &acmode, &lnmlst );

     if ( ret_status == SS$_NORMAL )
	strncpy( SecurityFile, equiv_string, equiv_len );

     strncpy( equiv_string, "\0", 255 );
     equiv_len = 0;
/*
 * Translate the logical name for the data directory if it exists
 */
     ret_status = sys$trnlnm( 0, &d_stable, &d_datadir, &acmode, &lnmlst );

     if ( ret_status == SS$_NORMAL )
	strncpy( Data_Dir, equiv_string, equiv_len );

     strncpy( equiv_string, "\0", 255 );
     equiv_len = 0;
/*
 * Translate the logical name for the tcpport to use if it exists
 */
     ret_status = sys$trnlnm( 0, &d_stable, &d_tcpport, &acmode, &lnmlst );

     if ( ret_status == SS$_NORMAL ) {
	d_equiv.dsc$w_length = equiv_len;
        d_equiv.dsc$a_pointer = equiv_string;
	ret_status = ots$cvt_ti_l( &d_equiv, &GopherPort, 4, 1 );
     }

     strncpy( equiv_string, "\0", 255 );
     equiv_len = 0;
/*
 * Translate the logical name to create the logical name for the root
 * directory of the gopher data
 */
     ret_status = sys$trnlnm( 0, &d_stable, &d_datadef, &acmode, &lnmlst );

     if ( ret_status != SS$_NORMAL ) {
       strncpy( equiv_string, "\0", 255 );
       equiv_len = 0;
       ret_status = sys$trnlnm( 0, &d_stable, &d_gopherroot, &acmode, &lnmlst );
     }
 
     lnmlst[0].length = 4;
     lnmlst[0].code = LNM$_ATTRIBUTES;
     lnmlst[0].bufadr = &lnm_attrib;
     lnmlst[0].retlen = 0;

     lnmlst[1].length = equiv_len;
     lnmlst[1].code = LNM$_STRING;
     lnmlst[1].bufadr = equiv_string;
     lnmlst[1].retlen = 0;

     lnmlst[2].length = 0;
     lnmlst[2].code = 0;
     lnmlst[2].bufadr = 0;
     lnmlst[2].retlen = 0;

     ret_status = sys$crelnm( 0, &d_jtable, &d_gopherroot, &acmode, &lnmlst );

     if (!RunFromInetd) {
	  printf("Data directory is %s\n", Data_Dir);
	  printf("Port is %d\n", GopherPort);
     }

     if (*LOGFile != '\0' && !RunFromInetd)
	  fprintf(stdout,"Logging to File %s\n", LOGFile);

     if (*SecurityFile != '\0' && !RunFromInetd)
	  fprintf(stdout,"Using Security file %s\n", SecurityFile);

     if (chdir(Data_Dir)) {
	  fprintf(stderr, "Cannot change to data directory!! %s \n",Data_Dir);
	  exit(-1);
     }

     /** Open up the LOGFile and the SecurityFile
         **Warning!** we have to do this over because the daemon_start
         function closes all fd's

	 This part just checks to see if the files exist...
      **/

     if (*LOGFile != '\0') {
	  LOGFileHandle = fopen(LOGFile, "a");
	  
	  if (LOGFileHandle == NULL) {
	       printf("Can't open the logfile: %s\n", LOGFile);
	       exit(-1);
	  }

	  fclose(LOGFileHandle);
     }

     if (*SecurityFile != '\0') {
	  SECFileHandle = fopen(SecurityFile, "r");

	  if (SECFileHandle == NULL) {
	       fprintf(stderr,"Can't open the security file: %s\n", SecurityFile);
	       exit(-1);
	  }
	  fclose(SECFileHandle);
     }
     
     /*** Hmmm, does this look familiar? :-) ***/

     if (LOGFile[0] != '\0') {
	  LOGFileHandle = fopen(LOGFile, "a");
	  
	  if (LOGFileHandle == NULL) {
	       fprintf(stderr,"Can't open the logfile: %s\n", LOGFile);
	       exit(-1);
	  }
          fclose(LOGFileHandle);
	  LOGGopher(-1, "Starting gopher daemon\n");
     }

     if (*SecurityFile != '\0') {
	  SECFileHandle = fopen(SecurityFile, "r");

	  if (SECFileHandle == NULL) {
	       fprintf(stderr,"Can't open the security file: %s\n", SecurityFile);
	       exit(-1);
	  }
     }

     /* Work out our fully-qualified name, for later use */
     
     if (gethostname(Zehostname, 128) != 0) {
	  strcpy(Zehostname, "<<no-name>>");
     }
     strcat(Zehostname, DOMAIN_NAME);
     
     if (RunFromInetd) {
	  while(do_command(0)!=0);	/* process the request */
	  exit(0);
     }

     /** Open a TCP socket (an internet stream socket **/
     
     if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	  err_dump("server: can't open stream socket");
     
     /*
      * Bind our local address so that the client can send to us
      */
     
     bzero((char *) &serv_addr, sizeof(serv_addr));
     serv_addr.sin_family 		= AF_INET;
     serv_addr.sin_addr.s_addr 	= htonl(INADDR_ANY);
     serv_addr.sin_port		= htons(GopherPort);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) <0)
	  err_dump("server: can't bind local address");
     
     listen(sockfd, 5);
     
     for ( ; ; ) {
	  /*
	   * Wait for a connection from a client process.
	   * This is an example of a concurrent server.
	   */
	  
	  clilen = sizeof(cli_addr);
	  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
			     &clilen);

	  /*** weird.. with this thing here our gethostaddrs work. 
            without it, it fails..... why?
           ***/
	  
	  if (newsockfd < 0)
	       err_dump("server: accept error");
	  
          while(do_command(newsockfd)!=0);{}	/* process the request */

#ifdef WOLLONGONG
	  netclose(newsockfd); 		/* parent process */
#endif

#ifdef MULTINET
	  socket_close(newsockfd); 		/* parent process */
#endif 

#ifdef UCX
	  close(newsockfd); 		/* parent process */
#endif

     }
}

/*
 *
 *  Code stolen from nntp.....
 *
 * inet_netnames -- return the network, subnet, and host names of
 * our peer process for the Internet domain.
 *
 *      Parameters:     "sock" is our socket, which we don't need.
 *                      "sin" is a pointer to the result of
 *                      a getpeername() call.
 *                      "host_name"
 *                      is filled in by this routine with the
 *                      corresponding ASCII names of our peer.
 *      Returns:        Nothing.
 *      Side effects:   None.
 */
inet_netnames(sock, sin, host_name)
        int                     sock;
        struct sockaddr_in      *sin;
        char                    *host_name;
{
        u_long                  net_addr;
        struct hostent          *hp;
        struct netent           *np;

        net_addr = inet_netof(sin->sin_addr);   /* net_addr in host order */
        np = getnetbyaddr(net_addr, AF_INET);

        hp = gethostbyaddr((char *) &sin->sin_addr.s_addr,
                sizeof (sin->sin_addr.s_addr), AF_INET);

        if (hp != NULL)
                (void) strcpy(host_name, hp->h_name);
        else
                (void) strcpy(host_name, inet_ntoa(sin->sin_addr));
}

/*
 * This finds the current peer and the time and  jams it into the
 * logfile (if any) and adds the message at the end
 */
void
LOGGopher(sockfd, message)
  int sockfd;
  char *message;
{
     struct sockaddr sa;
     int             length;
     static char     host_name[256];
     time_t          Now;
     char *cp;

     host_name[0] = '\0';

     LOGFileHandle = fopen(LOGFile, "a");

     if (LOGFileHandle != NULL) {
     
	  if (sockfd > -1) {
	       length = sizeof (sa);
	       getpeername(sockfd, &sa, &length);
	       inet_netnames(sockfd, &sa, host_name);
	  }

	  time(&Now);
	  cp = (char *) ctime(&Now);
	  ZapCRLF(cp);

	  fprintf(LOGFileHandle, "%s %s : %s\n", cp, host_name, message);
	  fflush(LOGFileHandle);
          fclose(LOGFileHandle);
     }
}

process_mailfile(sockfd, Mailfname)
  int sockfd;
  char *Mailfname;
{
     FILE *Mailfile;
     char Zeline[MAXLINE];
     char outputline[MAXLINE];
     char Title[MAXLINE];
     long Startbyte=0, Endbyte, Bytecount;

     Mailfile = fopen(Mailfname, "r");

     if (Mailfile == NULL) {
	  writestring(sockfd, "- Cannot access file\r\n.\r\n");
	  return;
     }

     while (fgets(Zeline, MAXLINE, Mailfile) != NULL) {

	  if (strncmp(Zeline, "Subject: ", 9)==0) {
	       strcpy(Title, Zeline + 9);
	       ZapCRLF(Title);
	       if (DEBUG)
		    fprintf(stderr, "Found title %s", Title);
	  }
	  
	  if (strncmp(Zeline, "From ", 5) == 0) {
	       Endbyte = Bytecount;

	       if (Endbyte != 0) {
		    sprintf(outputline, "0%s\tR%d-%d-%s\t%s\t%d\r\n", 
			    Title, Startbyte, Endbyte, Mailfname, 
			    Zehostname, GopherPort);
		    writestring(sockfd, outputline);
		    
		    Startbyte=Bytecount;
		    *Title = '\0';
	       }
	  }

	  Bytecount += strlen(Zeline);
     }

     if (*Title != '\0') {
	  sprintf(outputline, "0%s\tR%d-%d-%s\t%s\t%d\r\n", 
		  Title, Startbyte, Endbyte, Mailfname, 
		  Zehostname, GopherPort);
	  writestring(sockfd, outputline);
     }	  

     writestring(sockfd, ".\r\n");
}

boolean
Can_Read(sockfd)
  int sockfd;
{
     struct sockaddr sa;
     int             length;
     char            host_name[256];
     char *cp;
     boolean         MatchReturn;
     char            inputline[MAXLINE];

     if (*SecurityFile == '\0')
	  return(TRUE);

     length = sizeof (sa);
     getpeername(sockfd, &sa, &length);
     inet_netnames(sockfd, &sa, host_name);

     fseek(SECFileHandle, 0, 0);  /** Go to beginning of Security File **/
     
     while (fgets(inputline, MAXLINE, SECFileHandle) != NULL) {
	  ZapCRLF(inputline);

	  MatchReturn = TRUE;
	  cp = inputline;

	  if (*cp == '!') {
	       MatchReturn = FALSE;
	       cp++;
	  }
	  
	  if (isdigit(*cp)) {
	       /** Internet address x.x.x.x ..***/
	       /** Check for a match from the beginning **/
	       if (strstr(host_name,cp) == host_name)
		    return(MatchReturn);
	  }
	  
	  else if (*cp == '*') {
	       return(MatchReturn);
	  }

	  else if (*cp != '#') {
	       /*** Not a comment, must be a host name **/
	       if ((strstr(host_name, cp) + strlen(cp)) == (host_name + strlen(host_name))) {
		    return(MatchReturn);
	       }
	  }
     }

     /*** Hmmm, didn't find a match...  Let em have it***/
	  
     return(TRUE);
}

int
do_command(sockfd)
  int sockfd;
{
     char inputline[MAXLINE];
     int length;		/* Length of the command line */
     char logline[MAXLINE];

     /*** Reopen the security file ***/

     if (*SecurityFile != '\0') {
	  SECFileHandle = fopen(SecurityFile, "r");

	  if (SECFileHandle == NULL) {
	       LOGGopher(sockfd, "Security File dissappeared!");
	       exit(-1);
	  }
     }

     GDinit(&SortDir);

     length = readline(sockfd, inputline, MAXLINE); /** Get the line **/

     if (length <= 0)
	  err_dump("getcommand: readline error");
     
     ZapCRLF(inputline);

     /*** With the funky new capability system we can just check the
          first letter, end decide what the object refers to. ***/

     switch (inputline[0]) {
     case '\0':
     case '\t':

	  /*** The null capability, so it's not a file, probably wants
	       to talk to a directory server ***/

	  /*** we'll just do a "list" of the root directory, with no user
	       capability.  ***/

	  listdir(sockfd, "gopher_root:[000000]");
	  LOGGopher(sockfd, "Root Connection");
	  return(0);
	  break;

     case '0':
	  /*** It's a generic file capability ***/
	  printfile(sockfd, inputline+1, 0, -1);

	  /*** Log it ***/
	  strcpy(logline, "retrieved file ");
	  strcat(logline, inputline+1);
	  LOGGopher(sockfd, logline);
	  break;

     case '1':
	  /*** It's a directory capability ***/
	  listdir(sockfd, inputline+1);

	  /** Log it **/
	  strcpy(logline, "retrieved directory ");
	  strcat(logline, inputline+1);
	  LOGGopher(sockfd, logline);

	  break;

     case '7':
	  /*** It's an index capability ***/
	  Do_IndexTrans(sockfd, inputline+1);

	  /*** Log it **/
	  strcpy(logline, "did a search ");
	  strcat(logline, inputline+1);
	  LOGGopher(sockfd, logline);

	  break;

     case 's':
	  /*** It's a sound capability ***/
	  echosound(sockfd, inputline+1);

	  /* Log it */
	  strcpy(logline, "retrieved sound ");
	  strcat(logline, inputline);
	  LOGGopher(sockfd, logline);
	  break;

     case 'm':
	  /*** This is an internal identifier ***/
	  /*** The m paired with an Objtype of 1 makes a mail spool file
	       into a directory.
	  ***/

	  process_mailfile(sockfd, inputline + 1);
	  break;

     case 'R':
	  /*** This is an internal identifier ***/
	  /*** The R defines a range  ****/
	  /*** The format is R<startbyte>-<endbyte>-<filename> **/
     {
	  int startbyte, endbyte;
	  char *cp, *oldcp;

	  cp = ( char * ) strchr(inputline+1, '-');
	  
	  if (cp == NULL) {
	       writestring(sockfd, "Range specifier error.\r\n.\r\n");
	       break;
	  }
	  
	  *cp = '\0';
	  startbyte = atoi(inputline+1);
	  oldcp = cp+1;

	  cp = ( char * ) strchr(oldcp, '-');
	  
	  if (cp == NULL) {
	       writestring(sockfd, "Range specifier error.\r\n.\r\n");
	       break;
	  }

	  *cp = '\0';
	  endbyte = atoi(oldcp);
	  oldcp = cp + 1;
	  if (DEBUG)
	       fprintf(stderr, "Start: %d, End: %d  File: %s\n", startbyte, endbyte, oldcp);

	  printfile(sockfd, oldcp, startbyte, endbyte);
	  
	  break;
     }
     default:
	  /*** Hmmm, must be an old link... Let's see if it exists ***/

	  switch (isadir(inputline) == 1) {
	  case -1:
	       /* no such file */
	       return(1);
	  case 0:
	       /* it's a file */
	       printfile(sockfd, inputline, 0, -1);
	       
	       /* Log it... */
	       strcpy(logline, "retrieved file ");
	       strcat(logline, inputline);
	       LOGGopher(sockfd, logline);

	       break;
	  case 1:
	       /* it's a directory */
	       listdir(sockfd, inputline);

	       /* Log it */
	       strcpy(logline, "retrieved directory ");
	       strcat(logline, inputline);
	       LOGGopher(sockfd, logline);

	       break;
	  }
     }

     return(0);
}

/*
 * Returns true (1) for a directory
 *         false (0) for a file
 *         -1 for anything else
 */
boolean
isadir(path)
  char *path;
{
     struct stat buf;
     int result;

     result = stat(path, &buf);

     if (result != 0)
	  return(-1);
     
#ifdef BROKENDIRS

#define S_ISDIR(m)      (((m)&S_IFMT) == S_IFDIR)
#define S_ISREG(m)      (((m)&S_IFMT) == S_IFREG)

#endif

     if (S_ISDIR(buf.st_mode))
	  return(1);
     else if (S_ISREG(buf.st_mode))
	  return(0);
     else
	  return(-1);
}

/*
** filedate does a stat and returns the last modified date
*/
time_t
filedate(pathname)
  char *pathname;
{
     struct stat buf;
     int result;

     result = stat(pathname, &buf);

     if (result != 0)
	  return(0);
     
     return(buf.st_ctime);
}

void
process_old_link(sockfd, filename)
  int sockfd;
  char *filename;
{
     FILE *DotFile;
     int            DoneFlags[5];
     static char    inputline[1024];

     static GopherStruct Gopherp;
     char *cp;

     if (DEBUG == TRUE)
	  printf("Calling process_old_link with sockfd: %d, file: %s\n", sockfd, filename);

     if ((DotFile = fopen(filename, "r"))==NULL)
	  return;
     
     DoneFlags[0] = DoneFlags[1] = DoneFlags[2]
	  = DoneFlags[3] = DoneFlags[4] = 0;
     
     for (;;) {
	  for (;;) {
	       cp = fgets(inputline, 1024, DotFile);
	       if (inputline[0] != '#' || cp == NULL)
		    break;
	  }
	  
	  /*** Test for EOF ***/

	  if (cp==NULL)
	       break;

	  /*** Test for the various field values. **/
	  
	  if (strncmp(inputline, "Type=", 5)==0) {
	       GSsetType(&Gopherp, inputline[5]);
	       DoneFlags[0] = 1;
	  }
	  
	  if (strncmp(inputline, "Name=", 5)==0) {
	       ZapCRLF(inputline);
	       GSsetTitle(&Gopherp, inputline+5);
	       DoneFlags[1] = 1;
	  }
	  
	  if (strncmp(inputline, "Path=", 5)==0) {
	       ZapCRLF(inputline);
	       GSsetPath(&Gopherp, inputline+5);
	       DoneFlags[2] = 1;
	  }
	  
	  if (strncmp(inputline, "Host=", 5)==0) {
	       ZapCRLF(inputline);
	       GSsetHost(&Gopherp, inputline+5);
	       DoneFlags[3] = 1;
	  }
	  
	  if (strncmp(inputline, "Port=", 5)==0) {
	       ZapCRLF(inputline);
	       GSsetPort(&Gopherp, atoi(inputline+5));
	       DoneFlags[4] = 1;
	  }
	  
	  if (strncmp(inputline, "Numb=", 5)==0) {
	       ZapCRLF(inputline);
	       GSsetNum(&Gopherp, atoi(inputline+5));
	  }

	  /** Check to see if we've got all the parameters **/
	  /** If we do, then send out the line ***/
	  
	  if ((DoneFlags[0] + DoneFlags[1] +
	       DoneFlags[2] + DoneFlags[3] +
	       DoneFlags[4]) == 5) {
	       
	       DoneFlags[0] = DoneFlags[1] = DoneFlags[2]
		    = DoneFlags[3] = DoneFlags[4] = 0;

	       GDaddGS(&SortDir, &Gopherp);
	  }
     }
     fclose(DotFile);
}

/*
 * This function tries to find out what type of file a pathname is.
 * It then fills in the VAR type variables ObjType & ServerPath with
 * corresponding info.
 */
char
Getfiletypes(newpath, ObjType, ServerPath)
  char *newpath;
  char **ObjType;
  char **ServerPath;
{
     boolean dirresult;
     int Zefilefd;
     static char Zebuf[64];
     char *cp;
     static char Selstr[256];
     
     if (ServerPath != NULL)	     /* Don't overwrite existing path if any */
	  *ServerPath = Selstr; 

     dirresult = isadir(newpath);

     if (dirresult == -1)             /** Symlink or Special **/
	  return('3');

     if (dirresult == 1) {
	  *ObjType = "1";
	  *Selstr = '1';
	  strcpy(Selstr +1, newpath);
	  return;
     }
     else if (dirresult == 0) {	      /** Some kind of data file.... */
	  
	  if ((Zefilefd = open(newpath, O_RDONLY)) < 0) {
	       return('3');
	  }
	  
	  bzero(Zebuf, 64);
	  read(Zefilefd, Zebuf, 64);
	  close(Zefilefd);
	  
	  /*** Check the first few bytes for sound data ***/
	  
	  cp = Zebuf;

	  if (*cp++ == '.' && *cp++ == 's' && 
	      *cp++ == 'n' && *cp++ == 'd') {
	       *ObjType = "s";
	       *Selstr = 's';
	       strcpy(Selstr+1, newpath);
	       
	       return;
	  }

	  /*** Check and see if it's mailbox data ***/
	  
	  cp = Zebuf;

	  if (*cp++ == 'F' && *cp++ == 'r' &&
	      *cp++ == 'o' && *cp++ == 'm' &&
	      *cp++ == ' ') {
	       *ObjType = "1";
	       *Selstr = 'm';
	       strcpy(Selstr+1, newpath);
	       
	       return;
	  }

	  /*** Check for uuencoding data ***/

	  cp = Zebuf;
	  
	  if (*cp++ == 'b' && *cp++ == 'e' && *cp++ == 'g' &&
	      *cp++ == 'i' && *cp++ == 'n')  {
	       *ObjType = "6";
	       *Selstr = '6';
	       strcpy(Selstr+1, newpath);
	       
	       return;
	  }
	  
	  /*** The default is a generic text file ***/

	  *ObjType = "0";
	  *Selstr = '0';
	  strcpy(Selstr + 1, newpath);
	  return;
     }
     return('3');  /*** we shouldn't be able to get here..... ***/
}

/*
** This function lists out what is in a particular directory.
** it also outputs the contents of link files.
**
** Ack is this ugly.
*/
void
listdir(sockfd, pathname)
  int sockfd;
  char *pathname;
{
     struct FAB wild_fab;	/* Used in wildcard search */
     struct NAM wild_nam;	/* Used in conjunction with wild_fab */

     char template[] = "*.*;*"; /* default template */
     char fullname[256];	/* Input file spec given by user */
     char expanded[256];	/* filespec after logical expansion */
     char result[256];		/* result from search */
     char *pointer1, *pointer2, *pointer3, *pathp2, *sidep1;
     char *lbracketp, *rbracketp;
     int result2_len, side_len;
     register status;

     int            i;
     char           *cp, ch;

     char           sidename[256];
     char           filename[256];
     char           newpath[512];

     FILE           *SideFile;
     
     static char    portnum[16];
     GopherStruct   Gopherp;
     char	    *Typep, *Pathp;

/* Initialize the wildcard FAB and NAM */
     wild_fab = cc$rms_fab;
     wild_nam = cc$rms_nam;

     wild_fab.fab$l_fna = fullname;
     wild_fab.fab$b_fac = FAB$M_GET;
     wild_fab.fab$l_fop = FAB$V_NAM;
     wild_fab.fab$l_nam = &wild_nam;
     wild_fab.fab$l_dna = template;
     wild_fab.fab$b_dns = strlen(template);

     wild_nam.nam$l_esa = expanded;
     wild_nam.nam$b_ess = 255;
     wild_nam.nam$l_rsa = result;
     wild_nam.nam$b_rss = 255;

     strcpy(fullname, pathname);
     wild_fab.fab$b_fns = strlen(fullname);
     if (((status = SYS$PARSE(&wild_fab)) &1) != 1) {
       fprintf(stderr, "Error %d on parse\n",status);
       fprintf(stderr, "pathname: %s\n",pathname);
       writestring(sockfd, "- Cannot access that directory\r\n.\r\n");
       return;
     }
    
     pointer1 = ( char * ) strchr( pathname, '[' );
     if ( (pointer1 != pathname) && 
          (strncmp(pathname, "gopher_root", 11) != 0) ) {
	  writestring(sockfd, "- Cannot access that directory\r\n.\r\n");
          return;
     }

     if (chdir(pathname)<0) {
/*	  fprintf(stderr, "error: %d %s\n",vmserrno,vms_errno_string()); */
          writestring(sockfd, "- Cannot access that directory\r\n.\r\n");
          return;
     }

     sprintf(portnum, "%d", GopherPort);

     for ( ; ; ) {
       if ((( status = SYS$SEARCH(&wild_fab)) &1) != 1) {
         if ( (status == RMS$_NMF) || (status == RMS$_FNF) )
           break;
	 else {
	   fprintf(stderr, "Error %d on search\n",status);
	   writestring(sockfd, "- There were no files, or we had some other error on the search\r\n.\r\n");
	   return;
	 }
       }

	  expanded[wild_nam.nam$b_esl] = 0;
	  result[wild_nam.nam$b_rsl] = 0;
	  pointer1 = ( char * ) strchr(result,']') + 1;
	  pointer2 = ( char * ) strchr(result,';');
	  result2_len = pointer2 - pointer1;
	  pointer3 = ( char * ) malloc(result2_len + 1);
	  strncpy(pointer3, "\0", result2_len + 1);
	  strncpy(pointer3, pointer1, result2_len);

          strcpy(newpath, pathname);
	  sidep1 = ( char * ) strchr(pathname, ']');
	  side_len = sidep1 - pathname;
	  strncpy(sidename, "\0", 256 );
	  strncpy(sidename, pathname, side_len);
	  strcat(sidename, ".cap]\0" );
          strcat(newpath, pointer3);
	  strcpy(filename, pointer3);
	  strcat(sidename, pointer3);
	  free(pointer3);

	  /** Only chew list out files that don't start with a dot **/
	  /** or aren't named dev, usr, bin, etc, or core.         **/

	  if ((filename[0] == '.') && isadir(filename)==0) {
	       /*** This is a link file, let's process it ***/
	       process_old_link(sockfd, filename);
	  }


	  if ((filename[0] != '.')
	      && strcmp(filename, "bin") != 0 && 
	         strcmp(filename, "dev") != 0 &&
	         strcmp(filename, "usr") != 0 &&
	         strcmp(filename, "core")!= 0 &&
	         strcmp(filename, "etc") != 0)   {
	       
	       /** Check to see if there's a set-aside file with more info ***/
	       /** But first initialize the Gopherstruct **/

	       GSinit(&Gopherp);

	       GSsetHost(&Gopherp, Zehostname);
	       GSsetPort(&Gopherp, GopherPort);
	       Typep = Pathp = NULL;
	       

	       if ((SideFile = fopen(sidename, "r"))!=0) {
		    if (DEBUG == TRUE)
			 printf("Side file name: %s\n", sidename);
		    Process_Side(SideFile, &Gopherp);
	       }

	       if (GSgetType(&Gopherp)=='\0' && *GSgetPath(&Gopherp) == '\0') {
		    Getfiletypes(newpath, &Typep, &Pathp);

		    if (*Typep =='3')
			 break;
		    
		    GSsetType(&Gopherp, Typep[0]);

		    if ( *Typep == '1' ) {
			pointer2 = ( char * ) strrchr(result,'.');
			result2_len = pointer2 - pointer1;
/*			gopher20 = 0;
			gopher20 = strncmp(pathname,"gopher_root:[000000]",20);  */
			lbracketp = ( char * ) strchr(pathname, '[');
			if ((strncmp(pathname,"gopher_root",11) == 0 ) &&
			    (strncmp(lbracketp,"[000000]",8) == 0 ) )
			  pathp2 = ( char * ) strchr(pathname, '[');
			else
			  pathp2 = ( char * ) strchr(pathname, ']');
			strncpy(newpath, "\0", 511);
			strncpy(newpath, "1", 1);
			/*if (strncmp(pathname,"gopher_root:[000000]",20) == 0 ) */
			if ((strncmp(pathname,"gopher_root",11) == 0 ) &&
			    (strncmp(lbracketp,"[000000]",8) == 0 ) )
			  strncat(newpath,pathname, (pathp2 - pathname) + 1);
			else  {
			  strncat(newpath,pathname, (pathp2 - pathname) );
			  strcat(newpath,".");
			}
			strncat(newpath,pointer1,result2_len);
			strcat(newpath,"]\0");
			GSsetPath(&Gopherp, newpath);
		    }			
		    else
		        GSsetPath(&Gopherp, Pathp);
	       }

 	       if (GSgetTitle(&Gopherp) == NULL) {
		    /*** Check to see if we have a compressed file ***/
		    
		    if (strcmp(filename + strlen(filename) -2, ".Z") ==0)
			 filename[strlen(filename) - 2] = '\0';
		    
		    GSsetTitle(&Gopherp, filename);
	       }
	       else
		    GSsetTitle(&Gopherp, filename);

 	       /** Send out the complete line **/
  
	       GDaddGS(&SortDir, &Gopherp);
	  }
     }
     GDsort(&SortDir);
     GDtoNet(&SortDir, sockfd);
     
     writestring(sockfd, ".\r\n");
}


/*
 * This processes a file containing any subset of
 * Type, Name, Path, Port or Host, and returns pointers to the
 * overriding data that it finds.
 *
 * The caller may choose to initialise the pointers - so we don't
 * touch them unless we find an over-ride.
 */
Process_Side(sidefile, Gopherp)
  FILE *sidefile;
  GopherStruct *Gopherp;
{
     char inputline[MAXLINE];
     static char ItemType[3];
     static char ItemName[255];
     static char Path[255];
     static char Host[64];
     static char Port[10];
     char *cp;

     inputline[0] = '\0';

     for (;;) {
	  for (;;) {
	       cp = fgets(inputline, 1024, sidefile);
	       if (inputline[0] != '#' || cp == NULL)
		    break;
	  }
	  
	  /*** Test for EOF ***/
	  if (cp==NULL)
	       break;
	  
	  ZapCRLF(inputline);  /* should zap tabs as well! */

	  /*** Test for the various field values. **/
	  
	  if (strncmp(inputline, "Type=", 5)==0) {
	       GSsetType(Gopherp, inputline[5]);
	  }
	  else if (strncmp(inputline, "Host=", 5)==0) {
	       GSsetHost(Gopherp, inputline+5);
	  }

	  else if (strncmp(inputline, "Port=", 5)==0) {
	       GSsetPort(Gopherp, atoi(inputline+5));
	  }

	  else if (strncmp(inputline, "Path=", 5)==0) {
	       GSsetPath(Gopherp, inputline+5);
	  }

	  else if (strncmp(inputline, "Numb=", 5)==0) {
	       GSsetNum(Gopherp, atoi(inputline+5));
	  }
     }
     fclose(sidefile);
}


/*
** This function opens the specified file, starts a zcat if needed,
** and barfs the file across the socket.
**
** It now also checks and sees if access is allowed
**
**
*/
void
printfile(sockfd, pathname, startbyte, endbyte)
  int sockfd;
  char *pathname;
  int startbyte, endbyte;
{
     FILE *ZeFile;
     char inputline[512];
     int i, rmtmpfile;
     char Zcatcommand[256];
     char *cp, *pointer1;

     chdir("gopher_root:[000000]");

     rmtmpfile = 0;
     pointer1 = (char * ) strchr( pathname, ':' );
     if ( (pointer1 != 0) && 
          (strncmp(pathname, "gopher_root", 11) != 0) ) {
	  writestring(sockfd, "- Cannot access that file\r\n.\r\n");
          return;
     }

     /*** Check and see if the peer has permissions to read files ***/
     
     if (Can_Read(sockfd) == FALSE) {
	  writestring(sockfd, BUMMERSTR);
	  writestring(sockfd, "\r\nBummer.....\r\n.\r\n");
	  close(sockfd);
	  return;
     }

     if ( (ZeFile = fopen(pathname, "r")) == NULL) {
	  /*
	   * The specified file does not exist
	   */
	  writestring(sockfd, "- File does not exist!!!!!\r\n");
	  writestring(sockfd, ".\r\n");
	  return;
     }

     if (startbyte != 0)
	  fseek(ZeFile, startbyte, 0);
     {
	  FILE *pp;
	  if (pp = specialfile(ZeFile, pathname)) {
	       fclose(ZeFile);
	       ZeFile = pp;
	       rmtmpfile = 1;
	  }
     }

     while (fgets(inputline, MAXLINE, ZeFile) != NULL) {

	  ZapCRLF(inputline);
	  if( inputline[0] != '$' ) {
	    writestring(sockfd, inputline);
	    writestring(sockfd, "\r\n");
	  }

	  if (endbyte >0) {
	       if (ftell(ZeFile) >= endbyte)
		    break;
	  }
     }
     Specialclose(ZeFile);
     if ( rmtmpfile == 1 )
       remove( pathname );
     writestring(sockfd, ".\r\n");
}

/*
 * Warning! this part isn't working yet!
 */
#ifdef BUILTIN_SEARCH

Do_IndexTrans(sockfd, inputline)
  int sockfd;
  char *inputline;
{
     char *IndexDirectory;
     char *SearchString;
     char *cp;


     /** First siphon off the directory pathname **/
     IndexDirectory = inputline;

     cp = (char *) strchr(inputline, '\t');
     
     if (cp == NULL) {
	  /** Give up it won't work..... **/
	  writestring(sockfd, ".\r\n");
	  return;
     } 
     else
	  *cp = '\0';

     
     /** And siphon off the search string **/

     SearchString = cp +1;

     /** Just in case, get rid of anything following a tab **/

     cp = (char *) strchr(SearchString, '\t');
     if (cp != NULL)
	  *cp = '\0';
     
     /** Read in the proper hostdata file.... **/


     /** And call the appropriate query function **/

#ifdef NEXTSEARCH
     NeXTIndexQuery(sockfd, IndexDirectory, SearchString);
#endif

#ifdef WAISSEARCH
/*     WaisIndexQuery(sockfd, IndexDirectory, SearchString);*/
#endif

}

#endif /* BUILTIN_SEARCH */


#define BUFSIZE 1400  /* A pretty good value for ethernet */

void
echosound(sockfd, filename)
  int sockfd;
  char *filename;
{
     FILE *sndfile;
     unsigned char in[BUFSIZE];
     register int j;
     int gotbytes;

/*     if (strcmp(filename, "-") == 0) { */
	  /*** Do some live digitization!! **/
/*	  sndfile = popen("record -", "r");
     }
     else
	  sndfile = fopen(filename, "r");
*/
     while(1) {
	  gotbytes = fread(in, 1, BUFSIZE, sndfile);
	  
	  if (gotbytes == 0)
	       break;       /*** end of file or error... ***/

          j = writen(sockfd, in, gotbytes);

	  if (j == 0)
	       break;       /*** yep another error condition ***/
     }
}
