/*** glog33.c -- analysis tool for Unix gopherd logs ***/

/* Define NODETAIL if you don't want detail listings to be kept. Detail 
 * listings can double the pointer memory required and slow things down.
 * If you NEVER want to do detail listings then define it.  On fast machines
 * it really doesn't matter, but on my Amiga I can notice the difference.
 */

/* #define NODETAIL */

/******************** END CONFIGURATION ***************************/
/* That was really hard wasn't it :) */


/* Bug Reports/suggestions goto */
#define EMAIL "awick@csugrad.cs.vt.edu"

#define	GLOG_VERSION "Gopher Log Analyzer 3.3"

/* Version 3.3 7/20/93 jdc@selway.umt.edu
 * fixed up main() routine so that: errors on the command line (or -h as
 * first parameter) cause glog to print the help information and abort.
 * fixed up PrintHelp() routine so help message is more understandable
 * changed FILETYPE character from ' ' to 'I' (What about Image?)
 *
 */
/* Version 3.2 
 * Fixed a small bug with search
 */
/* Version 3.1  7/11/93
 * by: Andy Wick - awick@csugrad.cs.vt.edu
 * Added supported for the "missing" gopher types, time plots, month
 * reports and several other things that I forgot :)
 * Used unprotize from gnu to unprototype the functions for external
 * distrubition.  (My version still has prototypes)
 *
 * Version 3.0 
 * by: Andy Wick - awick@csugrad.cs.vt.edu
 * This version is an almost TOTAL rewrite of glog.  It now reads all
 * the information into memory before it does ANYTHING.  It then goes
 * through the arguments one at a time.  So inorder to effect something
 * you must change it before the report.  ie.  Argument order matters now.
 *
 * Version 2.2
 * by: Chuck Shotton - cshotton@oac.hsc.uth.tmc.edu
 *
 * Version 2.1 
 * by: Michael Mealling - Georgia Institute of Technology
 *       Office of Information Technology
 *       michael.meallingl@oit.gatech.edu
 *       12/29/92
 *
 * Versions 1.0
 * by: Chuck Shotton - U of Texas Health Science Center - Houston,
 *    Office	of Academic Computing
 *    cshotton@oac.hsc.uth.tmc.edu
 *    6/17/92
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


/* GENERAL STUFF */
typedef unsigned char byte;

/* Error log link list */
typedef	struct enode_list {
	char 	 *data;
	struct enode_list *next;
} ELIST_REC, *ELIST_PTR;

/* GOPHER LINE STUFF */

/* These are the different types of data that are currenly reconized*/
#define FILETYPE	'I'
#define BINARYTYPE	'B'
#define SOUNDTYPE	'S'
#define DIRTYPE		'D'
#define MAILDIRTYPE	'M'
#define FTPTYPE		'F'
#define RANGETYPE	'R'
#define SEARCHTYPE	'?'

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
NODE_PTR 	days  = NULL;
NODE_PTR 	dates = NULL;
NODE_PTR 	types = NULL;
NODE_PTR 	times = NULL;

long int 	TotalConnects = 0;

/*
 * Self-Documenting vars, that save memory
 */
char 		*ROOTNAME = "Root Connections";
char		*Days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char		*Months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/* The base file name for gnuplot reports */
char BASE[7] = "gopher";
char *base = BASE;

/* Plot Output. NOT SUPPORTED YET */
#define REPORTOUT	0
#define GNUOUT	 	1
#define HISTOUT		2

/* Type of plot to do */
char OutPlot=GNUOUT;

/* Used to tell the Plot routines that you are starting and stoping */
#define PLOTSTART	(GOPHER_PTR)0
#define PLOTDONE	(GOPHER_PTR)1

/* Width of reports */
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

/* Start stop dates */
char start_date[13], stop_date[13];

/* Start stop months */
int mbegin = 1, mend = 12;

/* The only forward decleration needed, since I wrote this the pascal way,
 * the way all programs should be written. :) You don't need all the stupid
 * forward declerations, or prototypes.
 */
void PrintInfo();


