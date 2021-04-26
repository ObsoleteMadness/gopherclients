/*
  This software is copyrighted by the University of Pennsylvania.
  Read COPYRIGHT for details.
  */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/dir.h>
#include <sys/stat.h>

#include "pdb.h"
#include "node.h"
#include "messages.h"
#include "network.h"

#include "gophernodes.h"

extern char     *transfields[];
extern int      num_transfields;
extern char     trans_type;
extern char     *msglist[]; /* the list of error messages */
extern char     RESVNODES_FILE[];
extern char     GW_NODES_FILE[];
extern char     GW_NODES_DIR[];
extern char     GW_NODES_BAK_FILE[];
extern unsigned long _allocated;
extern int      strlower(char *str);
extern void     catch_child();
extern int      debug;
extern short    todaysdate;
extern int      next_id;

#define PRIME1   5003
#define PRIME2   1009
  
static struct s1      *a1[PRIME1];   /* gateway info table */
static struct s2      *a2[PRIME2];   /* nodeid table */ 
static struct resvnode reservednodes[MAX_RESV_NODES];
static int            numresvnodes = 0;
static int            numgophnodes = 0;
/* static int            time_lastchecked = 0; */
static int            day_lastrmnodes = 0;

int                   tables_changed = 0;
int                   lastused_changed = 0;
int                   addednewnodes = 0;
int                   savedatastruct_pid = 0;

struct resvnode *get_resvnode(long node)
{
  int idx, foundr;

  for (idx = 0, foundr = 0; !foundr && idx < numresvnodes; ) {
    if (reservednodes[idx].nodeid == node)
      foundr = 1;
    else
      idx++;
  }
      
  if (foundr)
    return ( &(reservednodes[idx]) );
  else
    return ((struct resvnode *) NULL);
}




extern struct s1 *getnode_bynodeid (long nodeid)
{
  struct s2 *nlist;
  int found;
  int h;

  h =  nodeid % PRIME2;
  /* NOT assuming that the lists in a2 are sorted; though it'd be faster */
  for (found=0, nlist = a2[h]; !found && nlist != NULL;) {
    if (nodeid == nlist->nodeid)
      found = 1;
    else
      nlist = nlist->nextnode;
  }
  
  if (nlist == (struct s2 *) NULL) {
    return (struct s1 *)NULL;
  }
  else {
    return (nlist->node);
  }
}




static int
  ghash(char *gophName, char *gophHost, char *gophPort, char *gophPath)
{
  int sum, i, j;
  
  strlower (gophHost);

  sum = 0;
  for ( i=0; gophHost[i] ; i++)
    sum += gophHost[i];

  for ( i=0; gophPath[i]; i++)
    sum += gophPath[i];

  for ( i=0; gophName[i]; i++)
    sum += gophName[i];

  return (((sum + atoi(gophPort)) * 15511) % PRIME1);
}



extern struct s1 *getnode_bygopherinfo (char **gopherfields)
{
  int h;
  struct s1 *temp;
  int found;
  char gophtype;
  char *gophpath, *gophserver, *gophport, *gophtitle;

  gophtype = *(gopherfields[0]);
  gophtitle = gopherfields[0] + 1;
  gophpath = gopherfields[1];
  gophserver = gopherfields[2];
  gophport = gopherfields[3];

  found = 0;
  h = ghash(gophtitle, gophserver, gophport, gophpath);
  
  /*
    For now, assuming that the lists in a1 are NOT sorted.
    To be more efficient, we should make the list be sorted. 
  */

  for (temp = a1[h]; !found && temp != NULL; )
    {
      if (strcmp(temp->gopherport, gophport) == 0 &&
	  strcmp(temp->gopherserver,gophserver) == 0 &&
	  strcmp(temp->gopherpath, gophpath) == 0 &&
	  strcmp(temp->gophertitle, gophtitle) == 0 &&
	  temp->gophertype == gophtype) {
	found = 1;
      }
      else
	temp = temp->nextnode;
    }
  
  if (found)
    return (temp);
  else
    return (struct s1 *)NULL;
}


