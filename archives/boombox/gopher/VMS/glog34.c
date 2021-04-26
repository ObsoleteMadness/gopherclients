/*** glog34.c -- analysis tool for Unix gopherd logs ***/
/******************** START CONFIGURATION ***************************/

/* Define NODETAIL if you don't want detail listings to be kept. Detail 
 * listings can double the pointer memory required and slow things down.
 */

/* #define NODETAIL    */  /* uncomment if you DON'T want detail listings */
/* #define NOSEARCHTXT */  /* uncomment if you DON'T want to output WHAT  */
                           /*    was searched for                         */

#define MAXGLINESIZE	500 /* Maximum length of a gopher log line */
/******************** END CONFIGURATION ***************************/
/* That was really hard wasn't it :) */

/* Bug Reports/suggestions goto */
#define EMAIL "awick@csugrad.cs.vt.edu"

#define	GLOG_VERSION "Gopher Log Analyzer 3.4"
/* Version 3.4 4/4/94 awick
 * Improved coding efficiency in ProcessLine() routine
 * Fixed bug when identifying NOPASV ftp types (previously they went
 *  into the error log). If you use the NOPASV patch, you know what I mean.
 * Search Type now shows what word(s) was(were) searched for...UNLESS
 *  NOSEARCHTXT defined
 * Cleaned up the code alot, and changed the way reports are produced
 * Added the -g histograms option
 * Added the -a averaging/estimating stuff
 * Added the -i infile support
 * Added the -o outfile support
 * Added support for VMS dennis_sherman@unc.edu
 *
 * Version 3.3 7/20/93 jdc@selway.umt.edu
 * fixed up main() routine so that: errors on the command line (or -h as
 * first parameter) cause glog to print the help information and abort.
 * fixed up PrintHelp() routine so help message is more understandable
 * changed FILETYPE character from ' ' to 'I' (What about Image?)
 *
 * Version 3.2 
 * Fixed a small bug with search
 *
 * Version 3.1  7/11/93 Andy Wick - awick@csugrad.cs.vt.edu
 * Added supported for the "missing" gopher types, time plots, month
 * reports and several other things that I forgot :)
 * Also added -b and -e options.
 *
 * Version 3.0 
 * by: Andy Wick - awick@csugrad.cs.vt.edu
 * This version is an almost TOTAL rewrite of glog.  It now reads all
 * the information into memory before it does ANYTHING.  It then goes
 * through the arguments one at a time.  So in order to effect something
 * you must change it before the report.  ie.  Argument order matters now.
 *
 * Version 2.2
 * by: Chuck Shotton - cshotton@oac.hsc.uth.tmc.edu
 *
 * Version 2.1 12/29.92
 * by: Michael Mealling - Georgia Institute of Technology
 *       Office of Information Technology
 *       michael.meallingl@oit.gatech.edu
 *
 * Versions 1.0 6/17/92
 * by: Chuck Shotton - U of Texas Health Science Center - Houston,
 *    Office	of Academic Computing
 *    cshotton@oac.hsc.uth.tmc.edu
 */


#include <stdio.h>
#include <string.h>
/* Some machines don't have a stdlib.  You can remove it if need be
 *  it is here for type checking, usually :) 
 */
#include <stdlib.h>
#ifdef THINK_C
#include <console.h> 
#endif

/********************** THINGS THAT EFFECT INPUT/OUTPUT ********************

/* These are the different types of data that are currently reconized. 
   Each must be unique and it only effects the output. */
#define FILETYPE	'I'
#define BINARYTYPE	'B'
#define SOUNDTYPE	'S'
#define DIRTYPE		'D'
#define MAILDIRTYPE	'M'
#define FTPTYPE		'F'
#define RANGETYPE	'R'
#define SEARCHTYPE	'?'

char 	*ROOTNAME = "Root Connections";
char 	*AverageStrs[6] = {""," per year"," per month"," per week"," per day",
			" per hour"};

/* How they are in your gopher log file */
char	*Days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char	*Months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/* The base file name for gnuplot reports */
char DEFAULTBASE[7] = "gopher"; 
/**********************************************************************/

/* GENERAL STUFF */
typedef unsigned char byte;

/* Error log link list */
typedef	struct enode_list {
	char 	 *data;
	struct enode_list *next;
} ELIST_REC, *ELIST_PTR;

/* GOPHER LINE STUFF */

/* One line of the gopher log is stored in here */
typedef struct gopher_line {
   byte 	day;
   byte 	month;
   byte		hour;
   short 	date;
   char 	*hostname;
   char 	*path;
   char 	type;
} GOPHER_REC, *GOPHER_PTR;

/* A Linked list of gopher lines */
typedef	struct node_list {
	GOPHER_PTR 	 data;
        short 		hits;
	struct node_list *next;
}  LIST_REC, *LIST_PTR;

/* Main tree */
typedef	struct node_rec	{
	GOPHER_PTR	data;
#ifndef NODETAIL
	LIST_REC	 *llist;
#endif
        short 		hits;
	struct node_rec	*left, *right;
} NODE_REC, *NODE_PTR;

/* The cruft list is a general list for things that aren't parse-able by
 * ProcessLine().  "cruft" kept for historical reasons.
 */
ELIST_PTR 	cruft = NULL;

/*
 * The following lists are maintained.
 */
NODE_PTR 	hosts = NULL;
NODE_PTR 	docs  = NULL;
NODE_PTR 	dais  = NULL; /* Can't be days because of VMS :) */
NODE_PTR 	dates = NULL;
NODE_PTR 	types = NULL;
NODE_PTR 	times = NULL;

long int 	TotalConnects = 0;

char	*in_file_name = NULL; /* NULL if stdin */

/* Used to tell the Info routines that you are starting and stoping */
#define STARTINFO	(GOPHER_PTR)0
#define ENDINFO		(GOPHER_PTR)1

/* Width of reports, Width is a major hack, so be careful */
byte 	Width 	= 	62; /* 79 - WIDTHSUB */
#define WIDTHSUB 	17  /* The width of the standard print stuff */

/* Information */
   /* Internal */
