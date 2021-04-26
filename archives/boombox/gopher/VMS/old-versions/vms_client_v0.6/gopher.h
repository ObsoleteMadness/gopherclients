/* gopher.h
 *
 * Part of the Internet Gopher program, copyright (C) 1991
 * University of Minnesota Microcomputer Workstation and Networks Center
 *
 */

/*** All our mongo include files ***/

#include <stdio.h>
#include "sys$share:smg$routines.h"
#include "sys$share:smgdef.h"
#include "sys$share:smgtrmptr.h"
#include <curses.h>
#include <perror.h>
#include <unixlib.h>

#if __GNUC__
#include <vms/descrip.h>
#include <vms/ssdef.h>
#else
#include <processes.h>
#include <descrip.h>
#include <ssdef.h>
#endif

/*** Set global configuration options early ***/
#include "conf.h"

#ifndef IS_BSD
#ifndef IS_NEXT
#include <stdlib.h>
#endif
#endif

#if __GNUC__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include <types.h>
#include <socket.h>
#include <in.h>
#endif

#if UCX
#include <ucx$inetdef.h>
#endif

#include <netdb.h>
#include <signal.h>

#include <ctype.h>
#include <errno.h>
#include <time.h>

#define HALFLINE 40
#define WHOLELINE 80

#define MAXSTR 200           /* Arbitrary maximum string length */
                             /* WEIRD BUG!!!! A value greater than
				200 doesn't work on Sparcs!  Why???? */


#define A_FILE      '0'      /* Types of objects returned from a server */
#define A_DIRECTORY '1'
#define A_CSO       '2'
#define A_ERROR     '3'
#define A_INDEX     '7'
#define A_TELNET    '8'
#define A_CNS       '9'
#define A_SOUND     's'
#define A_EVENT     'e'
#define A_CALENDAR  'c'
#define A_EOI	    '.'

#include "gopherstruct.h"
#include "stack.h"

/** Get the configuration variables **/

#include "globals.h"

/** Load in the header files for the various structures **/

#include "forms.h"