void reservednodes_unload (void)
{
  int n;

  if (debug)
    fprintf (stderr, "Freeing %d reserved nodes\n", numresvnodes);

  for (n = 0; n < numresvnodes; n++) {
    do_free (reservednodes[n].title);
    do_free (reservednodes[n].file);
    
    if (reservednodes[n].children)
      free (reservednodes[n].children);
    reservednodes[n].numchildren = 0;
  }
  numresvnodes = 0;
}


/*
  MODIFIED make_link_block code from MIT (it had a bug) --lam
  */

static long * make_link_block (char links[], short *linkcnt, int chknodeids)
{
  char      *cp;
  int       numlink;
  long      *nid_block;
  int       chadded;
  int       c, c2;
  long      nid;
  short     repeat;
  short     nodexists;       
  struct    s1 *tempnode;
  
  numlink = *links ? strcnt(links, ',') + 1 : 0;
  nid_block = (long *) domalloc((unsigned int) numlink * sizeof(long));
  
  cp = links;
  chadded = 0;
  for (c = 0; c < numlink; c++) {
    nid = atol(cp);

    /* if nodeid is not a reserved node and is also not in a1[] table,
       then don't add it */

    if (is_reserved_node (nid))
      nodexists = 1;
    else {
      if (getnode_bynodeid(nid) != (struct s1 *) NULL)
	nodexists = 1;
      else
	nodexists = 0;
    }

    /* no repeats allowed */
    for (repeat = 0, c2 = 0; nodexists && c2 < c && !repeat; c2++)
      if (nid_block[c2] == nid)
	repeat++;
    
    if (!repeat && nodexists) {
      nid_block[chadded] = nid;
      chadded++;
    }
    /* MIT's BUG: if (repeat) continue -- but it doesn't increment cp */
    cp = index(cp, ',') + 1;
  }
  *linkcnt = chadded;
  /* chadded is better than numlink because of repeats --lam */
  return nid_block;
}




int reservednodes_load(int unloadfirst)
{
  char *resvfields[NUMFIELDS_RESVNODES];
  FILE *resvfile;
  char line[NLINE_MAXLEN];
  int numlines = 0;
  int numf;
  
  resvfile = fopen (RESVNODES_FILE, "r");
  
  if (resvfile == NULL) {
    perror (RESVNODES_FILE);
    return 0;
  }
  
  if (unloadfirst)
    reservednodes_unload();
  
  fprintf (stderr, "Reading reserved nodes from %s...", RESVNODES_FILE);
  
  numresvnodes = 0;
  while (fgets (line, NLINE_MAXLEN, resvfile) != NULL) {
    if (*line == '#')
      continue;
    numlines++;
    
    parsefields (line, resvfields, &numf, DLM, NUMFIELDS_RESVNODES);
    if (numf >= NUMFIELDS_RESVNODES) {
      reservednodes[numresvnodes].nodeid = atoi(resvfields[0]);
      
      reservednodes[numresvnodes].title =
	(char *) domalloc (strlen(resvfields[1]) + 1);
      strcpy (reservednodes[numresvnodes].title, resvfields[1]);
      
      reservednodes[numresvnodes].file =
	(char *) domalloc (strlen(resvfields[2]) + 1);
      strcpy (reservednodes[numresvnodes].file, resvfields[2]);
      
      reservednodes[numresvnodes].children = make_link_block
	(resvfields[3], &(reservednodes[numresvnodes].numchildren), 1);
    }
    else
      fprintf (stderr, "Not enough fields in line %d\n",numlines);
    numresvnodes++;
  }
  
  fclose (resvfile); /* close file */
  fprintf (stderr, "Done.\nRead %d lines, %d reserved nodes. _allocated=%ld bytes\n", numlines,numresvnodes, _allocated);

  return numresvnodes;
}



/* add node to array 1.  */
static void a1_add (struct s1 *nd)
{
  int h;
  
  h = ghash (nd->gophertitle, nd->gopherserver, nd->gopherport, nd->gopherpath);
  nd->nextnode = a1[h];
  /* add nd to the beginning of the list */
  a1[h] = nd;
  return;
}