#define NOINFO		0
#define DETAILINFO	1
   /* Changing these will change the command line options also */
   /* Error log requested, but not a valid SORT TYPE */
#define ERRORINFO	'E'
   /* SORT TYPES */
#define DATAINFO 	'D'
#define HOSTINFO	'H'
#define WEEKDAYINFO	'W'
#define MONTHDATEINFO	'M'
#define TYPEINFO	'T'
#define TIMEINFO	'I'

char *base = DEFAULTBASE;

/* Start stop dates */
char start_date[13], stop_date[13];

/* Start stop months */
int mbegin = 1, mend = 12;

/* Averages */
byte AverageStrPos = 0; /* Everything */
float AverageDiv = 1.0; /* Don't change the value */

/* The only forward decleration needed, since I wrote this the pascal way,
 * the way all programs should be written. :) You don't need all the stupid
 * forward declerations, or prototypes.
 */
void PrintInfo(NODE_PTR tree, void print(GOPHER_PTR), int cmp(GOPHER_PTR a, GOPHER_PTR b), byte DetailType);

/* Simple define that cleans up the code a little */
#define INFOERROR(str, strparam) {fprintf(stderr, (str), (strparam)); exit(1);}
#define EXITERROR(str) {fprintf(stderr, (str)); exit(1);}
#define HELPERROR(str) {fprintf(stderr, (str)); PrintHelp(stderr,0);}

/*******************************/
/* My StrStr, since it is not standard on all unix machines (sun 4.0.3) */
char *mystrstr(char *s1, char *s2)
{
register int len;

   if((len = strlen(s2)) == 0)
      return s1;

   while (*s1 != '\0')
   {
      if (strncmp(s1, s2, len) == 0)
         return s1;
      s1++;
   }

   return NULL;
}
/*******************************/
/* Add item to error log */
ELIST_PTR InsertErrorLog(ELIST_PTR list, char *data)
{
ELIST_PTR temp;
static ELIST_PTR ende;

   if (NULL == (temp = (ELIST_PTR)malloc(sizeof(ELIST_REC))))
      EXITERROR("Not enough memory to add to ErrorLog\n");
   if (NULL == (temp->data = (char *)malloc(sizeof(char) * (strlen(data) +1))))
      EXITERROR("Not enough memory to add to ErrorLog\n");

   strcpy(temp->data, data);
   temp->next = NULL;

   if (list == NULL)
      return (ende = temp);

   ende->next = temp;
   ende = ende->next;

   return(list);
}
#ifndef NODETAIL
/*******************************/
LIST_PTR InsertDetail(LIST_PTR list, GOPHER_PTR data)
{
LIST_PTR temp;

      if (NULL == (temp = (LIST_PTR)malloc(sizeof(LIST_REC))))
         EXITERROR("Not enough memory to add to DetailList\n");
      temp->data = data;
      temp->next = list;
      temp->hits = 1;
      return(temp);
}
#endif
/*******************************/
/* Insert data into tree, if that element is already in the tree
 * then increment the number of hits.  cmp is used to find the location
 * in the tree.
 */

NODE_PTR Insert(NODE_PTR tree, GOPHER_PTR data, int cmp(GOPHER_PTR a, GOPHER_PTR b))
{
int i;

   if (tree == NULL)
   {
      if (NULL == (tree = (NODE_PTR) malloc(sizeof(NODE_REC))))
         EXITERROR("No memory for InsertHost\n");
      tree->data = data;
      tree->left = tree->right = NULL;
#ifndef NODETAIL
      tree->llist = InsertDetail(NULL, data);
#endif
      tree->hits = 1;
      return(tree);
   }

   i=cmp(data, tree->data);

   if (i > 0)
      tree->right = Insert(tree->right, data, cmp);
   else if (i<0) 
      tree->left = Insert(tree->left, data, cmp);
   else
   {
      tree->hits += 1;
#ifndef NODETAIL
      tree->llist = InsertDetail(tree->llist, data);
#endif
   }

   return(tree);
}
/*******************************/
/* Return the node with the data in the tree, using the cmp routine */
NODE_PTR Find(NODE_PTR tree, GOPHER_PTR data, int cmp(GOPHER_PTR a, GOPHER_PTR b))
{
int i;

   if (tree == NULL)
      return (NULL);

   i=cmp(data, tree->data);

   if (i > 0)
      return(Find(tree->right, data, cmp));
   if (i<0) 
      return(Find(tree->left, data, cmp));

   return(tree);
}
/*******************************/
/* Get a single field from temp, and return the new spot */
char *getf(char *temp, char *field)
{
   while(*temp == ' ')
      temp++;

   *field = '\0';
   if (*temp == '\n')
      return(temp);

   while ((*temp != ' ') && (*temp != '\0'))
     *field++ = *temp++;

   *field = '\0';
   return(temp);
}
/*******************************/
int TypesCmp(GOPHER_PTR a, GOPHER_PTR b)
{
   return(a->type - b->type); 
}
/*******************************/
int TimesCmp(GOPHER_PTR a, GOPHER_PTR b)
{ 
   return(a->hour - b->hour); 
}
/*******************************/
int HostsCmp(GOPHER_PTR a, GOPHER_PTR b)
{ 
   return(strcmp(a->hostname, b->hostname)); 
}
/*******************************/
int DocsCmp(GOPHER_PTR a, GOPHER_PTR b)
{ 
   return(strcmp(a->path, b->path)); 
}
/*******************************/
int DaysCmp(GOPHER_PTR a, GOPHER_PTR b)
{ 
   return(a->day - b->day); 
}
/*******************************/
int DatesCmp(GOPHER_PTR a, GOPHER_PTR b)
{
int i = a->month - b->month;
   if (i == 0)
      return(a->date - b->date);
   else
      return(i); 
}
/*******************************/
byte MonthStr2Num(char *str)
{
static char lastmonth[4] = "Jan"; /* Who knows if saving the last month */
static int lastmonthnum = 1;      /* really makes it faster */
int i;

   if (strcmp(lastmonth, str) == 0)
      return(lastmonthnum);

   for(i=0;i<12;i++)
      if (strcmp(Months[i], str) == 0)
      {
         strcpy(lastmonth, Months[i]);
	 lastmonthnum = i+1;
	 return(lastmonthnum);
      }
   return(13); 
}
/*******************************/
byte DayStr2Num(char *str)
{
static char lastday[4] = "Sun"; /* Same here.  Is there a better way? */
static int lastdaynum = 1;
int i;

   if (strcmp(lastday, str) == 0)
      return(lastdaynum);

   for(i=0;i<7;i++)
      if (strcmp(Days[i], str) == 0)
      {
         strcpy(lastday, Days[i]);
	 lastdaynum = i+1;
	 return(lastdaynum);
      }
   return(8); 
}
/*******************************/
/* This routine adds all the data to all the trees.  Now that I have done
 * it this way, I really hate it.  What was I thinking? :)  It makes no
 * sense to have the dates sort be a tree, since it is always the worst
 * case tree :(.  Oh well maybe in 4.0 someone will fix it 
 */
