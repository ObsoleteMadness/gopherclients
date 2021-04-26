
 /*
 *------------------------------------------------------------------
 * $Source: /afs/net.mit.edu/dapps/project/techinfodev/src/srv_ti/RCS/pdb.h,v $
 * $Revision: 1.1 $
 * $Date: 92/08/04 16:28:02 $
 * $Author: ark $
 *------------------------------------------------------------------
 */
/*
 * pdb.h
 */

#include <sys/types.h>

#define THENAME         "PennInfo"

#ifndef TEST
#define STORE_DIRECTORY "/mit/ti_data/"
#else
#define STORE_DIRECTORY "/penninfo/data/"
#endif

#define LOG_MACHINE    "tiserve.mit.edu"


/* #define MAC */

#ifdef MAC
#define	bcopy(src, dest, len)	memcpy(dest, src, len)	
#endif	

#ifndef FALSE
#define	FALSE    	0
#endif
#ifndef TRUE
#define	TRUE     	1
#endif

#define	NO		0
#define	YES		1

#define	CR	'\015'
#define LF	'\012'
#define DLM     ':'
#define SIMPLE          0    /* display types */
#define FULL            1    /* display types */

#define NOMENU          0
#define MENU            1

#define flush()			fflush(stdout)

#ifndef max
#define	max(a, b)	((a) > (b) ? (a) : (b))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#endif

#define do_free(var) if ((var) !=NULL) free((var))

typedef struct server ASERVER;
struct server {
  char *name;
  char *addr;
  char *portnum;
  char *contact;
  char *email;
  char *comment;
  int  idnum;
  int  sock;
};


#define	HIST_STACK_SIZE	1024
#define MAXBACK 100
#define MAXPATHLEN 256
/*
 * see glob.c for these
 */

extern char	HELP_FILE[];
extern char     HELP_FILE_ADV[];
extern char	LOG_FILE[];
extern char     READ_LOG_FILE[];
extern char	WEB_FILE[];
extern char     WEB_BAK_FILE[];
extern char     WEB_TMP_FILE[];
extern char     MSG_FILE[];

int      SERVER; 
extern int SERVER; 
int menu;
extern int menu;
int disptype;
extern int disptype;

char *	domalloc(unsigned long	bytes);
char *  get_token(char *str,int pos);
char *  find_token(char *str,int pos);
char *  textbuf_line(int id);
char *	index();
/* long	atol(char *); for mac */













