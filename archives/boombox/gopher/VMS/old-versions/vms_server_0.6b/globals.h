/*
 *******************************************************************
 * $Author: lindner $
 * $Revision: 1.1 $
 * $Date: 91/03/25 21:12:22 $
 * $Source: /export/mermaid/staff/lindner/gopherd/RCS/globals.h,v $
 *******************************************************************
 */


/*
 * This is some funky defines that assures that global variables are
 * declared only once.  (when globals.c includes this file with EXTERN
 * defined.
 */

#ifndef EXTERN
#define EXTERN extern
#define INIT(x)
#else
#define EXTERN
#define INIT(x) =x
#endif

/**** Defines ****/
#define MAXLINE 512


/**** Globals.  ****/

EXTERN boolean   DEBUG INIT(FALSE);
EXTERN boolean   RunFromInetd INIT(FALSE);
EXTERN char      LOGFile[256];
EXTERN FILE      *LOGFileHandle INIT(NULL);
EXTERN char      SecurityFile[256];
EXTERN FILE      *SECFileHandle INIT(NULL);
EXTERN char      Data_Dir[256];

EXTERN GopherDirObj SortDir;


/*** Prototypes n' externals ****/

extern char *parse_input();
extern int do_command();
void intro_mesg(/* int */);
void listdir();
void printfile();
void echosound();
FILE *specialfile();