void AddData(GOPHER_PTR data)
{
   if ((data->month >= mbegin) && (data->month <= mend))
   {
      hosts = Insert(hosts, data, HostsCmp);
      docs  = Insert(docs, data, DocsCmp);
      dates = Insert(dates, data, DatesCmp); /* Slow */
      types = Insert(types, data, TypesCmp);
      times = Insert(times, data, TimesCmp); 
      dais  = Insert(dais, data, DaysCmp);
      /* Small hack to have start and stop dates */
      if (start_date[0] == '\0')
      {
         sprintf(start_date, "%s %s %d", Days[data->day-1], Months[data->month-1],
            data->date);
      }
      sprintf(stop_date, "%s %s %d", Days[data->day-1], Months[data->month-1],
         data->date);
      TotalConnects++;
   }
}

/*******************************/
/* Process a single line completely.  It checks to make sure it is a 
 * Valid line, if not it inserts it into the cruft
 */
void ProcessLine(char *line)
{
GOPHER_PTR data;
short len;
char *temp; /* Used to save line, incase it is needed for cruft */
char junk[MAXGLINESIZE];
char message1[MAXGLINESIZE];
char message2[MAXGLINESIZE];

   if (NULL == (data = (GOPHER_PTR)malloc(sizeof(GOPHER_REC))))
      EXITERROR("Not enough memory to process line. Sorry\n");

   temp = line;

   temp = getf(temp, junk); /* Day */
   if (8 == (data->day = DayStr2Num(junk))) 
   { /* Not a real day of week */
      free(data);
      cruft = InsertErrorLog(cruft, line);
      return;
   }
   temp = getf(temp, junk); /* Month */
   if (13 == (data->month = MonthStr2Num(junk)))
   { /* Not a real month */
      free(data);
      cruft = InsertErrorLog(cruft, line);
      return;
   }
   temp = getf(temp, junk); /* Date */
   data->date = atoi(junk);
   temp = getf(temp, junk); /* Time */
   junk[3] = '\0';
   data->hour = atoi(junk); /* Hour */
   temp = getf(temp, junk); /* Year */
   temp = getf(temp, junk); /* Process ID */
   temp = getf(temp ,junk); /* Hostname */
   if (junk[0] == ':')
   { /* A colon in the hostfield...baaad */
      free(data);
      cruft = InsertErrorLog(cruft, line);
      return;
   } 

/* This one is for that annoying 0.0.0.* IP address then gets stuck
 * in the log when someone is trying to access something you ain't got
 */
   if (strncmp(junk,"0.0.0", 5) == 0)
   { 
      free(data);
      cruft = InsertErrorLog(cruft, line);
      return;
   } 

   if (NULL == (data->hostname = (char *)malloc(sizeof(char) * (strlen(junk)+1))))
      EXITERROR("Not enough memory. Sorry\n");
   strcpy(data->hostname, junk);

   temp = getf(temp, junk); /* this should make junk[0]=':' */
   if (junk[0] != ':')
   { /* Now we don't have a colon */
      free(data->hostname);
      free(data);
      cruft = InsertErrorLog(cruft, line);
      return;
   } 
   temp = getf(temp, message1);
   temp = getf(temp, message2);
   while((*temp == ' ') && (*temp != '\0'))
       temp++;
   data->path = (char *)malloc(sizeof(char)*(strlen(temp)+1));
   strcpy(data->path, temp);
   data->path[strlen(temp)] = '\0';


   if (0 != (len = strlen(data->path)))
   {
      if (data->path[len-1] == '\n')
         data->path[len-1] = '\0';
   }

   if (strcmp(message1, "Root") == 0)
   {
      data->type = DIRTYPE;
      free(data->path);
      data->path = ROOTNAME;
      AddData(data);
   }
   else if ((strcmp(message1, "retrieved") == 0) && (strcmp(data->path, "/") == 0))
   {
      data->type = DIRTYPE;
      free(data->path);
      data->path = ROOTNAME;
      AddData(data);
   }
   else if (strcmp(message1, "search") == 0)
   {
      strcpy(junk, message2);
#ifdef NOSEARCHTXT
/*This finds and removes everything after the " for " in a search line*/
      if (strncmp(data->path, "for ", 4) == 0)
      {
         /* We found it at the beginning of data->path */
         temp = data->path;
      }
      else if (NULL == (temp = mystrstr(data->path, " for ")))
      { /* No " for " in the search */
         free(data->path);
         free(data->hostname);
         free(data);
         cruft = InsertErrorLog(cruft, line);
         return;
      }
      *temp = '\0';
#endif
      strcat(junk, " "); /* There is at least one space here */
      strcat(junk, data->path);
      free(data->path);
      data->path = (char *)malloc(sizeof(char)*(strlen(junk)+1));
      strcpy(data->path, junk);
      data->type = SEARCHTYPE;

      AddData(data);
   }
   /* changed from "ftp:",4 to "ftp",3 because of NO-PASV patch */
   /* which uses ftp-vms: and ftp-nopasv to denote special ftp types */
   else if (strncmp(message2, "ftp", 3) == 0)
   {
      strcpy(junk, data->path); /* Incase there was a space in the path */
      free(data->path); /* Then we have to save off path, since it contains it*/
      data->path = (char *)malloc(sizeof(char)*(strlen(message2)+strlen(junk)-2));
      strcpy(data->path, message2+4);
      if (strlen(junk) > 0)
      {
         strcat(data->path, " ");
         strcat(data->path, junk);
      }
      data->type = FTPTYPE;

      AddData(data);
   }
   else if (strcmp(message1, "retrieved") == 0)
   {
      if (data->path[0] == '\0')
      { /* We some how retrieved nothing */
         free(data->path);
         free(data->hostname);
         free(data);
         cruft = InsertErrorLog(cruft, line);
         return;
      } 

      if (strcmp(message2, "directory") == 0)
         data->type = DIRTYPE;
      else if (strcmp(message2, "maildir") == 0)
         data->type = MAILDIRTYPE;
      else if (strcmp(message2, "file") == 0)
         data->type = FILETYPE;
      else if (strcmp(message2, "binary") == 0)
         data->type = BINARYTYPE;
      else if (strcmp(message2, "sound") == 0)
         data->type = SOUNDTYPE;
      else if (strcmp(message2, "range") == 0)
         data->type = RANGETYPE;
      else
      {
         free(data->path);
         free(data->hostname);
         free(data);
         cruft = InsertErrorLog(cruft, line);
         return;
      } 
      AddData(data);

   }
   else /* wasn't anything we know about, g+ maybe?*/
   {
      free(data->path);
      free(data->hostname);
      free(data);
      cruft = InsertErrorLog(cruft, line);
      return;
   } 

   return;
}

