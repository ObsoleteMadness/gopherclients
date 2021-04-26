/* -------------------------------------------------
*	g2fd.c          Gopher to FTP gateway daemon.
*	Version 0.3 Hacked up: April 1992.  Farhad Anklesaria.
*	Based on a Perl story by John Ladwig.
*	Based on a Perl story by Farhad Anklesaria.
*
*	To be run by inetd.
*	For installation, read the companion README.... or
*	if you don't feel like doing that, the brief instructions
*	are below:
*	
*	First edit the local parameters in defines below.
*	Then do a "make g2fd"
*	Then place the binary (g2fd) in some reasonable place,
*		eg in /usr/local/bin
*	Finally, edit /etc/services and /etc/servers or
*	 	/etc/inetd.conf as the case may be.  Kill
*		and restart inetd.
*
*	This file looks best with tabstops set to 4 rather than 8.
---------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
/*==============Local parameters to be edited in===============*/
#define	LOCALHOST	"hell.micro.umn.edu"	/* This host's domain name */
#define	LOCALPORT	70					/* This daemon's port */
#define MAXLOAD		8.0						/* For load limit if > 1 */ 
#define	FTP			"/usr/ucb/ftp"			/* To invoke ftp */
#define	UPTIME		"/usr/ucb/uptime"		/* To check loadavg.  Ick. */
#define	LIST		"/tmp/gftpL+"			/* Temp list file prefix */
#define	DATA		"/tmp/gftpD+"			/* Temp data file prefix */
/*=============================================================*/

#define GFILE	0		/* Gopher item types */
#define GDIR	1
#define GBINHEX	4
#define GDOSB	5
#define GUNIXB	9

#define SLEN	255		/* Generic small buffer length */
#define	TMOUT	1200	/* 20 minutes is plenty long enough */

char	*appname;
char	ftp[SLEN] = FTP;
char	myName[SLEN] = LOCALHOST;
char	tmpList[SLEN] = LIST;
char	tmpData[SLEN] = DATA;
char	query[BUFSIZ];				/* input redirected by inetd */
char 	*host, *thing;				/* pointers into query */
int		gettingFile = 1;
int		gettingBinary = 0;
int		childpid;

main(argc, argv)
	int argc;
	char * argv[];

{
	char buf[BUFSIZ];
	int pid, Cleanup();
	

	appname = argv[0];
	GenerateUniqueFiles(tmpList, tmpData);
	
	if ((childpid = fork()) == -1) {
		Abort("Can't fork.");
	} else if (childpid != 0) {   /* We are the parent and we wait*/
		WaitForChild();
	}
	
	/* If we get here we are the child and we can do some work */
	signal(SIGPIPE, Cleanup);  /* This and the extra fork is paranoia */
	if (LoadTooHigh()) 
			Abort("Too many connections.  Try again later.");
	gets(query);					/* Courtesy of inetd */
	
	SendQuery(query);
	TranslateResults();
	Cleanup();
	exit(0);

}

/*--------------------------------*/

WaitForChild()
{
	int status, RoundEmUp();
	
	signal(SIGALRM, RoundEmUp);
	alarm(TMOUT);
	wait(&status);
	exit(status);
}

/*--------------------------------*/
LoadTooHigh()
{
	char stuff[SLEN];   float load; char *t;
	FILE *fd, *popen();

	if (MAXLOAD <= 1.0) return(0);

	if (((fd = popen(UPTIME, "r")) == NULL) || 
		(fgets(stuff, sizeof stuff, fd) == NULL))
			Abort("Can't determinine load average.");
	pclose(fd);
	t = strtok(stuff, " \t");
	while ((t != NULL) && (strcasecmp(t, "average:") != 0)) {
		t = strtok(NULL, " \t");
	}
	if (t == NULL) Abort("Can't determinine load average.");
	t = strtok(NULL, " \t");
	sscanf(t, "%f", &load);
	return(load > MAXLOAD);
}

/*--------------------------------*/

SendQuery(query)
	char *query;

{
	int sLen, termCh;
	char *at;
	FILE *fd, *popen();

	/*  printf("The full query was %s\n", query);  Debug  */
	
	if ((sLen = strlen(query)) <= 2) Abort("No host name specified.");
	query[--sLen] = '\0';			/* Knock off the CR: host@path/<cr>*/
	host = query;
	at = strchr(query, '@');
	if (at == NULL) Abort("Not a valid ftp query.");
	thing = at + 1;
	*at = '\0'; 			/*Sneakily chop it into two strings*/
	sprintf(ftp + strlen(ftp), " -n %s > %s", host, tmpList);
	if ((fd = popen(ftp, "w")) == NULL) Abort("Can't run ftp.");
	FailErr(fprintf(fd, "user anonymous gopher@%s\n", LOCALHOST));
	
	sLen = strlen(thing);
	termCh = thing[sLen - 1];			/* Grab possible end char: / etc */
	if ((termCh == '*') || (termCh == '@'))    /*  || (termCh == '/')  */
		thing[sLen - 1] = '\0';
	/* printf("At this point host: %s   thing: %s\n", host, thing);	*/
	if (termCh == '/') {	/* We have a directory */
		gettingFile = 0;
		if (strlen(thing) > 0) FailErr(fprintf(fd, "cd \"%s\"\n", thing));
		FailErr(fprintf(fd, "ls -F\n"));
	} else {				/* We have a file */
		gettingFile = 1;
		if (gettingBinary = IsBinaryType(thing))  
				FailErr(fprintf(fd, "binary\n"));
		FailErr(fprintf(fd, "get \"%s\" \"%s\"\n", thing, tmpData));
	}
	FailErr(fprintf(fd, "quit\n"));
	pclose(fd);
	

}

