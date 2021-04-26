/* globals.h
 *
 * Part of the Internet Gopher program, copyright (C) 1991
 * University of Minnesota Microcomputer Workstation and Networks Center
 *
 * In this file you will find all of the global variables and 
 * global defines.
 */


#define BOOLEAN      int                /* for TRUE/FALSE variables */
#define ROOT_LEVEL   0                  /* and be replaced with linked lists*/
#define ROOT_DIRECTORY "Root Directory" /* label for top level */

#define LINES        24
#define COLS         80
#define MAXRESP      9                  /* Max size of a response list*/
                                        /* Used in ourutil.c */

/*
 * These are some funky defines that assures that global variables are
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


/*** Global variables ***/

EXTERN int     iMenuLines;
EXTERN char    sGClearscreen[HALFLINE];     /* Termcaps screen-clear buffer */
EXTERN char    sGAudibleBell[8];            /* Same, for bell */
EXTERN char    sGCursorDown[10];
EXTERN char    sGCursorUp[10];
EXTERN char    sGHighlighton[20];
EXTERN char    sGHighlightoff[20];

EXTERN char    PrinterCommand[WHOLELINE]; /*used for configuration options */
EXTERN char    PagerCommand[WHOLELINE];   /*used for configuration options */
EXTERN char    TelnetCommand[WHOLELINE];  /*used for configuration options */
EXTERN char    PlayCommand[WHOLELINE];
EXTERN char    EditorCommand[WHOLELINE];

EXTERN WINDOW  *MainWindow; 
EXTERN char    SavedTitle[WHOLELINE];
EXTERN char    *Searchstring INIT(NULL);
EXTERN int     iLevel INIT(0);
EXTERN BOOLEAN SecureMode INIT(FALSE);

EXTERN GopherStruct OldGopher[300]; /* The gopher stacks, replaced in */
EXTERN GopherStruct *Gopher;       /* version 1 with linked lists */


EXTERN char    USERCAP[WHOLELINE];    /* The validated user capability */
EXTERN int     SOUNDCHILD INIT(0);     /* The pid of the sound player child. */

EXTERN int DisplayId;         /*  SMG display device id  */
EXTERN int KeyboardId;        /*  SMG keyboard id  */
EXTERN int PasteId;           /*  SMG pasteboard id  */

#include "version.h"


/*** Externals ***/

extern char **environ;                  /* User environment array */
extern char *sys_errlist[];

/*** DEFINES ***/

#define MAXLINE 512

/*** Prototypes and forward declarations ***/

void init_curses();                     /* lives in manager.c */

/*** Ourutils.c ***/
void display_file(/* char *Filename */);
void ZapCRLF( /* char *buffer */ );
int  outchar( /*char c*/ );        
void exit_curses();
char Mygetstr(/* char * */);
void CursesErrorMsg( /* char* */);
int GetOneOption(/* char*, char* */);
GopherStruct *cruise_dirs(/* GopherStruct* */);

void process_request(/* ZeGopher*/);
void Load_Dir(/*ZeGopher*/);
void Load_Index();
void Load_Index_or_Dir();
int GetOneOption(/* */);
void check_sock(/* int, char* */);