/*******************************/
/* The main loop for gathering the data from stdin */
void GatherInfo(void)
{
char line[MAXGLINESIZE];
FILE *fp;

   start_date[0] = '\0';

   if (in_file_name == NULL)
      fp = stdin;
   else if (NULL == (fp = fopen(in_file_name, "r")))
      INFOERROR("Error opening input file %s\n", in_file_name);
   while(!feof(fp))
   {
      fgets(line, MAXGLINESIZE, fp);
      if (feof(fp))
         break;
      ProcessLine(line);
   }
}
/*******************************/
/* These vars are only valid right after a call to TreeTo?List.  I could have
 *  done some fancy var passing, but why bother. :)  All the print/plot/graph
 *  routines should use the G vars.  Which are modified for averages.
 */
LIST_PTR GByNum;
int GByNumMin; /* These two are used for histograms */
int GByNumMax;
int GNodes;
long int GTotalConnects;

/*******************************/
/* Insert the data and number of hits that data got, into a sorted (by hits)
 * link list (GByNum).
 */
void InsertSByNum(GOPHER_PTR data, short hits)
{
LIST_PTR temp, temp2;

   if (NULL == (temp = (LIST_PTR)malloc(sizeof(LIST_REC))))
      EXITERROR("Not enough memory in InsertByNum\n");
   temp->data = data;
   temp->next = NULL;
   temp->hits = hits;

/* Figure out some vars */
   if (hits < GByNumMin)
      GByNumMin = hits;
   if (hits > GByNumMax)
      GByNumMax = hits;
   GNodes++;
   
   if (GByNum == NULL)
      GByNum = temp;
   else if (GByNum->hits < hits)
   {
      temp->next = GByNum;
      GByNum = temp;
   }
   else
   {
      temp2 = GByNum;
      while (temp2->next != NULL)
      {
         if (temp2->next->hits < hits)
	 {
	    temp->next = temp2->next;
	    temp2->next = temp;
	    return;
	 }
	 temp2 = temp2->next;
      }
      temp2->next = temp;
   }
}

/*******************************/
/* Place data and hits onto the front of the list GByNum */
void InsertUByNum(GOPHER_PTR data, short hits)
{
LIST_PTR temp;

   if (NULL == (temp = (LIST_PTR)malloc(sizeof(LIST_REC))))
      EXITERROR("Not enough memory in InsertUByNum\n");
   temp->data = data;
   temp->next = NULL;
   temp->hits = hits;

/* Figure out some vars */
   if (hits < GByNumMin)
      GByNumMin = hits;
   if (hits > GByNumMax)
      GByNumMax = hits;
   GNodes++;

   if (GByNum == NULL)
      GByNum = temp;

   else
   {
      temp->next = GByNum;
      GByNum = temp;
   }
}
/*******************************/
/* I did two different routines so it would be faster :).  I know this
 * doesn't follow the logic of the rest of the program, but oh well.
 * Do Inorder so that they remain in order, if two have the same
 * num of hits.  S<orted> mean use InsertSByNum. U<nsorted> means use
 * InsertUByNum.
 */
void TreeToSList(NODE_PTR tree)
{
   if (tree == NULL)
      return;
   
   TreeToSList(tree->left);
   InsertSByNum(tree->data, tree->hits);
   TreeToSList(tree->right);
}

/*******************************/
/* See above :) */
void TreeToUList(NODE_PTR tree)
{
   if (tree == NULL)
      return;
   
   TreeToUList(tree->right);
   InsertUByNum(tree->data, tree->hits);
   TreeToUList(tree->left);
}

