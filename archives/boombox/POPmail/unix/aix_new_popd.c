
/*
	POP2 Server

  (C) Copyright 1989 Regents of the University of Minnesota

  Permission to use, copy, modify, and distribute this program
  for any purpose and without fee is hereby granted, provided
  that this copyright and permission notice appear on all copies
  and supporting documentation, the name of University of Minnesota
  not be used in advertising or publicity pertaining to distribution
  of the program without specific prior permission, and notice be
  given in supporting documentation that copying and distribution is
  by permission of the University of Minnesota.
  The University of Minnesota makes no representations about
  the suitability of this software for any purpose.  It is provided
  "as is" without express or implied warranty.


	by George Gonzalez
	Microcomputer and Workstation Networks Center
	Version 1.5b   Sep 29, 1989

Changes since the 1.0 release:
	Fixed long word alignment problems in mv().
	Fixed byte order problems in DES code.
	Fixed network byte order problems in choosing port #.
	Fixed lingering socket problems.
	Fixed empty spool file problems.
	Fixed BSD compatibility problems.
	Fixed %m format portability problems.
	Added command line options.
	Moved log file to /usr/adm.
	Just one log file.

Notes:
	It works fine for us, but there may be a few lingering bugs. Beware.
	This software has been tried on A/UX, SunOS, Pyramid,
	Encore, and NeXT's MACH.
        This code is only conditionalized for the above variants.
        You are encouraged to try it on other Unix systems.

	*** The password lookup code may not work if you are using
	some non-standard password validation system, such as Kerberos.

	For BSD like systems, it should work much as-is.
        For System V Unix systems,  try -DAUX

	Note that I'm a Pascal programmer, so the code looks a bit
	strange to C purists.

To build:

	cc -o popd { options } popd.c

Compile options:
	-DAUX	        ( for A/UX or maybe system V )
	-DNOFLOCK	( if your flavor of Unix doesnt have flock(2) )
	-DAIX		( if AIX then turnoff sprintf prototyping )

To debug it:
Run the daemon (as root) with debugging turned on:
	% su
	Password:
	# ./popd -d 

This changes its behaviour so it writes debug information to the terminal.
Also it does not fork so you retain control from your terminal.
It should print some messages confirming that things are OK.

Try talking to it from another port with telnet:
	% telnet yourhost 109
It should say:
	+ POP2 yourhost ...... (C) 1989 U of Minn
Try identifying yourself:
	helo yourname yourpw
It should respond with:
	#xxxx
where xxxx is the number of mail messages you have waiting.
these messages have been read and copied to ~/Mail/inbox/yyyyy.
Then you can try reading the messages with:
	read 1
	retr
	acks

It should return the body of the message to you after the retr.

Now try connecting to it from the HyperCard stack.

When you think it works, you should put it in your system
startup script, probably in rc.local:

	if [ -f /etc/popd ]; then
		/etc/popd
	fi

Options:
	-d	Turns on debug mode.  Prints out more information,
		and does not fork away from you.

	-l xxx	Write log to file xxx.  Default is /usr/adm/popd.log.

	-m xxx	Look for mail spool files in directory xxx.  Default
		is /usr/mail for System V, /usr/spool/mail for others.
		
	-p yyy	Listen on port yyy.  Default is the standard POP port, 109.
		Useful if you already have port 109 busy running a POP1
		server.
		
	-s	Turn off mailing statistics to "POPADMIN". Normally mails
		out daily statistics.


To fine tune it:
	There are a bunch of tuneable constants that you can adjust
	as you see fit.


Problems with compiling?:
	There are several possible portability problems.  If you get
	compile or execution errors, look at:

	-DAUX switch to select AUX or system V compatibility.
	-DNOFLOCK if you get compile errors on the 'flock()' call.
	
	Check the definitions for longint and integer.


Problems?:
	The daemon writes to a logfile:
	/usr/adm/popd.log

	popd.log gets informational messages such as who read their mail
	This file goes to your terminal with the -d option.

Still have Problems?:
	Contact grg@boombox.micro.umn.edu if you have any stubborn problems
	or suggestions.  Note that I'm not a unix wizard, so you may know
	more than me.

Known problems:
	(1) If you read the messages out of order, POP gets confused.
	    The stack always reads them in order, so it gets by.

	(2) I'm not a wizard on security problems caused by programs
		running as root, so there are possible security
		problems.  Here's a few of them:
		(a) There is no limit on the number of POP connections.
		    A lot of connections could kill your system.
		(b) There's no limit to the number of commands sent to
		    POP.  An abuser could chew up quite a bit of CPU
		    time.
		    
           I'd be glad to receive any hints or improvements.

	(3) The log file can grow without bound.  You should trim it
	    back periodically.


Improvements since the 0.93a release:
	Optional password encryption added.  The HyperCard stack uses it.
	Added INQM command for quick mail checking.
	Slightly better signal handling.
	Fixed some 16/32 bit portability glitches.
	Read mail directly instad of calling mailer.
	Logs bad password attempts.

Future improvements:
	Add super secure random number exchange password mode.
	Check for CPU time on non-BSD systems.
	Wait for completions instead of later cleanups.
	Clean up error messages.
	Log other signals.
	Handle abusive users.
*/

#define __STRICT_BSD__		/* for MACH */
#define	_BSD 43			/* for AIX!! */
#define _BSD_INCLUDES


#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/dir.h>
#include	<sys/file.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<stdio.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<sys/socket.h>
#include	<pwd.h>
#include	<signal.h>
#include	<sys/param.h>
#include	<sys/ioctl.h>
#include	<sys/wait.h>
#include	<sys/time.h>
#include	<sys/resource.h>
#include	<sys/uio.h>

#ifndef	AUX
#define	AUX	0
#endif

#ifndef	AIX
#define	AIX	0
#endif


#if	AUX
#include	<time.h>
#endif

#define	STRLEN	256

#define	Version		"1.5aix"

/* define longint to be a 32-bit integer,  integer to be 16 or 32 bit integer*/

typedef	int	longint;
typedef	short	integer;
typedef	char *	String;
typedef char AllocString[STRLEN];
typedef	FILE *  File;

#define false           0
#define true            1

#define EOS             '\0'
#define EOL             '\n'
#define	NIL		0

#define not             !
#define and             &&
#define or              ||
#define is              ==
#define isnt            !=

#define procedure   void
#define function 
#define begin {
#define end   }
#define var   {
#define code  ;
#define endproc }
#define endfunc }

#define call  (void)

#define IF(X) if( X ) {
#define ELSE   } else {
#define FI         }

#define WHILE(X) while (X) {
#define ENDW }

#define repeat  do {
#define until(X) } while ( not ( X ) );

#define FOR(X,Y,Z)   for ( X = Y; X <= Z; X++) {
#define ROF          }

#define inc(X)       X++
#define dec(X)       --X

#define POPPORT         109
#define OTHERPOPPORT	50109
#define	FCMODE		0666


/*	Tuneable constants	*/

#define DECENTINTERVAL  60	/* Time to sleep between bind attempts 	*/
#define CONNECTTIMEOUT	120 	/* Time to wait between connects 	*/
#define CMDTIMEOUT	300	/* Time to wait between commands 	*/
#define DAEMONTIMELIMIT	1000L	/* Max CPU seconds for daemon 		*/
#define DAEMONFILELIMIT	10000000L	/* Max file size for daemon 	*/

#define	CLIENTTIMELIMIT	100L	/* Max CPU seconds per client		*/
#define	CLIENTFILELIMIT	1000000L/* Max file size for client files	*/
#define	MaxMsgs		1000	/* Max # of msgs outstanding		*/


/*-------------------------------------------------*/

#define Okay            0
#define Error           -1
#define FOLDER_SUFFIX  "/Mail/"

