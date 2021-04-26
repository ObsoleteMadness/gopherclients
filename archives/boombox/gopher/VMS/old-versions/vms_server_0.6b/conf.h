/*
 *******************************************************************
 * $Author: lindner $
 * $Revision: 1.1 $
 * $Date: 91/03/25 21:11:32 $
 * $Source: /export/mermaid/staff/lindner/gopherd/RCS/conf.h,v $
 *******************************************************************
 */

/*
**  You'll find all the user configurable parameters here
*/


/*
** This specifies the default port to run at.  Port 70 is the official 
** port number. You can run it on different ports if you want though.  
** This can be overridden on the command line. 
*/

#define GOPHER_PORT	70

/*
** If your hostname command returns the Fully Qualified Domain Name
** (i.e. it looks like foo.bar.edu and not just foo) then make
** the domain name a null string.  Otherwise put in the rest of
** your domain name that `hostname` doesn't return.
*/

#ifndef DOMAIN_NAME
/*#define DOMAIN_NAME ".micro.umn.edu"*/
#define DOMAIN_NAME ""
#endif

/*
** This is the default place where all the data is going to reside.
** 
** It can be overridden on the command line.
*/

#ifndef DATA_DIRECTORY
#define DATA_DIRECTORY "gopher_root:[000000]"
#endif

/*
** Uncomment this out if you are running a system that doesn't have
** dirent directory routines.  Note that Next (and Mach) have the
** dirent routines, but they don't seem to work.
*/

#define BROKENDIRS

#if defined(NeXT) || defined(n16)
#define BROKENDIRS
#endif


/*
**
** This #define is what will be transmitted by the server when it wants to
** refuse service to a client
**
*/

#define BUMMERSTR "We cannot allow off campus connections to this server. Sorry"


/*
** Uncomment this out if you're compiling on a NeXT machine.
**
** Would be nice if I could figure out why the varargs stuff dies without
** this!
*/

#ifdef NeXT
#define __STRICT_BSD__ /**/
#endif

/*
** Uncomment this out if you want to use allow the gopher data server to
** also act as a gopher index server.
*/

#define BUILTIN_SEARCH /**/

/*
** Uncomment *one* of the following search engines.  
**
** Right now we have search engines for NeXT & Wais.
*/

/*#define NEXTSEARCH /**/
#define WAISSEARCH /**/