/*******************************/
NODE_PTR ListToTree(LIST_PTR list, int cmp(GOPHER_PTR, GOPHER_PTR))
{
NODE_PTR temptree = NULL;
   for(;list != NULL; list = list->next)
      temptree = Insert(temptree, list->data, cmp);
   return(temptree);
}
/*******************************/
void FreeList(LIST_PTR list)
{
LIST_PTR temp;
   while (list != NULL)
   {
      temp = list;
      list = list->next;
      free(temp);
   }
}
/*******************************/
void FreeTree(NODE_PTR tree)
{
   if (tree == NULL)
      return;
   FreeTree(tree->left);
   FreeTree(tree->right);
#ifndef NODETAIL
   FreeList(tree->llist);
#endif
   free(tree);
   return;
}
/*******************************/
int SizeTree(NODE_PTR tree)
{
   if (tree == NULL)
      return 0;
   return (1 + SizeTree(tree->left) + SizeTree(tree->right));
}
/*******************************/
/* Given a string and and len, left justify and fill with fill */
void printl(char *str, int len, char fill)
{
   while (len > 0)
   {
      if (*str == '\n')
         str++;
      if (*str == '\0')
	 putc(fill, stdout);
      else
         putc(*str++, stdout);
      len--;
   }
}
/*******************************/
/* Given a string center justify and fill with fill */
void printc(char *str, char fill)
{
int i, len = strlen(str);
int start = (Width + WIDTHSUB - len)/2;

   for(i = 0; i < start; i++)
      putc(fill, stdout);

   fputs(str, stdout);

   if (fill != ' ')
      for(i = start+len; i < Width + WIDTHSUB ; i++)
         putc(fill, stdout);
   putc('\n', stdout);
}
/*******************************/
char *TypeNames(char type)
{
   switch(type)
   {
   case FILETYPE:
      return("File");
   case SOUNDTYPE:
      return("Sound");
   case BINARYTYPE:
      return("Binary File");
   case DIRTYPE:
      return("Directory");
   case MAILDIRTYPE:
      return("Mail Directory");
   case FTPTYPE:
      return("FTP");
   case RANGETYPE:
      return("Range");
   case SEARCHTYPE:
      return("Search");
   }
   return("Unknown");
}
/*******************************/
void PrintData(GOPHER_PTR data)
{
   if (data == STARTINFO)
         printf("Data access%s:\n", AverageStrs[AverageStrPos]);
   else if (data == ENDINFO)
   {
      printf("%d nodes accessed.  Average of %d connections per node.\n",
         GNodes, GTotalConnects/GNodes);
   }
   else
   {
      printf("%c ",data->type);
      printl(data->path, Width - 2, ' ');
   }
}
/*******************************/
void PrintTime(GOPHER_PTR data)
{
   if (data == STARTINFO)
      printf("Times%s:\n", AverageStrs[AverageStrPos]);
   else if (data == ENDINFO)
   {
      printf("Average of %d connections per hour.\n",
         GTotalConnects/GNodes);
   }
   else
   {
      printf("%2d:00",data->hour);
      printl("", Width - 5, ' ');
   }
}
/*******************************/
void PrintType(GOPHER_PTR data)
{
char *temp;
   if (data == STARTINFO)
      printf("Types%s:\n", AverageStrs[AverageStrPos]);
   else if (data == ENDINFO)
   {
      printf("Average of %d connections per type.\n",
         GTotalConnects/GNodes);
   }
   else
   {
      temp = TypeNames(data->type);
      printl(temp, Width, ' ');
   }
}
/*******************************/
void PrintHost(GOPHER_PTR data)
{
   if (data == STARTINFO)
      printf("Hosts%s:\n", AverageStrs[AverageStrPos]);
   else if (data == ENDINFO)
   {
      printf("%d hosts connected.  Average of %d connections per host.\n",
         GNodes, GTotalConnects/GNodes);
   }
   else
   {
      printl(data->hostname, Width, ' ');
   }

}
/*******************************/
void PrintDay(GOPHER_PTR data)
{
   if (data == STARTINFO)
      printf("Days%s:\n", AverageStrs[AverageStrPos]);
   else if (data == ENDINFO)
   {
      printf("Average of %d connections per day of week.\n",
         GTotalConnects/GNodes);
   }
   else
   {
      printl(Days[data->day-1], Width, ' ');
   }
}
/*******************************/
void PrintDate(GOPHER_PTR data)
{
   if (data == STARTINFO)
      printf("Dates %s:\n", AverageStrs[AverageStrPos]);
   else if (data == ENDINFO)
   {
      printf("Connections occured on %d different days.\n", GNodes);
      printf("Average of %d connections per day.\n", GTotalConnects/GNodes);
   }
   else
   {
      
      printf("%3s %3s %d", Days[data->day-1], Months[data->month-1], data->date);
      if (data->date < 10)
         printl("\0", Width - 9, ' ');
      else
         printl("\0", Width - 10, ' ');
   }
}
/*******************************/
void PlotData(FILE *rfp, int num, GOPHER_PTR data)
{
   if (data == STARTINFO)
      INFOERROR("Plot of Data is not currently supported. Mail me ideas: %s\n", EMAIL);
}
/*******************************/
void PlotType(FILE *rfp, int num, GOPHER_PTR data)
{
char *temp = NULL;
   if (data == STARTINFO)
   {
      fprintf(rfp,"set xtics (");
   }
   else if (data == ENDINFO)
   {
      fprintf(rfp,"\"\" %d)\n", num);
      fprintf(rfp,"set data style linespoints\n");
      fprintf(rfp,"set tics out\n");
      fprintf(rfp,"set grid\n");
      fprintf(rfp,"set title \"Gopher Usage\"\n");
      fprintf(rfp,"plot \"%s.dat\"\n", base);
   }
   else
   {
      temp = TypeNames(data->type);
      fprintf(rfp,"\"%s\" %d,", temp, num);
   }

}
/*******************************/
void PlotHost(FILE *rfp, int num, GOPHER_PTR data)
{
   if (data == STARTINFO)
      INFOERROR("Plot of Hosts is not currently supported.  Mail me ideas: %s\n", EMAIL);
}
/*******************************/
void PlotDay(FILE *rfp, int num, GOPHER_PTR data)
{
   if (data == STARTINFO)
   {
      fprintf(rfp,"set xtics (");
   }
   else if (data == ENDINFO)
   {
      fprintf(rfp,"\"\" %d)\n", num);
      fprintf(rfp,"set data style linespoints\n");
      fprintf(rfp,"set tics out\n");
      fprintf(rfp,"set grid\n");
      fprintf(rfp,"set title \"Gopher Usage\"\n");
      fprintf(rfp,"plot \"%s.dat\"\n", base);
   }
   else
   {
      fprintf(rfp,"\"%s\" %d,",Days[data->day-1], num);
   }
}
/*******************************/
void PlotTime(FILE *rfp, int num, GOPHER_PTR data)
{
   if (data == STARTINFO)
   {
      fprintf(rfp,"set xtics (");
   }
   else if (data == ENDINFO)
   {
      fprintf(rfp,"\"\" %d)\n", num);
      fprintf(rfp,"set data style linespoints\n");
      fprintf(rfp,"set tics out\n");
      fprintf(rfp,"set grid\n");
      fprintf(rfp,"set title \"Gopher Usage\"\n");
      fprintf(rfp,"plot \"%s.dat\"\n", base);
   }
   else
   {
      fprintf(rfp,"\"%2d\" %d,",data->hour, num);
   }
}
/*******************************/
void PlotDate(FILE *rfp, int num, GOPHER_PTR data)
{
   if (data == STARTINFO)
   {
      fprintf(rfp,"set xtics (");
   }
   else if (data == ENDINFO)
   {
      fprintf(rfp,"\"\" %d)\n", num);
      fprintf(rfp,"set data style linespoints\n");
      fprintf(rfp,"set tics out\n");
      fprintf(rfp,"set grid\n");
      fprintf(rfp,"set title \"Gopher Usage\"\n");
      fprintf(rfp,"plot \"%s.dat\"\n", base);
   }
   else
   {
      if ((data->date == 1) || (data->date == 15) || (num == 1))
         fprintf(rfp,"\"%s/%d\" %d,",Months[data->month-1], data->date, num);
   }
}
#ifndef NODETAIL
/*******************************/
void DoDetail(NODE_PTR tree, byte DetailType)
{
NODE_PTR newtree;
   switch(DetailType)
   {
   case DATAINFO:
      newtree = ListToTree(tree->llist, DocsCmp);
      PrintInfo(newtree, PrintData, DocsCmp, DETAILINFO);
      break;
   case HOSTINFO:
      newtree = ListToTree(tree->llist, HostsCmp);
      PrintInfo(newtree, PrintHost, HostsCmp, DETAILINFO);
      break;
   case WEEKDAYINFO:
      newtree = ListToTree(tree->llist, DaysCmp);
      PrintInfo(newtree, PrintDay, DaysCmp, DETAILINFO);
      break;
   case MONTHDATEINFO:
      newtree = ListToTree(tree->llist, DatesCmp);
      PrintInfo(newtree, PrintDate, DatesCmp, DETAILINFO);
      break;
   case TYPEINFO:
      newtree = ListToTree(tree->llist, TypesCmp);
      PrintInfo(newtree, PrintType, TypesCmp, DETAILINFO);
      break;
   case TIMEINFO:
      newtree = ListToTree(tree->llist, TimesCmp);
      PrintInfo(newtree, PrintTime, TimesCmp, DETAILINFO);
      break;
   default:
      newtree = NULL;
      break;
   }
   FreeTree(newtree);
}
#endif
/*******************************/
void PrintInfo(NODE_PTR tree, void print(GOPHER_PTR), int cmp(GOPHER_PTR a, GOPHER_PTR b), byte DetailType)
{
LIST_PTR temp;
LIST_PTR ByNum;
int Nodes;

   GByNum = NULL; /* Init the vars for the TreeToList function */
   GNodes = 0;
   TreeToSList(tree);

   if (DetailType != DETAILINFO)
   { /* We are not printing Detail info now, so do headers */
      print(STARTINFO);
      printc("", '=');
   }

   ByNum = GByNum; /* Save off and clear the globals vars */
   Nodes = GNodes;
   GByNum = NULL;

   temp = ByNum;
   while (temp != NULL)
   {
#ifndef NODETAIL
      if (DetailType == DETAILINFO)
         printf("   ");
#endif

      print(temp->data);
      printf(" %4d (%2.2f%%)\n", (int)(temp->hits/AverageDiv), (float)temp->hits*100.0/TotalConnects);
#ifndef NODETAIL
      if ((DetailType != NOINFO) && (DetailType != DETAILINFO))
         DoDetail(Find(tree, temp->data, cmp), DetailType);
	 /* Don't generate Detail for NOINFO or if we are already doing detail */
#endif
      temp = temp->next;
   }
   if (DetailType != DETAILINFO)
   { /* We are not printing Detail info now, so do footers */
      printf("\n");
      GNodes = Nodes; /* Restore, incase detail messed it up */
      GTotalConnects = TotalConnects/AverageDiv;
      print(ENDINFO);
   }
   printf("\n");
   FreeList(ByNum);
}
/*******************************/
void PlotInfo(NODE_PTR tree, void plot(FILE *, int, GOPHER_PTR))
{
LIST_PTR temp;
FILE *rfp, *dfp;
char *fn;
int points = 1;

   fn = (char *)malloc(strlen(base) + 5);
   sprintf(fn,"%s.run", base);
   if (NULL == (rfp = fopen(fn, "w")))
   {
      fprintf(stderr, "Could not open file \"%s\" for plot run\n", fn);
      free(fn);
      return;
   }
   sprintf(fn,"%s.dat", base);
   if (NULL == (dfp = fopen(fn, "w")))
   {
      fprintf(stderr, "Could not open file \"%s\" for plot data\n", fn);
      free(fn);
      return;
   }
   free(fn);

   plot(rfp, 0, STARTINFO);
   GByNum = NULL; /* Init the vars for the TreeToList function */
   GByNumMax = 0;
   GByNumMin = 36000;
   TreeToUList(tree);

   temp = GByNum;
   while (temp != NULL)
   {
      plot(rfp, points, temp->data);
      fprintf(dfp, "%d %d\n", points++, temp->hits);
      temp = temp->next;
   }
   plot(rfp, points, ENDINFO);
   printf("\n");
   FreeList(GByNum);
   fclose(rfp);
   fclose(dfp);
}
/*******************************/
void GraphInfo(NODE_PTR tree, void graph(GOPHER_PTR), int strwidth)
{
LIST_PTR temp;
LIST_PTR ByNum;
int Nodes;
int i, max, oldwidth = Width, MyWidth;


   GByNum = NULL; /* Init the vars for the TreeToList function */
   GNodes = 0;
   GByNumMax = 0;
   GByNumMin = 36000;
   TreeToUList(tree);

   ByNum = GByNum; /* Save off and clear the globals vars */
   Nodes = GNodes;
   GByNum = NULL;

   graph(STARTINFO);
   printc("", '=');

   /* Do the width stuff after printc, yes its a major hack. :) */
   MyWidth = Width - strwidth;
   Width = strwidth;

   temp = ByNum;
   while (temp != NULL)
   {
      graph(temp->data);
      printf(" %4d (%5.2f%%) ", (int)(temp->hits/AverageDiv), (float)temp->hits*100.0/TotalConnects);
      max = (int)((temp->hits/AverageDiv) * (MyWidth/(float)GByNumMax));
      for(i=0; i < max; i++)
	 printf("#");
      printf("\n");
      temp = temp->next;
   }
   GNodes = Nodes; /* Restore */
   GTotalConnects = TotalConnects/AverageDiv;
   printf("\n");
   graph(ENDINFO);
   printf("\n");
   FreeList(ByNum);
   Width=oldwidth;
}
/*******************************/
void PrintErrorInfo(void)
{
ELIST_PTR temp = cruft;

   printf("Exception/Problem Report\n");
   printf("NOTE: THESE ENTRIES MAY DENOTE A SERVER PROBLEM. THEY SHOULD BE LOOKED OVER!\n");
   printc("", '=');
   while (temp != NULL)
   {
      printf(temp->data);
      temp = temp->next;
   }
   printf("\n");
}