#define	LOGFILEDIR	"/usr/adm"
#define	LOGFILENAME	"popd.log"
#define DEF_FOLDER     "inbox"

#if 	AUX
#define	PREFMAILER	"/usr/bin/mailx"
#else
#define	PREFMAILER	"/usr/ucb/mail"
#endif
#define	MAILER		"/bin/mail"
#define POPADMIN	"popstats@boombox.micro.umn.edu"
#define	TEMPFILE	"/tmp/pop.temp"

#define	SLevelID	"SECURITYLEVEL"
#define	SKeyID		"KEY"
#define	SLEVELNONE	'1'
#define	SLEVELXOR	'2'
#define	SLEVELDES	'3'
#define	SLEVELRNX	'4'

#define	MaxSLevel	SLEVELDES

#if	AUX
#define	SYS_MAILDIR	"/usr/mail"
#define	COERCE		(int)
#define	WRITETYPE	(unsigned)
#define	SYS_MAILDIR_SUFFIX	"auxxx"
#else
#if AIX
#define SYS_MAILDIR    "/usr/spool/mail"
/*#define SYS_MAILDIR_SUFFIX ".newmail"*/
#define SYS_MAILDIR_SUFFIX ""
#define COERCE          (struct rusage *)
#define WRITETYPE
#else
#if !AUX & !AIX
#define SYS_MAILDIR    "/usr/spool/mail"
#define	COERCE		(struct rusage *)
#define	WRITETYPE	
#define SYS_MAILDIR_SUFFIX      "other"
#endif
#endif
#endif


#define	RootUID	0
#define	EEERRBASE	200
#define	EETIMEOUT	199
#define	EEERROR		198

#define TempDir        "Mail"

#define IBUFLEN         1024
#define OBUFSIZE        4096
#define LOGOPTIONS       O_WRONLY | O_CREAT | O_APPEND
#define	LOGFILE		stdout
#define	ERRFILE		stdout
#define	SEPARATOR	"-----------------------------------------------\n"
#define	SEPR2		"***************************************"

typedef   void ( * func ) ();

#define   AUTH  0
#define   MBOX  1
#define   ITEM  2
#define   NEXT  3
#define   Done  4


#define  PNULL 0
#define  HELO  1
#define  HELP  2
#define  FOLD  3
#define  READ  4
#define  RETR  5
#define  ACKS  6
#define  ACKD  7
#define  NACK  8
#define  INQU  9
#define  QUIT  10

#define NKEYS QUIT

#define	LOG 	call printtime( LOGFILE ); call fprintf( LOGFILE, 
#define	PE	call printtime( ERRFILE );
#define LOGEMS	call printtime( ERRFILE ); call fprintf( ERRFILE,

/* global variables */

integer	token;
integer	state = AUTH;
integer	debug = true; 
char    ident[] = "POP2 Server version ";
char    vers[] = Version;
char    copyrt[] = "(C) 1989 Regents of the University of Minnesota";

#if	AUX
char	outfilebuf[BUFSIZ];
char	errfilebuf[BUFSIZ];
#endif
integer	CurrentMessage;

union {
       longint L;
       char C[4];
       } Dual;

integer  IntelOrder;
integer  SLevel;

integer	NumOfMessages;
longint	LenOfThisMessage;
integer	msglist[MaxMsgs];
integer	rootpid, ourpid, parentspid;
integer	saveerr;
integer	alarmpending = false;

integer	OurArgCount;
char	PopArg[4][100];

static struct	passwd * pwd;
struct	sockaddr_in PopsName;
extern	int errno;

integer	serversocket;
integer	SockOfClient;

longint	SK[2];
char	SKHex[20];
char	hostname[100];

longint	Connections;
longint Messages;
longint Timeouts;
longint Errors;
AllocString	tempmailFile, of;
int	holdf, fk;
int ouruid;

struct itimerval	it, now, dummy;
time_t	timein;

char	herald[200];
struct	servent * sp;
struct	sockaddr_in ClientAddress;
integer	err;


        String ofile;
	String spfile;
	int portnum, stats;


String strncat();
String strncpy();


#if AIX
#define	NOFLOCK 1
#else
#if  AUX
	void perror();
	unsigned alarm();
	unsigned sleep();
	String asctime();
	struct tm * localtime();
	void exit();
	void _exit();
	long time();
#else
	String sprintf();
	time_t time();
#endif
#endif



procedure printtime( f ) File f;
var
   String c;
    time_t t;
    String name;
    integer pid;
    struct tm * TheTime;

code
    pid = getpid();
    call time( (time_t *) &t );
    TheTime = localtime( ( long * ) &t );
    c = asctime( TheTime );
    c[ strlen(c) - 6 ] = '\0';
    IF( pwd is NIL )
       name = "null";
    ELSE
       name = pwd->pw_name;
    FI
    IF( pid is rootpid )
       call fprintf( f, "pop daemon.: %s:%s: ", &c[11], name );
    ELSE
       IF( parentspid is rootpid )
          call fprintf( f, "popd /%5d: %s:%s: ", pid, &c[11], name );
       ELSE
          call fprintf( f, "%5d/%5d: %s:%s: ", parentspid, pid, &c[11], name );
       FI
    FI
endproc


procedure error( msg ) String msg;
var
   code
   LOGEMS "*** Error: %s Err #%d\n", msg, saveerr );
   PE;
   errno = saveerr;
   perror( "" );
endproc


procedure Assert( errcode, msg ) integer errcode; String msg;
var 
    AllocString errbuf;
 code
    IF( errcode is Error )
       saveerr = errno;
       call strncpy( errbuf, "Can't ", sizeof errbuf );
       call strncat( errbuf, msg, sizeof errbuf );
       error( errbuf );
    ELSE
       IF( debug )
           LOG "%s Okay\n", msg );
       FI
    FI
endproc



procedure BombTo( F, msg ) File F; String msg;
var code
           call printtime( F );
           call fprintf( F, "%s\n", msg );
endproc


function int RealShutDownHandler()
var code
   LOG "Timed out while closing a file\n" );
   _exit( EEERROR );  /* exit if hung in socket closing.... */
endfunc



procedure ShutDownHandler( msg ) String msg;
var
        integer i;
code
           BombTo( LOGFILE, msg );
	  /*  BombTo( ERRFILE, msg ); */
	   Assert( (int) signal(SIGALRM, RealShutDownHandler), "set hang sig" );
           i = NOFILE;
	   repeat
	      dec(i);
	      call alarm( 10 );
	      call close( i );
	   until( i < 0 );
        _exit( Messages ) /* return # of msgs processed to parent */;
endfunc

function int TermHandler()
var
code
    ShutDownHandler( "POP was asked to terminate." );
endfunc

function FSizeHandler()
var code
   ShutDownHandler( "POP ran out of file space." );
endfunc


function CPUTimeHandler()
var code
   ShutDownHandler( "POP ran out of CPU time." );
endfunc





function CleanUpOurMess()
var
        union wait status;
	integer pid, limit;
code
       limit = 0;
       repeat
	  pid = wait3( &status, WNOHANG, COERCE NIL);
	  IF( pid > 0 )
	     LOG "Cleaning up process id %d, msgs=%d\n",
	         pid, (int) status.w_retcode );
	     IF( status.w_termsig is 0 )
		IF( status.w_retcode < EEERROR ) Messages += status.w_retcode;
		ELSE IF( status.w_retcode is EETIMEOUT ) Timeouts++;
		        ELSE IF( status.w_retcode is EEERROR ) Errors++; FI
		     FI
		FI
	     FI
	  FI
          inc(limit);
       until( limit > 20 or pid <= 0 );
endfunc


procedure RedirectSignals( ) 
var
         integer i;