/*******************************/
/* My StrStr, since it is not standard on all unix machines (sun 4.0.3) */
char *mystrstr(s1, s2)
     char *s1;
     char *s2;
{
register int len;

   len = strlen(s2);

   if (len == 0)
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
ELIST_PTR InsertErrorLog(list, data)
     ELIST_PTR list;
     char *data;
{
ELIST_PTR temp, temp2;

   if (NULL == (temp = (ELIST_PTR)malloc(sizeof(ELIST_REC))))
   {
      fprintf(stderr, "Not enough memory to add to ErrorLog\n");
      exit(1);
   }
   if (NULL == (temp->data = (char *)malloc(sizeof(char) * (strlen(data) +1))))
   {
      fprintf(stderr, "Not enough memory to add to ErrorLog\n");
      exit(1);
   }
   strcpy(temp->data, data);
   temp->next = NULL;

   if (list == NULL)
      return (temp);

   for (temp2 = list; temp2->next != NULL ; temp2 = temp2->next);
   temp2->next = temp;

   return(list);
}
#ifndef NODETAIL
/*******************************/
LIST_PTR InsertDetail(list, data)
     LIST_PTR list;
     GOPHER_PTR data;
{
LIST_PTR temp;

      if (NULL == (temp = (LIST_PTR)malloc(sizeof(LIST_REC))))
      {
         fprintf(stderr, "Not enough memory to add to DetailList\n");
         exit(1);
      }
      temp->data = data;
      temp->next = list;
      temp->hits = 1;
      return(temp);
}
#endif
/*******************************/
/* Insert tree_element into the	appropriate symbol table. Increment the
 * number of hits if that element is already present.
 * Insert list_element into linked list	contained in the node that
 * tree_element	was put	in.
 */

NODE_PTR Insert(tree, data, cmp)
     NODE_PTR tree;
     GOPHER_PTR data;
     int (*cmp)();
{
int i;

   if (tree == NULL)
   {
      if (NULL == (tree = (NODE_PTR) malloc(sizeof(NODE_REC))))
      {
         fprintf(stderr, "No memory for InsertHost\n");
	 exit(1);
      }
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
NODE_PTR Find(tree, data, cmp)
     NODE_PTR tree;
     GOPHER_PTR data;
     int (*cmp)();
{
int i;

   if (tree == NULL)
   {
      return (NULL);
   }

   i=cmp(data, tree->data);

   if (i > 0)
      return(Find(tree->right, data, cmp));
   if (i<0) 
      return(Find(tree->left, data, cmp));

   return(tree);
}
/*******************************/
/* Get a single field from temp, and return the new spot */
char *getf(temp, field)
     char *temp;
     char *field;
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
int TypesCmp(a, b)
     GOPHER_PTR a;
     GOPHER_PTR b;
{
   return(a->type - b->type);
}
/*******************************/
int TimesCmp(a, b)
     GOPHER_PTR a;
     GOPHER_PTR b;
{
   return(a->hour - b->hour);
}
/*******************************/
int HostsCmp(a, b)
     GOPHER_PTR a;
     GOPHER_PTR b;
{
   return(strcmp(a->hostname, b->hostname));
}
/*******************************/
int DocsCmp(a, b)
     GOPHER_PTR a;
     GOPHER_PTR b;
{
   return(strcmp(a->path, b->path));
}
/*******************************/
int DaysCmp(a, b)
     GOPHER_PTR a;
     GOPHER_PTR b;
{
   return(a->day - b->day);
}
/*******************************/
int DatesCmp(a, b)
     GOPHER_PTR a;
     GOPHER_PTR b;
{
int i = a->month - b->month;
   if (i == 0)
      return(a->date - b->date);
   else
      return(i); 
}
/*******************************/
byte MonthStr2Num(str)
     char *str;
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
byte DayStr2Num(str)
     char *str;
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
void ADDDATA(data)
     GOPHER_PTR data;
{
   if ((data->month >= mbegin) && (data->month <= mend))
   {
      hosts = Insert(hosts, data, HostsCmp);
      docs  = Insert(docs, data, DocsCmp);
      dates = Insert(dates, data, DatesCmp); /* Slow */
      types = Insert(types, data, TypesCmp);
      times = Insert(times, data, TimesCmp); 
      days  = Insert(days, data, DaysCmp);
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
void ProcessLine(line)
     char *line;
{
GOPHER_PTR data;
short len;
char *temp; /* Used to save line, incase it is needed for cruft */
char junk[1025];
char message1[1024];
char message2[1024];

   if (NULL == (data = (GOPHER_PTR)malloc(sizeof(GOPHER_REC))))
   {
      fprintf(stderr, "Not enough memory. Sorry\n");
      exit(1);
   }

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
   temp = getf(temp, junk); /* What is this number ? */
   temp = getf(temp ,junk); /* hostname */
   if (junk[0] == ':')
   { /* A colon in the hostfield */
      free(data);
      cruft = InsertErrorLog(cruft, line);
      return;
   } 

   if (NULL == (data->hostname = (char *)malloc(sizeof(char) * (strlen(junk)+1))))
   {
      fprintf(stderr, "Not enough memory. Sorry\n");
      exit(1);
   }
   strcpy(data->hostname, junk);

   temp = getf(temp, junk); /* : COLON */
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

/* This one is for that annoying 0.0.0.* IP address then gets stuck
 * in the log when someone is trying to access something you ain't got
 */

   if (strncmp(data->hostname,"0.0.0", 5) == 0)
   { 
      free(data->path);
      free(data->hostname);
      free(data);
      cruft = InsertErrorLog(cruft, line);
      return;
   } 

   if (strcmp(message1, "Root") == 0)
   {
      data->type = DIRTYPE;
      free(data->path);
      data->path = ROOTNAME;
      ADDDATA(data);
   }
   else if ((strcmp(message1, "retrieved") == 0) && (strcmp(data->path, "/") == 0))
   {
      data->type = DIRTYPE;
      free(data->path);
      data->path = ROOTNAME;
      ADDDATA(data);
   }
   else if (strcmp(message1, "search") == 0)
   {
      strcpy(junk, message2);

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
      *temp = '\0'; /* Remove for stuff */
      strcat(junk, " "); /* There is at least one space here */
      strcat(junk, data->path);
      free(data->path);
      data->path = (char *)malloc(sizeof(char)*(strlen(junk)+1));
      strcpy(data->path, junk);
      data->type = SEARCHTYPE;

      ADDDATA(data);
   }
   else if (strncmp(message2, "ftp:", 4) == 0)
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

      ADDDATA(data);
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
      ADDDATA(data);

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
void GatherInfo()
{
char line[1025];

   start_date[0] = '\0';

   while(!feof(stdin))
   {
      fgets(line, 1024, stdin);
      if (feof(stdin))
         break;
      ProcessLine(line);
   }
}

/*******************************/

/* These vars are only valid right after a call to TreeTo?List.  I could have
 *  done some fancy var passing, but why bother. :) 
 */
LIST_PTR GByNum;
int GByNumMax;
int GByNumMin; /* These two will be used for histograms in the future */
	

/*******************************/
void InsertSByNum(data, hits)
     GOPHER_PTR data;
     short int hits;
{
LIST_PTR temp, temp2;

   if (NULL == (temp = (LIST_PTR)malloc(sizeof(LIST_REC))))
   {
      fprintf(stderr, "Not enough memory in InsertByNum\n");
      exit(1);
   }
   temp->data = data;
   temp->next = NULL;
   temp->hits = hits;

/* Figure out some vars */
   if (hits < GByNumMin)
      GByNumMin = hits;
   if (hits > GByNumMax)
      GByNumMax = hits;

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
void InsertUByNum(data, hits)
     GOPHER_PTR data;
     short int hits;
{
LIST_PTR temp;

   if (NULL == (temp = (LIST_PTR)malloc(sizeof(LIST_REC))))
   {
      fprintf(stderr, "Not enough memory in InsertByNum\n");
      exit(1);
   }
   temp->data = data;
   temp->next = NULL;
   temp->hits = hits;

/* Figure out some vars */
   if (hits < GByNumMin)
      GByNumMin = hits;
   if (hits > GByNumMax)
      GByNumMax = hits;

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
 * num of hits 
 */
void TreeToSList(tree)
     NODE_PTR tree;
{
   if (tree == NULL)
      return;
   
   TreeToSList(tree->left);
   InsertSByNum(tree->data, tree->hits);
   TreeToSList(tree->right);
}

/*******************************/
void TreeToUList(tree)
     NODE_PTR tree;
{
/* I did two different routines so it would be faster :).  I know this
 * doesn't follow the logic of the rest of the program, but oh well.
 * Do reverse inorder, so the insert just basicly sticks it at the
 * beginning.  Someone should rewrite this, maybe later :)
 */
   if (tree == NULL)
      return;
   
   TreeToUList(tree->right);
   InsertUByNum(tree->data, tree->hits);
   TreeToUList(tree->left);
}

/*******************************/
NODE_PTR ListToTree(list, cmp)
     LIST_PTR list;
     int (*cmp)();
{
NODE_PTR temptree = NULL;
   for(;list != NULL; list = list->next)
      temptree = Insert(temptree, list->data, cmp);
   return(temptree);
}
/*******************************/
void FreeList(list)
     LIST_PTR list;
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
void FreeTree(tree)
     NODE_PTR tree;
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
/* Given a string and and len, left justify and fill with fill */
void printl(str, len, fill)
     char *str;
     int len;
     char fill;
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
void printc(str, fill)
     char *str;
     char fill;
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
void PrintData(data)
     GOPHER_PTR data;
{
   if (data == NULL)
   {
      printf("Data:\n");
   }
   else
   {
      printf("%c ",data->type);
      printl(data->path, Width - 2, ' ');
   }
}
/*******************************/
void PrintTime(data)
     GOPHER_PTR data;
{
   if (data == NULL)
   {
      printf("Times:\n");
   }
   else
   {
      printf("%2d",data->hour);
      printl("", Width - 2, ' ');
   }
}
/*******************************/
void PrintType(data)
     GOPHER_PTR data;
{
char *temp;
   if (data == NULL)
   {
      printf("Types:\n");
   }
   else
   {
      switch(data->type)
      {
	 case FILETYPE:
	    temp = "File";
	    break;
	 case SOUNDTYPE:
	    temp = "Sound";
	    break;
	 case BINARYTYPE:
	    temp = "Binary File";
	    break;
	 case DIRTYPE:
	    temp = "Directory";
	    break;
	 case MAILDIRTYPE:
	    temp = "Mail Directory";
	    break;
	 case FTPTYPE:
	    temp = "FTP";
	    break;
	 case RANGETYPE:
	    temp = "Range";
	    break;
	 case SEARCHTYPE:
	    temp = "Search";
	    break;
         default:
	    temp = "Unknown";
	    break;
      }
      printl(temp, Width, ' ');
   }
}
/*******************************/
void PrintHost(data)
     GOPHER_PTR data;
{
   if (data == NULL)
   {
      printf("Hosts:\n");
   }
   else
   {
      printl(data->hostname, Width, ' ');
   }

}
/*******************************/
void PrintDay(data)
     GOPHER_PTR data;
{
   if (data == NULL)
   {
      printf("Days:\n");
   }
   else
   {
      printl(Days[data->day-1], Width, ' ');
   }
}
/*******************************/
void PrintDate(data)
     GOPHER_PTR data;
{
   if (data == NULL)
   {
      printf("Dates:\n");
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
void PlotData(rfp, num, data)
     FILE *rfp;
     int num;
     GOPHER_PTR data;
{
   if (data == PLOTSTART)
   {
      fprintf(stderr, "Plot of Data is not currently supported, since I am not quite sure what it is suppose to do.  Mail me ideas: %s\n", EMAIL);

   }

}
/*******************************/
void PlotType(rfp, num, data)
     FILE *rfp;
     int num;
     GOPHER_PTR data;
{
char *temp = NULL;
   if (data == PLOTSTART)
   {
      fprintf(rfp,"set xtics (");
   }
   else if (data == PLOTDONE)
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
      switch(data->type)
      {
	 case FILETYPE:
	    temp = "File";
	    break;
	 case SOUNDTYPE:
	    temp = "Sound";
	    break;
	 case BINARYTYPE:
	    temp = "Binary File";
	    break;
	 case DIRTYPE:
	    temp = "Directory";
	    break;
	 case MAILDIRTYPE:
	    temp = "Mail Directory";
	    break;
	 case FTPTYPE:
	    temp = "FTP";
	    break;
	 case RANGETYPE:
	    temp = "Range";
	    break;
	 case SEARCHTYPE:
	    temp = "Search";
	    break;
         default:
	    temp = "Unknown";
	    break;
      }
      fprintf(rfp,"\"%s\" %d,", temp, num);
   }

}
/*******************************/
void PlotHost(rfp, num, data)
     FILE *rfp;
     int num;
     GOPHER_PTR data;
{
   if (data == PLOTSTART)
   {
      fprintf(stderr, "Plot of Hosts is not currently supported, since I am not quite sure what it is suppose to do.  Mail me ideas: %s\n", EMAIL);

   }
}
/*******************************/
void PlotDay(rfp, num, data)
     FILE *rfp;
     int num;
     GOPHER_PTR data;
{
   if (data == PLOTSTART)
   {
      fprintf(rfp,"set xtics (");
   }
   else if (data == PLOTDONE)
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
void PlotTime(rfp, num, data)
     FILE *rfp;
     int num;
     GOPHER_PTR data;
{
   if (data == PLOTSTART)
   {
      fprintf(rfp,"set xtics (");
   }
   else if (data == PLOTDONE)
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
void PlotDate(rfp, num, data)
     FILE *rfp;
     int num;
     GOPHER_PTR data;
{
   if (data == PLOTSTART)
   {
      fprintf(rfp,"set xtics (");
   }
   else if (data == PLOTDONE)
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
void DoDetail(tree, DetailType)
     NODE_PTR tree;
     byte DetailType;
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
void PrintInfo(tree, print, cmp, DetailType)
     NODE_PTR tree;
     void (*print)();
     int (*cmp)();
     byte DetailType;
{
LIST_PTR temp;
LIST_PTR ByNum;

   GByNum = NULL; /* Init the vars for the TreeToList function */
   TreeToSList(tree);

   if (DetailType != DETAILINFO)
   { /* We are not printing Detail info now, so do headers */
      print(NULL);
      printc("", '=');
   }

   ByNum = GByNum; /* Save off and clear the globals vars, since this */
   GByNum = NULL;

   temp = ByNum;
   while (temp != NULL)
   {
#ifndef NODETAIL
      if (DetailType == DETAILINFO)
         printf("   ");
#endif

      print(temp->data);
      printf(" %4d (%2.2f%%)\n", temp->hits, (float)temp->hits*100.0/TotalConnects);
#ifndef NODETAIL
      if ((DetailType != NOINFO) && (DetailType != DETAILINFO))
         DoDetail(Find(tree, temp->data, cmp), DetailType);
	 /* Don't generate Detail for NOINFO or if we are already doing detail */
#endif
      temp = temp->next;
   }
   printf("\n");
   FreeList(ByNum);
}
/*******************************/
void PlotInfo(tree, plot)
     NODE_PTR tree;
     void (*plot)();
{
LIST_PTR temp;
FILE *rfp, *dfp;
char *fn;
int points = 1;

   if (OutPlot == GNUOUT)
   {
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
   }
   else
   {
      rfp = stdout;
      dfp = stdout;
   }

   plot(rfp, 0, PLOTSTART);
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
   plot(rfp, points, PLOTDONE);
   printf("\n");
   FreeList(GByNum);
}
/*******************************/
void PrintErrorInfo()
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
void PrintHelp()
{
#ifndef NODETAIL
   fprintf(stderr,"Usage: glog -%c [-<SORTTYPE>[<SORTTYPE>]] [OPTIONS] [<logfile]\n", ERRORINFO);
#else
   fprintf(stderr,"Usage: glog -%c [-<SORTTYPE>] [OPTIONS] [<logfile]", ERRORINFO);
#endif
   
   fprintf(stderr, "  glog -h prints this help information\n");
   fprintf(stderr, "  glog -%c displays an ERROR LOG\n",ERRORINFO);
   fprintf(stderr, "  stdin is expected to be your gopher logfile\n\n");
   fprintf(stderr, "SORTTYPE is one of the following\n");
   fprintf(stderr, "   %c = Host Names       %c = Day of Week \n",
  	HOSTINFO, WEEKDAYINFO);
   fprintf(stderr, "   %c = Document Names   %c = Month/Day\n",
	DATAINFO, MONTHDATEINFO);
   fprintf(stderr, "   %c = Type             %c = Time\n\n", TYPEINFO, TIMEINFO);
   fprintf(stderr, "OPTIONS is one of the following\n");
   fprintf(stderr, "   [-b <beginning month #>] [-e <ending month #>]\n");
   fprintf(stderr, "   [-w <width of report>] [-p <SORTTYPE>] PLOT <by SORTTYPE>\n");
   fprintf(stderr, "   [-f <base filename for plot>] [-o <plot outputtype>] (currently unsupported)\n\n");
   fprintf(stderr, "WARNING: -b and -e are applied to the whole command,\n"); 
   fprintf(stderr, "   any other arguments are applied left to right\n\n");
}

/*******************************/
int main(argc, argv)
     int argc;
     char **argv;
{
int i;
char center[80];

#ifdef THINK_C
	argc = ccommand(&argv); 
#endif   
   printf("\n");
   printc(GLOG_VERSION,' ');
   fflush(stdout);

   if  (1 == argc || argv[1][1] =='h')
   {
      PrintHelp(); /* Clueless */
      exit(-1);
   }

   i = 1;
/* We must go through the arguments twice.  Once for the the arguments that
 * can only be set once.  I coult add argument checking here, but why bother
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
	 {
	    fprintf(stderr, "\nexpected beginning month argument for -%c\n", argv[i-1][1]);
	    PrintHelp();
	 }
         break;
      case 'e':
         if (i<argc-1)
	    mend = atoi(argv[++i]);
	 else
	 {
	    fprintf(stderr, "\nexpected ending month argument for -%c\n", argv[i-1][1]);
	    PrintHelp();
	 }
         break;
      }
      i++;
   }
   GatherInfo();
   sprintf(center, "%s to %s", start_date, stop_date);
   printc(center, ' ');
   sprintf(center, "%d Connections", TotalConnects);
   printc(center, ' ');
   printf("\n\n");
   fflush(stdout);
   fflush(stderr);

   i = 1;
   /* Now go through them again and actually do them, from left to right */
   while (i<argc)
   {
      switch (argv[i][1])
      {
      case ERRORINFO:
	 PrintErrorInfo();
	 break;
      case DATAINFO:
	 PrintInfo(docs, PrintData, DocsCmp, argv[i][2]);
	 break;
      case TYPEINFO:
	 PrintInfo(types, PrintType, TypesCmp, argv[i][2]);
	 break;
      case TIMEINFO:
	 PrintInfo(times, PrintTime, TimesCmp, argv[i][2]);
	 break;
      case WEEKDAYINFO:
         PrintInfo(days, PrintDay, DaysCmp, argv[i][2]);
	 break;
      case MONTHDATEINFO:
	 PrintInfo(dates, PrintDate, DatesCmp, argv[i][2]);
	 break;
      case HOSTINFO:
	 PrintInfo(hosts, PrintHost, HostsCmp, argv[i][2]);
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
	    PlotInfo(days, PlotDay);
	    break;
         case MONTHDATEINFO:
	    PlotInfo(dates, PlotDate);
	    break;
         case HOSTINFO:
	    PlotInfo(hosts, PlotHost);
	    break;
	 }
 	 break;
      case 'w':
         if (i<argc-1)
	    Width = atoi(argv[++i]) - WIDTHSUB;
	 else
	 {
	    fprintf(stderr, "expected width argument for -%c\n", argv[i-1][1]);
	    PrintHelp();
	 }
	 break;
      case 'o':
         if (i<argc-1)
	    OutPlot = atoi(argv[++i]);
	 else
	 {
	    fprintf(stderr, "expected output type argument for -%c\n", argv[i-1][1]);
	    PrintHelp();
	 }
	 break;
      case 'f':
         if (i<argc-1)
	    base = argv[++i];
	 else
	 {
	    fprintf(stderr, "expected filename argument for -%c\n", argv[i-1][1]);
	    PrintHelp();
	 }
	 break;
      case 'b':
      case 'e':
         i++;   /* Skip over their arguments */
         break; /* These are before GatherInfo arguments */
      case '?':
      default:
         fprintf(stderr, "Unknown option \"%c\" .  -h for help\n", argv[i][1]);
	 break;
      } /*switch*/
	
      i++; /* next arg...*/
		
   } /*while*/
  
   exit(0);
}
/*******************************/