/*******************************/

void PrintHelp(FILE *fp, int needreturn)
{
  fprintf(fp,"\nUsage: glog [<REPORTTYPE> | -%c | -h] ", ERRORINFO);
#ifdef VMS
   fprintf(fp,"[OPTIONS] -i INFILENAME [-o OUTFILENAME]\n");
#else
   fprintf(fp,"[OPTIONS] [-i INFILENAME] [-o OUTFILENAME]\n");
#endif
   fprintf(fp,"  -h prints this help information\n");
   fprintf(fp,"  -%c displays an ERROR LOG\n", ERRORINFO);
   fprintf(fp,"  -i INFILENAME specifies the input file (your gopher logfile)\n");
#ifndef VMS
   fprintf(fp,"     stdin is expected to be your gopher logfile, unless -i is used\n");
#endif
   fprintf(fp,"  -o OUTFILENAME specifies the output file.  If an output file\n");
   fprintf(fp,"     is not specified, output will be to the screen.\n\n");
   
   fprintf(fp,"REPORTTYPE is:\n");
   fprintf(fp,"   -g<SORTTYPE> for a histogram\n");
   fprintf(fp,"   -p<SORTTYPE> to output plot data (gnuplot required to display plot)\n");
   fprintf(fp,"   -r<SORTTYPE>[<SORTTYPE>] for a report [with detail]\n\n");
   
#ifdef VMS
   fprintf(fp,"SORTTYPE is: (must be inclosed in quotes)\n");
#else
   fprintf(fp,"SORTTYPE is:\n");
#endif
   fprintf(fp,"   %c = Host Names       %c = Day of Week \n",
  	HOSTINFO, WEEKDAYINFO);
   fprintf(fp,"   %c = Document Names   %c = Month/Day\n",
	DATAINFO, MONTHDATEINFO);
   fprintf(fp,"   %c = Type             %c = Time\n\n", TYPEINFO, TIMEINFO);
   if (needreturn)
   {
      fprintf(stderr, "PRESS [ENTER] TO CONTINUE\n");
      fflush(stderr);
      getc(stdin);
   }
   fprintf(fp,"OPTIONS are\n");
   fprintf(fp,"   [-b <beginning month #>] [-e <ending month #>]\n");
   fprintf(fp,"   [-w <width of report>] [-f <base filename for plot>]\n");
   fprintf(fp,"   [-a<AVERAGETYPE>]\n\n");
#ifdef VMS
   fprintf(fp,"AVERAGETYPE is: (must be inclosed in quotes)\n");
#else
   fprintf(fp,"AVERAGETYPE is:\n");
#endif
   fprintf(fp,"   E = Everything       Y = Per Year\n");
   fprintf(fp,"   M = Per Month        W = Per Week\n");
   fprintf(fp,"   D = Per Day          H = Per Hour\n\n");
   fprintf(fp,"WARNING:  All arguments are evaluated from left to right,\n");
   fprintf(fp,"   except for -i, -o, -b, and -e.  Which are evaluated only at\n");
   fprintf(fp,"   the start of execution.  \n\n");

#ifdef VMS
   fprintf(fp,"Example: glog -w 132 -g\"I\" -p\"T\" -a\"Y\" -r\"H\" -i gopher.log\n");
   fprintf(fp,"   (Remember to enclose capital letters in double quotes!)\n");
#else
   fprintf(fp,"Example: glog34 -w 132 -gI -pT -aY -rH < gopher.log | more\n");
#endif /* VMS */
   fprintf(fp,"Width of 132, give a Time Histogram, a Type Plot and a Host Report.\n");
#ifdef VMS
   fprintf(fp,"Input from gopher.log, output to the screen except for the plot data.\n");
#else
   fprintf(fp,"Input from gopher.log output to stdout, except for the plot\n");
#endif /* VMS */
   fprintf(fp, "The report is made to look like the logfile contained a years worth of data.\n\n");

   fflush(fp);
}