code
        FOR( i, 1, NSIG-1 )
            IF( i isnt SIGKILL and i isnt SIGSTOP and i isnt SIGCONT and
                i isnt SIGTERM and i isnt SIGXCPU and i isnt SIGXFSZ and
		i isnt SIGCHLD and i isnt SIGQUIT and i isnt SIGSEGV )
               call (int) signal(i, SIG_IGN);
            FI
        ROF
	Assert( (int) signal(SIGTERM, TermHandler), "set terminate trap" );
        Assert( (int) signal(SIGXFSZ, FSizeHandler), "set FSIZE trap" );
        Assert( (int) signal(SIGXCPU, CPUTimeHandler), "set CPU signal" );
endproc



procedure check( errn ) integer errn;
var
code
   IF( errn is Error ) exit( EEERRBASE + errno ); FI
endproc



procedure OpenLogFile( )
var
   AllocString f;
code
   IF( ouruid is RootUID )
      call strncpy( f, ofile, sizeof f );
   ELSE
      call strncpy( f, LOGFILENAME, sizeof f );
   FI
   check( open( f, LOGOPTIONS, FCMODE ) );
endproc


procedure ReconnectIO()
var
        integer i;
code
        IF( fork() ) exit(0); FI
        rootpid = getpid();
        Assert( setpgrp(0, 0), "set process group.");

        FOR(i,0,NOFILE) call close(i); ROF
        /* Reset stdin, LOGFILE, ERRFILE. */
        check( open("/dev/null", O_RDONLY) );
        OpenLogFile( );
	OpenLogFile( );
#if	AUX
	IF( setvbuf( LOGFILE, outfilebuf, _IOLBF, BUFSIZ ) isnt Okay )
	    Assert( Error, "setvbuf output" );
	FI 
	IF( setvbuf( ERRFILE, errfilebuf, _IOLBF, BUFSIZ ) isnt Okay )
	    Assert( Error, "setvbuf stderr" );
	FI
#else
	setlinebuf( LOGFILE );
	setlinebuf( ERRFILE );
#endif
/* Clear controlling tty. */
	IF( (i = open("/dev/tty", O_RDWR)) > 0)
                call ioctl(i, TIOCNOTTY, 0);
                call close(i);
        FI
endproc


#if	AUX
#else
procedure SetLimits( T, F ) long T, F;
var
   struct rlimit rlp;
code
     rlp.rlim_cur = T;
     rlp.rlim_max = T + 10;
     Assert( setrlimit( RLIMIT_CPU, ( struct rlimit * ) &rlp ),
             "set CPU limit" );
     rlp.rlim_cur = F;
     rlp.rlim_max = F + 10000;
     Assert( setrlimit( RLIMIT_FSIZE, ( struct rlimit * ) &rlp ),
             "set file size limit" );
endproc

#endif

String function onoff( boo ) int boo;
var
code
   IF( boo is true )
      return( "ON" );
   ELSE
      return( "OFF" );
   FI
endfunc



procedure StartMsg( )
var
    int port;
code
    LOG "%s\n", SEPR2 );
    port = ntohs( PopsName.sin_port );
    LOG "%s %s\n", ident, vers );
    LOG "%s\n", copyrt );
    LOG "On TCP/IP port #%d\n", port );
    LOG "Logging to file %s.\n", ofile );
    LOG "Looking in mail spool directory %s.\n", spfile );
    LOG "Debug option is %s.\n", onoff( debug ) );
    LOG "Send out statistics option is %s.\n", onoff( stats ) );
endproc


    
procedure NewHerald()
var
code
    IF( debug )
	SK[0] = 0x1C587F1C;
        SK[1] = 0x13924FEF;
    ELSE
        call time( (time_t *) &SK[0] );
	SK[0] ^= SK[0] << 27;
        SK[1] = SK[0] ^ ourpid ^ ( ourpid << 4 ) ^ ( ourpid << 8 )
	^ ( ourpid << 13 ) ^ ( ourpid << 21 ) ^ (ourpid << 27 );
    FI
    call sprintf( SKHex, "%.8X%.8X", SK[0], SK[1] );
    call sprintf( herald,
     "+ POP2 %s server %s %c %s %s %s (C) 1989 University of Minnesota", 
           hostname, SLevelID, SLevel, SKeyID, SKHex,
	   Version );
endproc



procedure init( ourname ) String ourname;
var
    unsigned long lo, hlo;
    int berr;
    int err;
    int enable;
code    
    rootpid = getpid();
    Dual.L = 0x01020304;
    IntelOrder = Dual.C[0] is 04;
    IF( IntelOrder )
       SLevel = SLEVELXOR;
    ELSE
       SLevel = MaxSLevel;
    FI
    SLevel = MaxSLevel;
    ourpid = rootpid;
    ouruid = getuid();
    IF( ouruid isnt RootUID )
       call fprintf( ERRFILE, "*** Error: not running as root, uid = %d ***\n",
		     ouruid );
    FI
    parentspid = 1;
    Connections = Messages = Timeouts = Errors = 0;
    bzero( (String) &ClientAddress, sizeof ClientAddress );
    pwd = NIL;
    IF( gethostname( hostname, sizeof hostname ) isnt Okay )
       call strncpy( hostname, "<<unknown host>>", sizeof hostname );
    FI
    NewHerald();
    call fprintf ( LOGFILE, "%s PID: %d\n", herald, rootpid);
    IF( not debug )
       RedirectSignals( );
       ReconnectIO();
    FI
    sp = getservbyname("pop", "tcp");
#if	AUX
#else
    SetLimits( DAEMONTIMELIMIT, DAEMONFILELIMIT );
#endif
    IF( ouruid isnt RootUID )
	hlo = OTHERPOPPORT;
    ELSE
     IF( portnum is 0 )
       IF( sp is NIL )
          hlo = POPPORT;
       ELSE
          hlo = ntohs( sp->s_port );
       FI
     ELSE
       hlo = portnum;
     FI
    FI
    lo = htons( hlo );
    PopsName.sin_port = lo;
    StartMsg( );
    WHILE((serversocket = socket(AF_INET, SOCK_STREAM, 0)) < Okay )
        LOGEMS "%s: socket: %d\n", ourname, errno);
        call sleep( (unsigned) DECENTINTERVAL);
    ENDW
/*
    val.l_onoff = 1;
    val.l_linger = 10;
    err = setsockopt( serversocket, SOL_SOCKET, SO_LINGER,
		      (char *) &val, sizeof( val ) );
    Assert( err, "set short linger option" );
*/
    enable = 1;
    err = setsockopt( serversocket, SOL_SOCKET, SO_REUSEADDR,
			(char *) &enable, sizeof enable );
    Assert( err, "set reuse address option" );
    WHILE( ( berr = bind( serversocket, ( struct sockaddr * ) &PopsName,
		sizeof(PopsName) )) < Okay )
        PE; perror( "bind:" );
        call sleep( (unsigned) DECENTINTERVAL );
    ENDW
    Assert( berr, "bind socket to port number" );
endproc



procedure SendToNet(s) String s;
var
    AllocString outline;
code
    call strncpy( outline, s, sizeof outline - 4 );
    call strncat( outline, "\r\n", sizeof outline );
    IF( write( SockOfClient, outline, WRITETYPE strlen( outline ) ) < Okay )
        LOGEMS "SendToNet: (%d) %d\n", SockOfClient, errno );
        exit( EEERROR );
    FI
endproc

procedure ClearTimeOut()
var
code
    IF( alarmpending )
      call alarm( 0 );
      alarmpending = false;
    FI 
endproc



procedure logout()
var
    time_t timeout, elapsed;
    long uvtime, vtime, cputime; 
    float pcnt;
    String name;