static void a2_add (struct s1 *nd)
{
  struct s2 *nd2;
  int h;

  /* add new node to the beginning of the list */

  nd2 = (struct s2 *) domalloc (sizeof (struct s2));
  nd2->node = nd;
  nd2->nodeid  = nd->nodeid;

  h = nd->nodeid % PRIME2;  /* which list does it belong on? */
  nd2->nextnode = a2[h];
  a2[h] = nd2;

  return;
}




/* allocate memory for node and put initial values in it */
/* If checkexists && the nodeid or gopherinfo exists already,
   NULL is returned */

struct s1 *
init_gopher_node (char **gofields, long nodeid, short lastused, int checkexists)
{
  struct s1 *nd;
  int found;
  
  if (checkexists) { /*serv,port,path */ /*type-title path server port */
    nd = getnode_bygopherinfo(gofields);
    
    if (nd != NULL) {
      fprintf (stderr, "Gopher info exists (nodeid %d)\n", nodeid);
      return NULL;
    }
    nd = getnode_bynodeid (nodeid);
    
    if (nd != NULL) {
      return NULL;
    }
  }
  
  nd = (struct s1 *) domalloc (sizeof(struct s1));
  nd->nodeid         = nodeid;
  nd->lastused       = lastused;
  nd->gophertype     = *(gofields[0]);
  nd->gophertitle    = (char *)domalloc(strlen(gofields[0])+1);
  nd->gopherpath     = (char *)domalloc(strlen(gofields[1])+1);
  nd->gopherserver   = (char *)domalloc(strlen(gofields[2])+1);
  nd->gopherport     = (char *)domalloc(strlen(gofields[3])+1);
  
  strcpy (nd->gophertitle,    gofields[0]+1);
  strcpy (nd->gopherpath,     gofields[1]);
  strcpy (nd->gopherserver,   gofields[2]);
  strcpy (nd->gopherport,     gofields[3]);

  a1_add(nd);
  a2_add(nd);

  return nd;
}





struct s1 *load_gopher_node (char **gophernodestr)
{
  long nodeid;
  short lastused;
  char *gofields[NUMFIELDS_GOPHER];
  int numf;

  nodeid = atoi(gophernodestr[0]);
  lastused = atoi(gophernodestr[1]);

  if (is_reserved_node(nodeid)) {
    fprintf (stderr, "load_gopher_node: %d is reserved nodeid\n", nodeid);
    return (struct s1 *) NULL;
  }

  else if (GW_NODE_UNUSED(lastused)) {
    fprintf (stderr, "load_gopher_node: Nodeid %d last used %d, too old.\n",
	     nodeid, lastused);
    return (struct s1 *) NULL;
  }

  else {
    parsefields (gophernodestr[2],gofields,&numf,GOPHER_DLM,NUMFIELDS_GOPHER);
    if (numf < NUMFIELDS_GOPHER) { /* gopher info not parseable */
      fprintf (stderr, "Only %d Gopher fields in %s\n",
	       numf, gophernodestr[2]);
      return (struct s1 *) NULL;
    }
    else
      return(init_gopher_node(gofields, nodeid, lastused, 1));
  }
}



void gophernodes_unload ()
{
  int i, numunloaded;
  struct s1 *temp1, *temp2;
  struct s2 *temp3, *temp4;

  for (i = 0, numunloaded = 0; i < PRIME1; i++) {
    for (temp1 = a1[i]; temp1 != NULL; temp1 = temp2 ) {
      temp2 = temp1->nextnode;

      /* free the char data as well... */
      do_free(temp1->gophertitle);
      do_free(temp1->gopherserver);
      do_free(temp1->gopherpath);
      do_free(temp1->gopherport);

      free (temp1);
      numunloaded++;
    }
  }

  if (debug)
    fprintf (stderr, "Freed %d gopher nodes\n", numunloaded);

  for (i = 0; i < PRIME2; i++) {
    for (temp3 = a2[i]; temp3 != NULL; ) {
      temp4 = temp3->nextnode;
      free (temp3); /* there's no other data to worry about here */
      temp3 = temp4;
    } 
  }

  numgophnodes = 0;
}