/*--------------------------------*/

TranslateResults()
{
	FILE *fp, *OpenOrDie();
	char buf[BUFSIZ], theName[SLEN];
	int fd, nRead, checkIt;

	checkIt = 1;
	if (gettingFile) {
		if (gettingBinary) {				/* icky binary file */
			/* printf("Whoa!  That's a binary file\n"); */
			fd = open(tmpData, "r");
			while ((nRead = read(fd, buf, sizeof buf)) > 0) {
				write(1, buf, nRead);
			}
			close(fd);
		} else {							/* must be a nice texty file */
			fp = OpenOrDie(tmpData, "r");
			while (fgets(buf, sizeof buf, fp) != NULL) {
				if (checkIt) { /* Just peek at it once */
					checkIt = 0;
					if (NotText(buf)) {
						fclose(fp);
						Abort("Sorry.  File does not appear to contain text.");
					}
				}
				buf[strlen(buf) - 1] = '\0';	/* munge LF */
				FailErr(printf("%s\r\n", buf));
			}
			fclose(fp);
			FailErr(printf(".\r\n"));
		}
	} else {						/* Must be a directory */
		fp = OpenOrDie(tmpList, "r");
		while (fgets(buf, sizeof buf, fp) != NULL) {
			GopherType(buf, theName);
			FailErr(printf("%s\t%s@%s%s\t%s\t%d\r\n", theName, host, thing,
					buf, myName, LOCALPORT));
		}
		fclose(fp);
		FailErr(printf(".\r\n"));
	}
}

/*--------------------------------*/

FILE *OpenOrDie(file, mode)
	char *file, *mode;
{
	FILE *fp, *fopen();
	if ((fp = fopen(file, mode)) != NULL) {
		return(fp);
	} else {
		Abort("Could not complete the transfer.");
	}
}

/*--------------------------------*/
NotText(buf)
	char * buf;
{
	int max;   char *c;
	
	if ((max = strlen(buf)) >= (BUFSIZ - 50)) max = BUFSIZ - 50;
	for (c = buf; c < (buf + max); c++) {
		if (*c > '~') return(1);
	}
	return(0);
}

/*--------------------------------*/

Abort(complaint)
	char *complaint;

{
	printf("3 Error: %s\r\n.\r\n", complaint); 
	Cleanup();
	exit(1);
}

/*--------------------------------*/

IsBinaryType(thing)
	char *thing;
{
	static char *binExt[] = {
		".zip", ".zoo", ".arj", ".arc", ".lzh", ".hyp", ".pak", ".exe", ".com",
		".ps", ".gif", ".pict", ".pct", ".tiff", ".tif", ".tar", ".Z"
	};

	int extType, i;
	
	for (extType = 0; extType < 17; extType++) { 
		i = strcasecmp(thing + strlen(thing) - strlen(binExt[extType]),
			 			binExt[extType]);
		if (i == 0) return(1);
	}
	return(0);

}

/*--------------------------------*/

GenerateUniqueFiles(tmpList, tmpData)
	char *tmpList, *tmpData;
{
	char *s;
	int pid;
	
	pid = getpid();
	s = strchr(tmpList, '+');
	sprintf(s, "%d", pid);
	s = strchr(tmpData, '+');
	sprintf(s, "%d", pid);
}

/*--------------------------------*/

GopherType(buf, theName)
	char *buf, *theName;

{
	static char ext4[] = ".hqx";
	static char *ext5[] = {".zip", ".zoo", ".arj", ".arc", ".lzh", ".hyp", 
						".pak", ".exe", ".com", ".ps", ".gif", ".pict", 
						".pct", ".tiff", ".tif"};
	static char *ext9[] = {".tar", ".Z"};
	int extType, i, last;
	char	tmpName[SLEN];	
	
	last = strlen(buf) - 1;     buf[last--] = '\0';  /* Munge the LF */
	strcpy(tmpName, buf);
	if (buf[last] == '/') {
		tmpName[last] = '\0';
		sprintf(theName, "%d%s", GDIR, tmpName);
		return;
	}
	if ((buf[last] == '*') || (buf[last] == '@')) {		/* Hack out * and @ */
		buf[last] = '\0';
		tmpName[last] = '\0';
	}
	
	/* At this point we're looking at a file */
	if (strcasecmp(buf + strlen(buf) - strlen(ext4), ext4) == 0) { /* BinHex? */
		sprintf(theName, "%d%s", GBINHEX, tmpName);
		return;
	}

	for (extType = 0; extType < 15; extType++) {			/* PC garbage? */ 
		i = strcasecmp(buf + strlen(buf) - strlen(ext5[extType]), 
						ext5[extType]);
		if (i == 0) {
			sprintf(theName, "%d%s", GDOSB, tmpName);
			return;
		}
	}

	for (extType = 0; extType < 2; extType++) {				/* unix binary? */ 
		i = strcasecmp(buf + strlen(buf) - strlen(ext9[extType]), 
						ext9[extType]);
		if (i == 0) {
			sprintf(theName, "%d%s", GUNIXB, tmpName);
			return;
		}
	}
	
	sprintf(theName, "%d%s", GFILE, tmpName);
	return;		/* Some other and hopefully text file */
}

/*--------------------------------*/

Cleanup()
{
	unlink(tmpList);
	unlink(tmpData);
	exit(1);
}

/*--------------------------------*/

RoundEmUp()
{
	
	kill(childpid, SIGKILL);
	Cleanup();
}

/*--------------------------------*/

FailErr(result)
	int result;
{
	if (result < 0) {
		Cleanup();
	}
}

/*--------------------------------*/