code
   ClearTimeOut();
   call time( (time_t *) &timeout );
   elapsed = timeout - timein;
   IF( elapsed < 1 ) elapsed = 1; FI
   call getitimer( ITIMER_VIRTUAL, &now );
   vtime = it.it_value.tv_sec - now.it_value.tv_sec;
   uvtime = it.it_value.tv_usec - now.it_value.tv_usec;
   cputime = vtime * 1000 + ( uvtime / 1000 );
   pcnt = (float) cputime / (float) ( elapsed * 10 );
   IF( pwd is NIL ) name = "<< unknown >>";
   ELSE name = pwd->pw_name;
   FI
   LOG "Closing session for  '%s'\n", name );
   LOG "Time used: CPU: %ld msec,  real: %u secs.%7.3f%%\n", cputime, elapsed, pcnt );
   LOG SEPARATOR );
endproc



function TimeOutHandler()
var
code
    alarmpending = false;
    call ShutDownHandler( "Connection idle too long.. Dropping it!" );
    exit( EETIMEOUT );
endfunc



function TheParentTimeOutHandler()
var
code
    alarmpending = false;
endfunc


procedure bomb( msg ) String msg;
begin code
   SendToNet( msg );
   LOGEMS "Error: %s\n", msg );
   logout( );
   err = Error;
endproc


function longint SizeOfFile(FileN) String FileN;
var
    integer c;
    longint len;
    File jfn;
    AllocString tbuf;
code

    IF( (jfn = fopen (FileN, "r") ) is NIL )
        call sprintf( tbuf, "- Can't open file '%s' to count chars.", FileN);
        bomb (tbuf);
        return ( Error );
    FI
    len = 0;
    repeat
        c = fgetc ( jfn );
        switch (c) {
            case EOL:    len += 2; break;
            case '\r':   break;
            case  EOF:    break;
            default:    inc(len);
        }
    until(c is EOF)
    Assert( fclose(jfn), "close file" );
    return (len);
endproc



procedure ReturnFileSize()
var
        char fname[20], tbuf[20];
   integer msgnum;
code
  msgnum = msglist[ CurrentMessage ];
  IF( msgnum is 0 )
    LenOfThisMessage = 0;
  ELSE
     call sprintf( fname, "%d", msgnum );
     LenOfThisMessage = SizeOfFile( fname );
  FI
  call sprintf(tbuf, "=%d", LenOfThisMessage);
  SendToNet(tbuf);
endproc




procedure ensure_dir( dir ) String dir;
begin code
    IF( chdir(dir) isnt Okay )
        IF( errno is ENOENT ) 
            Assert( mkdir( dir, 0700 ), "make directory." );
            Assert( chdir( dir ), "chdir to users mail directory." );
        ELSE
           bomb( "- Can't chdir to your directory");
        FI
     FI
endproc



procedure mclose( fil, lines )  File * fil; integer * lines;
var
code
    IF( * fil isnt NIL )
       IF( debug ) LOG "Closing new mail file, lines = %d\n", * lines ); FI
       Assert( fclose( * fil ), "close mail file" );
       * fil = NIL;
       * lines = 0;
    FI
endproc



function integer MessageCounter(foname) String foname;
var
    DIR *dirp;
    struct direct *dirent;
    integer msg, msgid;
    struct stat statbuf;

code
  msg = 0;
  IF( chdir(foname) is Okay )
    dirp = opendir(foname);
IF( dirp isnt NIL )
    dirent = readdir(dirp);
    WHILE( dirent isnt NIL )
IF( isnumber(dirent->d_name) )
        IF( access(dirent->d_name, R_OK) is Okay )
                call stat(dirent->d_name, &statbuf);
            IF( statbuf.st_size is (off_t) 0 )
                   Assert( unlink(dirent->d_name), "unlink file" );
                ELSE
                    msgid = atoi( dirent->d_name );
                    inc(msg);
                    msglist[ msg ] = msgid;
                FI
            FI
        FI
        dirent = readdir(dirp);
    ENDW
    closedir(dirp);
ELSE
   LOG "ReadDir failed\n" );
FI
  ELSE
   LOG "Cant chdir to dir '%s'\n", foname );
  FI
  LOG "Counted %d messages in '%s'.\n", msg, foname );
  return (msg);
endfunc



function LockAcquire( fn ) String fn;
#define	maxlockloops	10
#define	locksleeptime	3	

var
   int f, loops;
code
   loops = 0;
   IF( debug ) LOG "Acquiring lock %s\n", fn ); FI
   WHILE( (f = open( fn, O_CREAT | O_EXCL, 000 )) is Error
                 and loops < maxlockloops )
      Assert( f, "create lock file" );
      IF( debug ) LOG "Lock busy... waiting...\n" ); FI
      call sleep( locksleeptime );
      inc( loops );
   ENDW
   IF( loops >= maxlockloops )
      IF( debug ) LOG "Couldnt get the lock... proceeding anyway!!!!\n" ); FI
      Assert( unlink( fn ), "delete lingering lock file..." );
      return( Okay );
   FI
   IF( debug ) LOG "Got the lock...\n" ); FI
   call close( f );
  return( Okay );
endfunc

procedure LockRelease( fn ) String fn;
var
code
   Assert( unlink( fn ), "remove lock file" );
   IF( debug ) LOG "Removed the lock file %s...\n", fn ); FI
endproc



function FLockAcquire( fn ) String fn;
#define	maxlockloops	10
#define	locksleeptime	3	

var
   int loops, Err;
code
   loops = 0;
   IF( debug ) LOG "Acquiring flock type of lock on '%s'\n", fn ); FI
   WHILE( loops < maxlockloops )
      fk = open( fn, O_RDWR );
      IF( fk is Error )
	 Err = fk;
      ELSE
#ifdef NOFLOCK
	Err = Okay;
#else
	Err = flock( fk, LOCK_EX | LOCK_NB );
#endif
      FI 
      IF( Err is Okay )
	 break;
      ELSE
         IF( fk isnt Error ) call close( fk ); FI
	 IF( debug ) LOG "LockF busy... waiting...\n" ); FI
         call sleep( locksleeptime );
         inc( loops );
      FI
   ENDW
   IF( loops >= maxlockloops )
      IF( debug ) LOG "Couldnt get the flock...proceeding anyway!!!!\n" ); FI
      return( Okay );
   FI
   IF( debug ) LOG "Got the flock...\n" ); FI
   return( Okay );
endfunc



procedure FLockRelease()
var
code
   call close( fk );
   IF( debug ) LOG "Released the flock\n" ); FI
endproc


procedure GetCurrentMail(uname) String uname;
var
  File fp, inf;
  integer fnum, lc, cc;
  AllocString outmailFile, line;
  String p;
code
  fnum = MessageCounter( "." );
  IF( holdf is NIL )
      inf = NIL;
      IF( debug ) LOG "holdf is NIL\n" ); FI
  ELSE
      inf = fdopen( holdf, "r+");
  FI
  IF( inf is NIL )
     err = Okay;
     IF( debug ) LOG "inf is NIL\n" ); FI
     perror("help with inf");
  ELSE
  fp = NIL; lc = 0;
  WHILE( true )
    p = fgets( line, sizeof line, inf );
    IF( p is NIL ) break; FI
    IF( p[0] is 'F' and p[1] is 'r' and p[2] is 'o' 
    and p[3] is 'm' and p[4] is ' ' )
	mclose( &fp, &lc );
	inc( fnum );
        call sprintf(outmailFile, "%d", fnum);
        fp = fopen(outmailFile, "w+" );
        lc = 0;
        IF( debug )
	   LOG "Opening new mail file '%s' for %s\n", outmailFile, uname );
	FI
	IF( fp is NIL )
            bomb("- Can't gather new mail.  Host mail dir. unwritable.");
        FI
    FI
    cc = fputs( line, fp ); inc(lc);
    IF( cc is EOF )
      LOGEMS "Cant write to file!!\n" );
      bomb( "- Can't gather mail, file write error." );
    FI
ENDW
  mclose( &fp, &lc );
  call fclose( inf );
  FI
endproc