int gophernodes_load(int unloadfirst)
{
#define ADAY 86400

  int numf;
  char *gonodefields[NUMFIELDS_GOPHNODES];
  int numlines = 0;
  FILE *gophernodesfile;
  char line[NLINE_MAXLEN];

  gophernodesfile = fopen(GW_NODES_FILE, "r");
  if (gophernodesfile == NULL) {
    perror (GW_NODES_FILE);
    return 0;
  }
  
  if (unloadfirst) 
    gophernodes_unload ();
  init_arrays();

  fprintf (stderr, "Reading gopher nodes from %s...", GW_NODES_FILE);
  addednewnodes = 0;
  tables_changed = 0;
  lastused_changed = 0;

  todaysdate = time(0) / ADAY;
  numgophnodes = 0;
  while (fgets(line, NLINE_MAXLEN, gophernodesfile) != NULL) {
    if (*line == '#')
      continue;
    numlines++;

    parsefields (line, gonodefields, &numf, DLM, NUMFIELDS_GOPHNODES);
    if (numf >= NUMFIELDS_GOPHNODES) {
      if (load_gopher_node (gonodefields))
	numgophnodes++;
    }
    else
      fprintf (stderr, "Not enough fields in line %d\n", numlines);
  }

  fclose (gophernodesfile);
  fprintf (stderr, "Done.\nRead %d lines, %d gopher nodes.  _allocated=%ld\n", numlines, numgophnodes, _allocated);

  return numgophnodes;
}    





/* Two arrays of pointers to nodes.  Make them all null to start with */
init_arrays ()
{
  long i;

  for (i = 0; i < PRIME1; i++)
    a1[i] = (struct s1 *) NULL;

  for (i = 0; i < PRIME2; i++)
    a2[i] = (struct s2 *) NULL;
}


/* Load reserved nodes table, gopher nodes tables */
/* maybe add option to load one or the other & not both from disk */
int datastruct_load (int unloadfirst)
{
  int okay;

  okay = gophernodes_load(unloadfirst);
  /*
    get gopher nodes loaded first so that when reserved nodes are loaded,
    no unknown gopher nodes will be added to the reserved table
    */

  if (okay)
    okay = reservednodes_load(unloadfirst);

  get_nextid();
  fprintf (stderr, "Nextid = %d\n", next_id);
  return okay;
}

void cleancache(void)  /* any files whose mtime is older than cache timeout
			  should be removed */
{
  DIR *cachedir;
  struct direct *nextfile;
  struct stat stbuf;
  char filename[MAXPATHLEN];
  long now;
  int numrm;

/*  fprintf (stderr, "Removing old files from %s...", CACHEDIR); */
  cachedir = opendir(CACHEDIR);
  
  if (cachedir == NULL)  /* cache directory is not being used */
    return;

  now = time (0);
  numrm = 0;
  while ( (nextfile = readdir(cachedir)) != NULL ) {
    if (strcmp (".", nextfile->d_name) != 0 &&
	strcmp ("..", nextfile->d_name) != 0) {
      sprintf (filename, "%s%s", CACHEDIR, nextfile->d_name);
      if (stat(filename, &stbuf) >= 0 
	  && (now - stbuf.st_mtime) > CACHETIMEOUT) {
	unlink (filename);
	numrm++;
      }
    }
  }
  (int) closedir(cachedir);
  if (numrm)
    fprintf (stderr, "T=%d. Removed %d files from the cache\n", time(0),numrm);
}


/* Safekeeping:

  If an unreserved node _is_ a child of a reserved 
  node, then it is safe, even if it hasn't been used
  for a while.

  */

static void get_safekeeping (long **safe, int *numsafe)
{
  int r, c;
  int maxsafe;

  /* first, we figure out the max # of nodes that could be safe */
  for (r = 0, maxsafe = 0; r < numresvnodes; r++)
    maxsafe += reservednodes[r].numchildren;

  *safe = (long *) malloc ( (unsigned int)maxsafe * sizeof(long));
  *numsafe = 0;

  for (r = 0, maxsafe = 0; r < numresvnodes; r++) {
    for (c = 0; c < reservednodes[r].numchildren; c++) {
      if ( ! is_reserved_node(reservednodes[r].children[c] ) ) {
	(*safe)[*numsafe] = reservednodes[r].children[c];
	(*numsafe)++;
      }
    }
  }
}