/*******************************/
void PrintHdr()
{
char center[80];

   printf("\n");
   printc(GLOG_VERSION,' ');
   fflush(stdout);

   sprintf(center, "%s to %s", start_date, stop_date);
   printc(center, ' ');
   if (AverageStrPos == 0)
   {
      sprintf(center, "%d connections", TotalConnects);
      printc(center, ' ');
   }
   else
   {
      sprintf(center, "%d real connections", TotalConnects);
      printc(center, ' ');
      printf("\n");
      sprintf(center, "%d estimated connections%s", 
          (int)(TotalConnects/AverageDiv), AverageStrs[AverageStrPos]);
      printc(center, ' ');
   }
   printf("\n\n");
   fflush(stdout);
   fflush(stderr);
}
/*******************************/
/* Yes I KNOW main is more than 1 page */
int main(int argc, char **argv)
{ 
int i;
int NumOfDays;

#ifdef THINK_C
	argc = ccommand(&argv); 
#endif   

   if  (1 == argc || argv[1][1] =='h')
   {
      PrintHelp(stdout,1); /* Clueless */
      exit(-1);
   }

   i = 1;
/* We must go through the arguments twice.  Once for the the arguments that
 * can only be set once.  I could add argument checking here, but why bother
 */
   while (i<argc)
   {
      if (argv[i][0] == '-')
      switch (argv[i][1])
      {
      case 'b':
         if (i<argc-1)
	    mbegin = atoi(argv[++i]);
	 else
	    HELPERROR("\nexpected beginning month argument for -b\n");
         break;
      case 'e':
         if (i<argc-1)
	    mend = atoi(argv[++i]);
	 else
	    HELPERROR("\nexpected ending month argument for -e\n");
         break;
      case 'i':
         if (i<argc-1)
	    in_file_name = argv[++i];
	 else
	    HELPERROR("\nexpected input file name for -i\n");
         break;
      case 'o':
         if (i<argc-1)
         {
            i++;
            if (freopen(argv[i], "w", stdout) == NULL)
               INFOERROR("Unable to open requested output file %s\n", argv[i]);
         }
	 else
	    HELPERROR("\nexpected output file name for -o\n");
         break;
      }
      i++;
   }
#ifdef VMS
      /* VMS barfs on stdin as input so FORCE filename */
      if (in_file_name == NULL)
         EXITERROR("No input file name on command line%\n");
#endif /* VMS */

   GatherInfo();

   if (TotalConnects == 0)
      EXITERROR("\nNo [matching] data to process\n");

   NumOfDays = SizeTree(dates);
   fflush(stdout);
   fflush(stderr);

   i = 1;
   /* Now go through them again and actually do them, from left to right */
   while (i<argc)
   {
      switch (argv[i][1])
      {
      case 'a': /* average changes */
         switch(argv[i][2])
	 {
	 case 'E':
	    AverageStrPos = 0;
	    AverageDiv = 1.0;
	    break;
	 case 'Y':
	    AverageStrPos = 1;
	    AverageDiv = NumOfDays/365.0;
	    break;
	 case 'M':
	    AverageStrPos = 2;
	    AverageDiv = NumOfDays/(365.0/12.0);
	    break;
	 case 'W':
	    AverageStrPos = 3;
	    AverageDiv = NumOfDays/7.0;
	    break;
	 case 'D':
	    AverageStrPos = 4;
	    AverageDiv = NumOfDays;
	    break;
	 case 'H':
	    AverageStrPos = 5;
	    AverageDiv = NumOfDays*24.0;
	    break;
	 }
	 break;
      case ERRORINFO:
	 PrintErrorInfo();
	 break;
      case 'r': /*custom reports*/
	 PrintHdr();
         switch(argv[i][2])
	 {
         case DATAINFO:
	    PrintInfo(docs, PrintData, DocsCmp, argv[i][3]);
	    break;
         case TYPEINFO:
	    PrintInfo(types, PrintType, TypesCmp, argv[i][3]);
	    break;
         case TIMEINFO:
	    PrintInfo(times, PrintTime, TimesCmp, argv[i][3]);
	    break;
         case WEEKDAYINFO:
            PrintInfo(dais, PrintDay, DaysCmp, argv[i][3]);
	    break;
         case MONTHDATEINFO:
	    PrintInfo(dates, PrintDate, DatesCmp, argv[i][3]);
	    break;
         case HOSTINFO:
   	    PrintInfo(hosts, PrintHost, HostsCmp, argv[i][3]);
	    break;
	 default:
	    HELPERROR("expected SORTTYPE argument for -r\n");
	    break;
	 }
         printf("\n");
	 break;
      case 'p': /*custom plots*/
         switch (argv[i][2])
	 {
         case DATAINFO:
            PlotInfo(docs, PlotData);
	    break;
         case TYPEINFO:
	    PlotInfo(types, PlotType);
	    break;
         case TIMEINFO:
	    PlotInfo(times, PlotTime);
	    break;
         case WEEKDAYINFO:
	    PlotInfo(dais, PlotDay);
	    break;
         case MONTHDATEINFO:
	    PlotInfo(dates, PlotDate);
	    break;
         case HOSTINFO:
	    PlotInfo(hosts, PlotHost);
	    break;
	 default:
	    HELPERROR("expected SORTTYPE argument for -p\n");
	    break;
	 }
 	 break;
      case 'g': /*custom graphs*/
	 PrintHdr();
         switch (argv[i][2])
	 {
         case DATAINFO:
            GraphInfo(docs, PrintData, 40);
	    break;
         case TYPEINFO:
	    GraphInfo(types, PrintType, 15);
	    break;
         case TIMEINFO:
	    GraphInfo(times, PrintTime,15);
	    break;
         case WEEKDAYINFO:
	    GraphInfo(dais, PrintDay, 15);
	    break;
         case MONTHDATEINFO:
	    GraphInfo(dates, PrintDate,15);
	    break;
         case HOSTINFO:
	    GraphInfo(hosts, PrintHost, 35);
	    break;
	 default:
	    HELPERROR("expected SORTTYPE argument for -g\n");
	    break;
	 }
         printf("\n");
 	 break;
      case 'w':
         if (i<argc-1)
	    Width = atoi(argv[++i]) - WIDTHSUB;
	 else
	    HELPERROR("expected width argument for -w\n");
	 break;
      case 'f':
         if (i<argc-1)
	    base = argv[++i];
	 else
	    HELPERROR("expected filename argument for -f\n");
	 break;
      case 'o':
      case 'i':
      case 'b':
      case 'e':
         i++;   /* Skip over their arguments */
         break; /* These are before GatherInfo arguments */
      case '?':
      default:
         fprintf(stderr, "Unknown option \"%c\" Use -h for help\n", argv[i][1]);
	 break;
      } /*switch*/
	
      i++; /* next arg...*/
		
   } /*while*/
  
   exit(0);
}


