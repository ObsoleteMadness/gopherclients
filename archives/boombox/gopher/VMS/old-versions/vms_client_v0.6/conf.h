/* conf.h
 *
 * Part of the Internet Gopher program, copyright (C) 1991
 * University of Minnesota Microcomputer Workstation and Networks Center
 *
 * These are the user definable parameters
 */

#ifndef DEFAULT_HOST
/* #define DEFAULT_HOST     "gopher.micro.umn.edu"  */
#define DEFAULT_HOST     "ummvxm.mrs.umn.edu"
#endif

#ifndef GOPHER_PORT
#define GOPHER_PORT      70
#endif

/*
** redefine GOPHERHELP to point to where you keep the help file that
** the gopher client will display when you press '?'
*/

#define GOPHERHELP	"umm$help:gopher.hlp\0"

/*
** Uncomment out the following if you plan on allowing anonymous logins
** to a gopher account.
*/

/*#define PARANOID /**/

/*
** Uncomment this out if you want to make a client with a slightly
** different user interface that will work for people with really, really
** stupid terminals (i.e. people using VM/CMS ANET for a vt100 terminal
** emulator
*/

/*#define STUPIDTERM  /**/

/*
 *  Try to automatically determine the type of system we're using
 */

#if !defined(IS_A_SUN) && defined(sun)
#define IS_A_SUN
#endif

#if !defined(IS_A_DEC) && defined(ultrix)
#define IS_A_DEC
#endif

#if !defined(IS_NEXT) && defined(NeXT)
#define IS_NEXT
#endif

#ifdef n16
#define IS_BSD
#endif



#ifdef IS_A_SUN
#define PLAY_COMMAND "/usr/local/bin/play -v 40 -"
#define USE_DIRENT
#endif

#ifdef IS_A_DEC
#define SYSVCURSES
#define USE_DIRENT
#endif

#ifdef IS_BSD
#define tolower
#define NO_VPRINTF
#endif

#ifdef STUPIDTERM
#define PAGER_COMMAND "builtin"
#endif

#ifndef PAGER_COMMAND
#define PAGER_COMMAND "type/page"
#endif

#ifndef TELNET_COMMAND
#define TELNET_COMMAND "telnet"
#endif

#ifndef PRINTER_COMMAND
#define PRINTER_COMMAND "print/delete"
#endif

#ifndef PLAY_COMMAND
#define PLAY_COMMAND "/bin/false"
#endif
