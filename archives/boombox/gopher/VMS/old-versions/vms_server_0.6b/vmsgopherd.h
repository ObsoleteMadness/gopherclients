/*
 *******************************************************************
 * $Author: $
 * $Revision: $
 * $Date: $
 * $Source: $
 *******************************************************************
 */

#include "conf.h"

#include <ctype.h>
#include <stdio.h>
#include <perror.h>
#ifdef __GNUC__
#include <sys/file.h>
/*#include <fcntl.h>*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vms/psldef.h>
#include <vms/lnmdef.h>
#else
#include <file.h>
#include <types.h>
#include <socket.h>
#include <in.h>
#include <netdb.h>
#include <psldef.h>
#include <lnmdef.h>
#endif

#include <errno.h>
#include <string.h>
#include <stat.h>
#include <time.h>

#include "gopherstruct.h"

#ifdef __GNUC__
#include <vms/descrip.h>
#include <vms/rms.h>
#include <vms/iodef.h>
#include <vms/ssdef.h>
#else
#include <descrip.h>
#include <rms.h>
#include <iodef.h>
#include <ssdef.h>
#endif

typedef int boolean;
#define TRUE 1
#define FALSE 0

#define MAXLST 4

/*  Uncomment if you are using UCX  */
/*#define UCX 1  */

/*  Uncomment if you are using Wollongong  */
/*#define WOLLONGONG 1  */

/*  Uncomment if you are using Multinet */
#define MULTINET 1

static struct itmlst {
  unsigned short int length;
  unsigned short int code;
  char *bufadr;
  int *retlen;
} lnmlst[MAXLST];

/*** This one must be last ***/

#include "globals.h"

char *pname;