procedure setenv(ename, eval, buf) String ename; String eval; String buf;
var
     String cp; String dp;
     extern char    **environ;
     String * ep;
code
    ep = environ;
    WHILE( dp = *ep++ )
        cp = ename;
        WHILE(*cp is *dp and *cp )
         inc(cp); inc(dp);
        ENDW
        IF( ( *cp is EOS ) and ( (*dp is '=') or (*dp is EOS) ) )
            call strncat(buf, eval, 128);
            *--ep = buf;
            return;
        FI
    ENDW
endproc




/* Sofware DES functions
 * written 12 Dec 1986 by Phil Karn, KA9Q; large sections adapted from
 * the 1977 public-domain program by Jim Gillogly

 Modified by George Gonzalez to not do initial/final permutations

 */

#define __STRICT_BSD__		/* for MACH */

/* Tables defined in the Data Encryption Standard documents */

/* permuted choice table (key) */
static char pc1[] = {
	57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18,
	10,  2, 59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36,

	63, 55, 47, 39, 31, 23, 15,  7, 62, 54, 46, 38, 30, 22,
	14,  6, 61, 53, 45, 37, 29, 21, 13,  5, 28, 20, 12,  4
};

/* number left rotations of pc1 */
static char totrot[] = { 1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28 };

/* permuted choice key (table) */
static char pc2[] = {
	14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10,
	23, 19, 12,  4, 26,  8, 16,  7, 27, 20, 13,  2,
	41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
	44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
};

/* The (in)famous S-boxes */
static char si[8][64] = {
	/* S1 */
	14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
	 0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
	 4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
	15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13,

	/* S2 */
	15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
	 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
	 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
	13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9,

	/* S3 */
	10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
	13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
	13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
	 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12,

	/* S4 */
	 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
	13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
	10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
	 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14,

	/* S5 */
	 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
	14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
	 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
	11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3,

	/* S6 */
	12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
	10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
	 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
	 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13,

	/* S7 */
	 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
	13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
	 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
	 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12,

	/* S8 */
	13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
	 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
	 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
	 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
};

/* 32-bit permutation function P used on the output of the S-boxes */
static char p32i[] = {	
	16,  7, 20, 21, 29, 12, 28, 17,  1, 15, 23, 26,  5, 18, 31, 10,
	 2,  8, 24, 14, 32, 27,  3,  9, 19, 13, 30,  6, 22, 11,  4, 25
};

/* Lookup tables initialized once only at startup by desinit() */
static longint (*spbox)[64];		/* Combined S and P boxes */


static unsigned char (*kn)[8];

/* bit 0 is left-most in byte */
static integer bytebit[] = { 0200,0100,0040,0020,0010,0004,0002,0001 };

/* Initialize the lookup table for the combined S and P boxes */
procedure spinit()
var
	char pbox[ 32 ];
	integer p, i, s, j, t, rowcol;
	longint val;
code
	FOR( p, 1, 32 )
		FOR( i, 0, 31 )
			IF( p32i[ i ] is p )
				pbox[ p - 1 ] = 31 - i;
				break;
			FI
		ROF
	ROF
	FOR( s, 0, 7 )
		FOR( i, 0, 63 )
		   val = 0;
	 	   rowcol = (i & 32) | ((i & 1) ? 16 : 0) | ((i >> 1) & 0xf);
		   t = si[ s ] [ rowcol ];
		   FOR( j, 0, 3 )
			IF( t & ( 8 >> j ) )
				 val |= 1L << pbox[ ( s << 2 ) + j ];
			FI
		   ROF
		   spbox[ s ] [ i ] = val;
		ROF
	ROF
endproc

/* Allocate space and initialize DES lookup arrays
 * mode is 1: DEA without initial and final permutations for speed
 */

function desinit()
var
	String malloc();
code
	IF( spbox isnt NIL ) return 0; FI

	spbox = ( longint (*)[64] ) malloc( sizeof( longint ) * 8 * 64);
	IF( spbox isnt NIL)	
	   spinit();
	   kn = (unsigned char (*)[8]) malloc( sizeof( char ) * 8 * 16);
	   IF( kn is NIL )
		free( (String) spbox );
		spbox = NIL;
	   FI	
	FI
	IF( spbox is NIL or kn is NIL )
	   return -1;
	ELSE
	   return 0;
	FI
endfunc


/* Free up storage used by DES */
procedure desdone()
var code
    IF(spbox isnt NIL)
	call free( (String) spbox );
	call free( (String) kn );
	spbox = NIL;
	kn = NIL;
    FI
endproc



procedure setdeskey( key ) String key;
var
	char pc1m[56];		/* place to modify pc1 into */
	char pcr[56];		/* place to rotate pc1 into */
	register integer i, j, k;
code
	/* Clear key schedule */
	FOR(i, 0, 15) FOR(j, 0, 7) kn[i][j] = 0; ROF ROF

	FOR(j, 0, 55 )
		k = pc1[ j ] - 1;
		pc1m[ j ] = ( key[ k >> 3 ]  & bytebit[ k & 07 ]) ? 1 : 0;
        ROF	
	FOR( i, 0, 15 )
 	  FOR( j, 0, 55 )
	      k = j + totrot[ i ];
	      pcr[ j ] = pc1m[ k < ( j < 28 ? 28 : 56 ) ? k : k - 28 ];
	  ROF
	  FOR( j, 0, 47 )
	     IF( pcr[ pc2[ j ] - 1 ] )
		kn[ i ] [ j / 6 ] |= bytebit[ j % 6 ] >> 2;
	     FI
	  ROF
        ROF
endproc


#define	DD	unsigned long

procedure byteswap( x ) DD * x;
var
	register String cp;
	register char tmp;
code
	cp = (String) x;
	tmp = cp[3];
	cp[3] = cp[0];
	cp[0] = tmp;

	tmp = cp[2];
	cp[2] = cp[1];
	cp[1] = tmp;
endfunc


procedure mv( in, out ) char * in, *out;
var  integer i;
code
	FOR( i, 0, 7 );
	  out[i] = in[i];
	ROF
	IF( IntelOrder )
          byteswap( (DD * ) &out[0] );
          byteswap( (DD * ) &out[4] );
	FI
endproc