/* a2 is the nodeid table.
   Using the given nodeid, find the corresponding struct s2
   in the nodeid table and remove it from the table
   */
static void  removenode_nodeidtab (long nodeid)
{
  int h, done;
  struct s2 *nd, *b4, *tmp;

  h = nodeid % PRIME2;

  for (done = 0, b4 = NULL, nd = a2[h]; !done && nd != NULL; ) {
    if (nodeid == nd->nodeid) {
      done = 1;
      if (b4 == NULL)
	a2[h] = nd->nextnode; /* nd was the first on the list */
      else
	b4->nextnode = nd->nextnode;

      tmp = nd;
      nd = nd->nextnode;
      free (tmp);              /* get rid of nodeid node */
    }
    else {
      b4 = nd;
      nd = nd->nextnode;
    }
  }
}


/* remove unused should be improved so that it checks to see if
   todaysdate has changed since the last time nodes were removed
   from the tables because the granularity is a day; it's more 
   efficient to avoid checking thousands of nodes unnecessarily */

static void remove_unused(void)
{
  int n;
  struct s1 *node, *b4, *temp;
  long *safelist;
  int numsafe;
  int s, is_safe;
  sigset_t prev_sigmask;
  sigset_t new_sigmask;
  sigset_t temp_sigmask;
  int numremoved = 0;

  if (todaysdate == day_lastrmnodes) {
    return;
  }
  else
    fprintf (stderr, "checking remove_unused (%d != %d)\n", todaysdate, day_lastrmnodes);

  day_lastrmnodes = todaysdate;

  /* block Children from interrupting this delicate stuff */
  sigprocmask (SIG_SETMASK, (sigset_t *)NULL, &prev_sigmask);
  new_sigmask = prev_sigmask | SIGCHLD;
  sigprocmask (SIG_SETMASK, &new_sigmask, NULL);
  /*  sigprocmask (SIG_SETMASK, NULL, &temp_sigmask);
      fprintf (stderr, "Signal mask has been set to %d.\n", temp_sigmask); */

  get_safekeeping(&safelist, &numsafe);
  for (n = 0; n < PRIME1; n++) {
    for (node = a1[n], b4=NULL; node != (struct s1 *) NULL; ) 
      { 
	if ( ! GW_NODE_UNUSED(node->lastused) ) {
	  b4 = node;
	  node = node->nextnode;
	}
	else {  
	  for (s = 0, is_safe = 0; s < numsafe && !is_safe; s++)
	    if (safelist[s] == node->nodeid) is_safe = 1;
	  
	  if ( is_safe ) {
	    b4 = node;
	    node = node->nextnode;
	  }
	  else { 	    /* node should be removed from the tables  */
	    fprintf (stderr, "rm %d (%d)\n",
		     node->nodeid, node->lastused);
	    numremoved++;
	    removenode_nodeidtab (node->nodeid);
	    if (b4 == NULL)  /* node was the first on the list */
	      a1[n] = node->nextnode;
	    else
	      b4->nextnode = node->nextnode;
	    /* leave b4 as it is... */
	    temp = node;
	    node = node->nextnode;
	    free(temp);       /* Get rid of old node */
	  }
	}
      }
  }
  
  if (safelist)
    free (safelist);

  tables_changed += numremoved;

  /* okay, set the signal mask back to whatever it was */
  sigprocmask (SIG_SETMASK, &prev_sigmask, NULL);
  if (numremoved)
    fprintf (stderr, "Removed %d nodes from tables\n", numremoved);
}