/* The nonlinear function f(r,k), the heart of DES */
function longint f( r, subkey ) DD r; unsigned char subkey[8];
{
	register DD rval, rt;
code
	/* Run E(R) ^ K through the combined S & P boxes
	 * This code takes advantage of a convenient regularity in
	 * E, namely that each group of 6 bits in E(R) feeding
	 * a single S-box is a contiguous segment of R.
	 */
	rt = (r >> 1) | ((r & 1) ? 0x80000000 : 0);
	rval = 0;
	rval |= spbox[0][((rt >> 26) ^ *subkey++) & 0x3f];
	rval |= spbox[1][((rt >> 22) ^ *subkey++) & 0x3f];
	rval |= spbox[2][((rt >> 18) ^ *subkey++) & 0x3f];
	rval |= spbox[3][((rt >> 14) ^ *subkey++) & 0x3f];
	rval |= spbox[4][((rt >> 10) ^ *subkey++) & 0x3f];
	rval |= spbox[5][((rt >> 6) ^ *subkey++) & 0x3f];
	rval |= spbox[6][((rt >> 2) ^ *subkey++) & 0x3f];
	rt = (r << 1) | ((r & 0x80000000) ? 1 : 0);
	rval |= spbox[7][(rt ^ *subkey) & 0x3f];
	return rval;
endfunc



procedure pass( num, block ) integer num; DD *block;
var
     integer b;
code
	b = num & 1;
	block[ b ] ^= f( block[ 1 - b ], kn[ num ] );
endproc




/* In-place encryption of 64-bit block */
procedure endes(block) DD * block;
var
	register integer i;
	DD work[3]; 		/* Working data storage */
	longint tmp;
code
	mv( ( char * ) block, (char * ) work );	/* Initial Permutation */
	FOR( i, 0, 15 ) pass( i, work ); ROF
	tmp = work[0];
	work[0] = work[1];	
	work[1] = tmp;
	mv( (char *) work, (char *) block );
endproc



/* In-place decryption of 64-bit block */
procedure dedes(block) DD * block;
var
	register integer i;
	DD work[3];	/* Working data storage */
	longint tmp;
code
	mv( (char * ) block, (char *) work );	/* Initial permutation */

	/* Left/right half swap */
	tmp = work[0];
	work[0] = work[1];	
	work[1] = tmp;

	/* Do the 16 rounds in reverse order */
	FOR( i, 0, 15 ) pass( 15 - i, work ); ROF

	mv( (char *)work, (char *) block );
endproc


integer function Hx( c ) char c;
var
code
   IF( c >= '0' and c <= '9' )
      return( c - '0' );
   ELSE
      IF( c >= 'A' and c <= 'F' )
	return( c - 'A' + 10 );
     ELSE 
	return( 0 );
      FI
   FI
endfunc



procedure HexToBin( In, Out ) String In; DD Out[];
var
     integer i;
     String OutC;
code
    OutC = (String) Out;
    FOR( i, 0, 7 )
      OutC[ i ] = ( Hx( In[ i + i ] ) << 4 ) + Hx( In[ i + i + 1 ] );
    ROF
endproc



procedure exxor( v1, v2, out ) String v1; String v2; DD out[];
var
    integer i;
    DD b1[3], b2[3];
    String OutC, bv1, bv2;
code
    OutC = (String) out;
    HexToBin( v1, b1 );
    HexToBin( v2, b2 );
    bv1 = (String) b1;
    bv2 = (String) b2;
    FOR( i, 0, 7 ) OutC[ i ] = bv1[ i ] ^ bv2[ i ]; ROF
    OutC[ 8 ] = EOS;
endproc





procedure desconv( pwrd, key, out ) String pwrd; String key; DD out[];
var
    integer i;
    DD bpwrd[3], bkey[3];
    String OutC;
    integer deserr;
code
  deserr = desinit();
  IF( deserr isnt Okay )
    LOGEMS "DES could not init itself!\n" );
    out[ 0 ] = 0L;
  ELSE
    HexToBin( pwrd, bpwrd );
    HexToBin( key, bkey );
setdeskey( ( String ) bkey );
FOR( i, 0, 1 ) out[ i ] = bpwrd[ i ]; ROF
dedes( out );
OutC = (String) out;
    OutC[ 8 ] = EOS;
    IF( debug )
       LOG "DES convert: key = %.8lX%.8lX\n", bkey[0], bkey[1] );
       LOG "input: %.8lX%.8lX, output = %.8lX%.8lX  '%s'\n",
                     bpwrd[0], bpwrd[1], out[0], out[1],(char *) out);
    FI
  FI
endproc




procedure fwdesconv( pwrd, key, out ) String pwrd; String key; DD out[];
var
    integer i;
    DD bpwrd[3], bkey[3];
    integer deserr;
code
  deserr = desinit();
  IF( deserr isnt Okay )
       LOGEMS "Error initializing DES!!\n" );
  ELSE
    HexToBin( pwrd, bpwrd );
    HexToBin( key, bkey );
    setdeskey( (String) bkey );
    FOR( i, 0, 1 ) out[ i ] = bpwrd[ i ]; ROF
    endes( out );
    IF( debug )
       LOG "DES convert: key = %.8lX%.8lX\n", bkey[0], bkey[1] );
       LOG "input: %.8lX%.8lX, output = %.8lX%.8lX  '%s'\n",
                     bpwrd[0], bpwrd[1], out[0], out[1],(char *) out);
    FI
  FI
endproc



procedure GetTheLock( uname ) String uname;
var
    AllocString lockFile, inmailFile;
    integer f;
    struct stat statbuf;
code
  call sprintf( inmailFile, "%s/%s%s", spfile, uname, SYS_MAILDIR_SUFFIX );  
  call strncpy( tempmailFile, "xyzzy foo", sizeof tempmailFile );
  f = open( inmailFile, O_RDONLY );
  IF( f is Error )
     IF( debug ) LOG "No mail spool file '%s'\n", inmailFile ); FI
     holdf = NIL;
  ELSE
     call close( f );
    call stat( inmailFile, &statbuf );
    IF( statbuf.st_size is 0 )
      IF( debug ) LOG "Spool file exists, but is size 0\n" ); FI
      holdf = NIL;
    ELSE
      call sprintf( lockFile, "%s.lock", inmailFile );
     IF( LockAcquire( lockFile ) is Okay )
	IF( FLockAcquire( inmailFile ) is Okay )
           call sprintf( tempmailFile, "%s.temp", inmailFile );
           Assert( rename( inmailFile, tempmailFile ), "rename spool file" );
	   holdf = open ( tempmailFile, O_RDWR );
           Assert( unlink( tempmailFile ), "delete temp file" );
	   FLockRelease();
	FI
        LockRelease( lockFile );
     FI
  FI
FI
endfunc


procedure BeginSession()
var
    String crypt();
    static char userbuf[128] = "USER=";
    static char luserbuf[128] = "LOGNAME=";
    static char homebuf[128] = "HOME=";
code
          LOG SEPARATOR );
          LOG "Starting session #%ld for '%s'\n" , (long) Connections, pwd->pw_name );
		  it.it_interval.tv_sec = it.it_interval.tv_usec = 0;
          it.it_value.tv_sec = 1000000; it.it_value.tv_usec = 0;
          Assert( setitimer( ITIMER_VIRTUAL, &it, &dummy ), "set timer" );
	  GetTheLock( pwd->pw_name );
          IF( ouruid is RootUID )
	     Assert( setgid (pwd->pw_gid), "set group id" );
             Assert( initgroups(pwd->pw_name, pwd->pw_gid), "init groups" );
             Assert( setuid (pwd->pw_uid), "set user id" );
          FI
	  setenv("USER", pwd->pw_name, userbuf);
          setenv("LOGNAME", pwd->pw_name, luserbuf);
          setenv("HOME", pwd->pw_dir, homebuf);
          Assert( chdir (pwd->pw_dir), "go to your home directory" );
          ensure_dir( TempDir );
          ensure_dir( DEF_FOLDER );
          GetCurrentMail( pwd->pw_name );
	  call close( holdf );
endproc




procedure BadLogin()
var
code
    SendToNet( "- login incorrect." );
endproc

struct passwd * function FetchPW( name ) String name;
var 
code
   return( getpwnam( name ) );
endfunc


procedure RawUser( user, passwrd ) String user; String passwrd;
var
    String namep, crypt();
    integer p;
code
    p = strlen( passwrd ) - 1;
    WHILE( p > 0 and passwrd[ p ] is ' ' )
	 passwrd[ p ] = EOS;
	 p = p - 1;
    ENDW
    IF( debug ) LOG "Looking up user '%s', pwd '%s'\n", user, passwrd ); FI
    pwd = FetchPW( user );
    IF( pwd is NIL )
       BadLogin();
       LOG "Invalid user name during login: '%s'\n", user );
       err = Error;
    ELSE
       namep = crypt(passwrd, pwd->pw_passwd);
       IF( strcmp( namep, pwd->pw_passwd ) isnt Okay )
          BadLogin();
		  LOG "Encrypted password mismatch: /etc/passwd='%s', helo='%s'\n",
		  pwd->pw_passwd, namep );
	      LOG "Wrong password for user '%s': '%s'\n", user, passwrd );
          err = Error;
       ELSE
          BeginSession();
       FI
    FI
endproc