static void save_gwnodes (char *tempfilename)
{
  FILE *tempfile;
  int n;
  int g = 0;
  struct s1 *node;
  
  tempfile = fopen (tempfilename, "w");
  if (tempfile == NULL) { /* we're in trouble... */
    fprintf (stderr, "Unable to write gopher nodes temp file\n");
    perror (tempfilename);
  }
  else {
    fprintf (stderr, "%d: Saving gateway nodes to tempfile %s\n",
	     getpid(), tempfilename);
    
    for (n = 0; n < PRIME1; n++) {
      for (node = a1[n]; node != (struct s1 *) NULL; node=node->nextnode)
	{/* go down the list writing out the data to temp file */
	  
	  /*<nodeid>:<lastu>:<cnt>:<inicnt>:<gtype><gtitle>TAB<gpath>TAB<gserver>TAB<gport>*/
	  fprintf(tempfile, "%ld:%d:%c%s\t%s\t%s\t%s\n",
		  node->nodeid, node->lastused,
		  node->gophertype, node->gophertitle, node->gopherpath,
		  node->gopherserver, node->gopherport);
	  g++;
	}
      fflush(tempfile);
    }
    fclose (tempfile);
    
    /* rename() requires the two files to be on the same file system */
    rename(GW_NODES_FILE, GW_NODES_BAK_FILE);
    rename(tempfilename, GW_NODES_FILE);
    chmod(GW_NODES_FILE,0644);
    fprintf(stderr, "%d: wrote %d gopher nodes to disk\n", getpid(), g);
  }
}



/* write gophernodes arrays out to disk */
void save_datastruct ()
{
  char *tempfilename;
  int forkstat;
  union wait status;
  struct rusage rusage;

  (void) cleancache();     /* parent cleans the cache */

  remove_unused();  /* get rid of useless nodeids in the tables */

  if (! tables_changed && ! addednewnodes && ! lastused_changed) {
    return;
  }
  
  get_nextid();
  fprintf (stderr,
	   "T=%d. save_datastruct: added %d newnodes, %d lastused changes, %d table changes, nextid = %d.\n", 
	   time(0), addednewnodes, lastused_changed, tables_changed, next_id);
  
  /* Because these variables need to be known by the parent,
     the child cannot update them */
  tables_changed = 0;
  addednewnodes = 0;
  lastused_changed = 0;

  /* generate temporary file name */
  tempfilename = tempnam(GW_NODES_DIR, "gwnodes");
  forkstat = fork();
  if (forkstat != 0)
    signal(SIGCHLD, catch_child);
  
  if (forkstat <= 0) { /* child or error */
    save_gwnodes (tempfilename);
    if (forkstat == 0) {  /* child */
      exit (0);
    }
  }
  else { /* parent */
    savedatastruct_pid = forkstat;
    fprintf (stderr, "Save_datastruct child is %d.\n", forkstat);
/*    forkstat = wait3(&status, WNOHANG, &rusage); */
    do_free(tempfilename);  /* tempnam called malloc() */
  }
}





void catch_hup(void)
{
  /* print nodes tables */
  int i, j;
  long numnodes, numlists, maxlistlen, listlen;
  struct s1 *nd1;
  struct s2 *nd2;

  maxlistlen = 0;
  numnodes = 0;
  numlists = 0;
  for (i = 0; i < PRIME1; i++) {
    if (a1[i]) {
      fprintf (stderr, "a1[%d]\n", i);
      numlists++;
      listlen = 0;
      for (j=1, nd1 = a1[i];  nd1 != NULL;  j++, nd1 = nd1->nextnode) {
	fprintf (stderr, "%d) %d %ld %ld pa=%s, s=%s, po=%s t=%s\n",
		 j, nd1->lastused, nd1->nodeid, nd1,
		 nd1->gopherpath, nd1->gopherserver, nd1->gopherport,
		 nd1->gophertitle);
	listlen++;
      }
      if (listlen > maxlistlen)
	maxlistlen = listlen;
    }
  }

  fprintf (stderr, "\nNumlists = %d, maxlistlen = %d\n\n", numlists,maxlistlen);

  for (i = 0; i < PRIME2; i++) {
    if (a2[i]) {
      fprintf (stderr, "a2[%d].  ", i);
      for (j=1, nd2 = a2[i];  nd2 != NULL;  j++, nd2 = nd2->nextnode) {
	fprintf (stderr, "%d) %ld %ld %ld\n",
		 j, nd2->nodeid, nd2, nd2->node);
      }
    }
  }
}