procedure ValidateNewUser ( user, passwrd, level ) String user, passwrd, level;
var
    DD clearpwd[3];
    char unhexpwd[20];
    DD binpw[3];
    integer i, ok;

code
   switch( level[0] ) {
   default:
   case SLEVELNONE:
	  RawUser( user, passwrd );
	break;
   case SLEVELXOR:
	  exxor( passwrd, (char *) SKHex, clearpwd );
	  RawUser( user, (String) clearpwd );
	break;
   case SLEVELDES:
	  desconv( passwrd, SKHex, clearpwd );
	  RawUser( user, (String) clearpwd );
	break;
   case SLEVELRNX:
	 fwdesconv( SKHex, (String) clearpwd, binpw );
	 HexToBin( passwrd, binpw );
         ok = true;
	 FOR( i, 0, 7 )
	    ok = ok & ( binpw[i] is unhexpwd[i] );
	 ROF
	 IF( ok )
	     pwd = FetchPW( user );
	     IF( pwd is NIL )
		BadLogin();
	     ELSE
		BeginSession();
	     FI
	 ELSE
	     BadLogin();
	 FI
	break;
   }
endproc



function isnumber (s) String s;
var
    integer i;
code
    FOR(i,0,strlen(s)-1)
        IF( not isdigit( s[ i ] ) ) return(false); FI
    ROF 
    return(true);
endfunc


#define	PATH_CMAX	200


function integer GetNumOfMessages( path ) String path;
var
    char thisdir[ PATH_CMAX ];
    integer Count;
 code
      call getwd( thisdir );
      Count = MessageCounter( path );
      call chdir( thisdir );
      return( Count );
endproc


procedure ChangeToFolder(name) String name;
var
    char tbuf[50];
    char FolderName[400];
code
    call strncpy(FolderName, pwd->pw_dir, sizeof FolderName );
    call strncat(FolderName, FOLDER_SUFFIX, sizeof FolderName );
    call strncat(FolderName, name, sizeof FolderName );
    NumOfMessages = MessageCounter( FolderName );
    call sprintf(tbuf, "#%d", NumOfMessages);
    SendToNet (tbuf);
endproc


procedure outToFile (inx) integer inx;
var
    integer c;
    File jfn;
    longint i;
    char FileN[20], obuf[ OBUFSIZE ];
code
    call sprintf( FileN, "%d", inx );
    IF((jfn = fopen (FileN, "r")) is NIL)
        bomb("- Can't open file for send");
 ELSE
    i = 0;
    repeat
        WHILE( i < OBUFSIZE - 1 )
            c = fgetc ( jfn );
            IF(c is EOF) break; FI
            IF(c is EOL) obuf[ inc(i) ] = '\r'; FI
            obuf[ inc(i) ] = c;
        ENDW
        obuf[i] = EOS;
        IF( write(SockOfClient, obuf, WRITETYPE i) < Okay )
            LOGEMS "%d: error writing to net: %d\n", SockOfClient, errno);
            exit( 200 + Error );
        FI
        i = 0;
    until(c is EOF);
    Assert( fclose(jfn), "close file" );
    err = Okay;
    FI
endproc


/*  state processors... */

procedure se()
begin code
   bomb("- error.  This command is not allowed at this time.");;
endproc

procedure doquit()
var
code
   SendToNet("+ Bye.  POP goes the weasel." );
   logout();
   err = Okay;
   state = Done;
endproc

procedure authhelo()
begin code
   ValidateNewUser( PopArg[1], PopArg[2], PopArg[3] );
   IF( err is Okay )
      ChangeToFolder(DEF_FOLDER);
      state = MBOX;
   FI
endproc




procedure dofolder()
var code
    IF( OurArgCount < 1 )
        bomb ("- FOLD requires an argument");
    ELSE
        ChangeToFolder( PopArg[1] );
        state = MBOX;    
    FI
endproc



procedure doread()
var
    AllocString tbuf;
code   
   IF( OurArgCount > 0 )
       CurrentMessage = atoi( PopArg[1] );
    ELSE
       CurrentMessage = 1;
    FI
    IF( CurrentMessage < 1 or CurrentMessage > NumOfMessages )
        call sprintf(tbuf, "- Invalid message number %d", CurrentMessage);
        bomb (tbuf);
    ELSE
        ReturnFileSize();
        IF( err is Okay ) state = ITEM; FI
    FI
endproc




procedure doretr()
var code
     IF(LenOfThisMessage is 0)
         bomb ("- zero length message");
     ELSE
        outToFile( msglist[ CurrentMessage ] );
	inc( Messages );
	err = Okay;
        state = NEXT;
     FI
endproc


procedure AckSaveNext()
var code
    inc(CurrentMessage);
    ReturnFileSize();
    state = ITEM;
endproc



procedure AckDelNext()
var 
   char FileN[20];
code
   call sprintf( FileN, "%d", msglist[ CurrentMessage ] );
   Assert( unlink( FileN ), "unlink file" );
   AckSaveNext();
endproc



procedure nextnack()
var code
  state = ITEM;
endproc


function ChFromNet ()
var
static	integer	ilen = 0;
static	integer	inptr = 0;
static	char	ibuf[ IBUFLEN ];
code
    WHILE( true )
        IF( inptr < ilen ) return(ibuf[ inc(inptr) ]); FI
        ilen = read(SockOfClient, ibuf, IBUFLEN);
        inptr = 0;
        IF( ilen is 0 ) return( EOF ); FI
    ENDW
endfunc



function integer LineFromNet( s, maxlen ) String s; integer maxlen;
var
   integer i;
   integer c;
code
    i = 0;
    WHILE( (c = ChFromNet() ) isnt EOL )
        IF(c is EOF) return ( Error ); FI
        IF(c isnt '\r')
	   IF( i < ( maxlen - 2 ) )
	      s[ i ] = c;
	      inc( i );
           FI
	FI
    ENDW
    s[i] = EOS;
    return (0);
endfunc


function integer CheckForKeyword(s) char s[4];
var
    integer i;
static String keytab[] = { "null",  "helo", "help", "fold", "read",
                   "retr",  "acks", "ackd", "nack", "inqm", "quit" };
code
    WHILE( not isalpha( *s ) and *s isnt EOS )
        inc(s);
    ENDW
    IF( *s isnt EOS )
        FOR(i,0,3)
          IF( isupper(s[i]) ) 
            s[i] = tolower(s[i]);
          FI
        ROF
        FOR(i,1,NKEYS)
            IF( strncmp (keytab[i], s, 4) is Okay )
                return ( i );
            FI
        ROF
    FI
    return ( Okay );
endfunc


function integer GetCmdLine()
var
   AllocString xline;
   integer i;
code
    OurArgCount = -1;
    FOR( i, 0, 3 ) PopArg[i][0] = EOS; ROF
    WHILE( OurArgCount < 0 )
        IF( LineFromNet(xline, sizeof xline ) < Okay ) return ( Error ); FI
        OurArgCount = sscanf(xline, "%s%s%s%s", PopArg[0], PopArg[1], 
				                PopArg[2], PopArg[3] );
           IF( strcmp( "helo", PopArg[0] ) is Okay &&
		PopArg[3][0] < '2' )
              LOG "Command: helo %s xxxxx %s\n", PopArg[1], PopArg[3] );
           ELSE
              LOG "Command: '%s'\n", xline );
           FI
        dec(OurArgCount);
    ENDW
    return( CheckForKeyword( PopArg[0] ) );
endfunc


procedure dohelp()
begin code
  SendToNet( "POP2 Server" );
  SendToNet( "(C) 1989 University of Minnesota" );
  SendToNet( "Microcomputer and Workstation Networks Research Center" );
  SendToNet( "Commands:" );
  SendToNet( "HELP           Displays this list." );
  SendToNet( "HELO usr pwd encr Idents you as a user. Returns # of msgs." );
  SendToNet( "INQM usr       Returns # of mail msgs for usr." );
  SendToNet( "READ X         Returns # of chars in msg #X." );
  SendToNet( "RETR           Returns body of last message." );
  SendToNet( "ACKD           Deletes last msg read." );
  SendToNet( "ACKS           Saves last msg read." );
  SendToNet( "QUIT           Logs you off from POP2." );
endproc

procedure doinqm()
var
    char temp[30];
    char mbox[ PATH_CMAX ];
    struct stat buf;
    integer fi, Count;
    longint Siz;
    struct passwd * userinfo;
code
   call sprintf( mbox, "%s/%s%s", spfile, PopArg[1], SYS_MAILDIR_SUFFIX );
   fi = open( mbox, O_RDONLY );
   IF( fi > 0 )
     call fstat( fi, &buf );
     Siz = buf.st_size;
     call close( fi );
   ELSE
     Siz = 0;
   FI
   userinfo = FetchPW( PopArg[1] );
   IF( userinfo is NIL )
     Count = 0;
   ELSE
     call sprintf( mbox, "%s/Mail/inbox", userinfo->pw_dir );
     Count = GetNumOfMessages( mbox );
   FI
   call sprintf( temp, "+ %d %d", Siz, Count );
   SendToNet( temp );
endproc



procedure MainStateController()
var
     static func stateprocs [Done] [QUIT] = {
  { authhelo, dohelp, se, se,se,se,se,se, doinqm, doquit },
  { se, dohelp, dofolder, doread, se,se,se,se, se, doquit },
  { se, dohelp, dofolder, doread, doretr, se, se, se, se, doquit },
  { se, dohelp, se, se, se, AckSaveNext, AckDelNext, nextnack, se, se }
  };
     AllocString tline;
code  
    token = GetCmdLine();
    IF( token is Error ) state = Done;
    ELSE
    IF( token is 0 )
        PopArg[0][30] = EOS;
	call sprintf( tline, "- Invalid command '%s'", PopArg[0] );
        bomb( tline );
    ELSE
        IF( state >= AUTH and state <= Done )
           (*stateprocs [ state ] [ token - 1 ] )();
        ELSE
           se();
        FI
    FI
   FI
endproc



procedure SetTimeOut()
var
code
          ClearTimeOut();
	  Assert( (int) signal(SIGALRM, TimeOutHandler), "set TimeOut signal" );
           call alarm(CMDTIMEOUT);
	   alarmpending = true;
endproc


procedure SetParentTimeOut()
var
code
	call signal(SIGALRM, TheParentTimeOutHandler);
	call alarm(CONNECTTIMEOUT);
	alarmpending = true;
endproc


procedure MainLoop ()
var code
    SetTimeOut();
#if	AUX
#else
    SetLimits( CLIENTTIMELIMIT, CLIENTFILELIMIT );
#endif
    Messages = 0;
    RedirectSignals( );
    ourpid = getpid();
    parentspid = getppid();
    NewHerald();
    SendToNet(herald);
    repeat
      SetTimeOut();
      MainStateController();
    until( state is Done or err isnt Okay );
    call ShutDownHandler( "Process closing down now" );
endproc


procedure ThingForParentToDo()
begin
code
    Assert( close( SockOfClient ), "close client socket from parent" );
endproc



procedure ConnectionHandler()
var
    unsigned char CA[4];
    struct hostent *fhost;
code
        call time( (time_t *) &timein );
	SetTimeOut();
	bcopy( (String) &ClientAddress.sin_addr.s_addr, (char *) CA, 4 );
        LOG "Client at %d.%d.%d.%d (", CA[0], CA[1], CA[2], CA[3] );
        fhost = gethostbyaddr( (String) &ClientAddress.sin_addr.s_addr,
				 4, AF_INET);
        IF( fhost is NIL ) call fprintf( LOGFILE, "unknown" );
        ELSE call fprintf( LOGFILE, "%s", fhost->h_name); FI
        call fprintf( LOGFILE, ")\n" );
        inc( Connections );
	ClearTimeOut();
	IF( fork() )
           ourpid = getpid();
           parentspid = 0;
           ThingForParentToDo();
        ELSE
           MainLoop();
           exit( err );
        FI
endproc



function exists( fn ) String fn;
var
   File f;
code
   f = fopen( fn, "r" );
   IF( f is NIL )
      return( false );
   ELSE
      call fclose( f );
      return( true );
   FI
endfunc


procedure SendStatistics()
var
   File f;
   AllocString line;
   integer err;
   String mail;

code
 IF( Connections + Messages + Errors + Timeouts > 0 )
 LOG "Saving statistics....\n" );
 f = fopen( TEMPFILE, "w" );
 IF( f isnt NIL )
   call fprintf( f,
                "POPmail totals:\nconnections   messages  errors timeouts\n" );
   call fprintf( f, "%11d%11d%8d%9d\n",
                 Connections, Messages, Errors, Timeouts );
   Connections = Messages = Errors = Timeouts = 0;
   Assert( fclose( f ), "close stat file" );
   IF( exists( PREFMAILER ) )
      mail = PREFMAILER;
   ELSE
      mail = MAILER;
   FI
   call sprintf( line,"/bin/sh -c '%s %s <%s'", mail, POPADMIN, TEMPFILE );
   err = system( line );
   Assert( err, "execute send command" );
   call unlink( TEMPFILE );
 FI
 FI
endproc



procedure PeriodicActions()
var
    time_t t;
    struct tm * TheTime;
    static integer lasthour = 99;
    integer hour;
code
    call time( (time_t *) &t );
    TheTime = localtime( ( long * ) &t );
   hour = TheTime->tm_hour;
   IF( lasthour isnt 99 and lasthour is 23 and hour is 0 )
     SendStatistics();
   FI
   lasthour = hour;
endproc



procedure getargs( ac, av ) int ac; char * av[];
var
	extern int	optind;
	extern char *optarg;
        char * port;
	int c;
	int errflg;

	errflg = 0;
	ofile = of;
	call sprintf( ofile, "%s/%s", LOGFILEDIR, LOGFILENAME );
	spfile = SYS_MAILDIR;
	port = "0";
	debug = false;
	portnum = 9999;
	stats = true;

	WHILE( (c = getopt(ac, av, "sdp:l:m:")) isnt EOF )
		switch (c) {
		case 'd':
			debug++;
			break;
			case 's':
			stats = 0;
			break;
		case 'p':
			port = optarg;
			break;
		case 'l':
			ofile = optarg;
			break;
		case 'm':
			spfile = optarg;
			break;
		case '?':
			errflg++;
			break;
		default: 
		      errflg++;
		       break;
	}
	ENDW
	IF(errflg)
		call fprintf(stderr,
		"usage: %s -d -p 109 -l /usr/adm -m /usr/spool/mail -s\n", av[0]);
		exit (2);
	ELSE	
	   portnum = atoi( port );
	FI	
endproc



procedure main (argc, argv) int argc; String argv[];
var
    int ClientAddress_size;
code
    getargs( argc, argv );
    init( argv[0] );
    IF( debug )
       LOG "POP2 Server listening on socket #%d\n", serversocket );
    FI
    ClientAddress_size = sizeof ClientAddress;
    Assert( listen(serversocket, 10), "listen" );
    WHILE( true )
       	repeat
	  SetParentTimeOut(); CleanUpOurMess(); PeriodicActions();
	  SockOfClient = accept( serversocket, (struct sockaddr *)
				  &ClientAddress, &ClientAddress_size );
	until( not ( SockOfClient is Error and errno is EINTR ) );
    IF( SockOfClient > 0 )
  	 LOG "Accepted connection on socket #%d\n", SockOfClient );
	FI
	Assert( SockOfClient, "accept a connection request" );
	IF( SockOfClient > 0 ) ConnectionHandler(); FI
     ENDW
endproc

