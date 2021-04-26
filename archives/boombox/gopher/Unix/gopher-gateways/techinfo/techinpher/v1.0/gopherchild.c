/*
  This software is copyrighted by the University of Pennsylvania.
  Read COPYRIGHT for details.
  */

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

#include "network.h"
#include "pdb.h"
#include "node.h"
#include "gophernodes.h"
  
extern int      addednewnodes;
extern int      tables_changed;
extern short    todaysdate;
extern pid_t    savedatastruct_pid;
extern struct   s1 *getnode_bygopherinfo(char **fields);
extern struct   s1 *init_gopher_node (char **fields, long nid, short lastu,
				      int chkexists);
extern void     nio_send_file(int s, char *fn, long stpt, long req, int isnlist);
extern void     send_empty_document (int s);
extern void     send_empty_menu (int s);
extern char     *nodeinfostr(long nid, struct s1 *node, int flgfmt);


extern CONN     conntab[];
#define	conn_established(cn)	(conntab[cn].c_socket != -1)

extern void remove_helper(WAITFORHELPER *ptr)
{
  int i, found;

  if (ptr == NULL)
    return;

  kill (ptr->pid, SIGTERM); /* just in case child is out there */
  do_free (ptr->file);
  do_free (ptr->cachefile);
  /* DONT free the ptr->node, it's part of the datastruct */

  for (i=0, found = 0; !found && i < FD_SETSIZE; ) {
    if (conntab[i].c_hptr == ptr)
      found  = 1;
    else
      i++;
  }
  free (ptr);
  if (found) 
    conntab[i].c_hptr = NULL;
  return;
}

int next_id;

/* increments next id in memory + on disk */
/* assumes that get_nextid() has been called already */
static void
inc_nextid(void)
{
  char            line[20];
  int dfd;

  if ((dfd = open(NEXTID_FILE, O_WRONLY | O_TRUNC | O_CREAT)) <= 0)
    {
      printf("error incrementing next id\n");
      return;
    }
  next_id++;
  sprintf(line, "%d\n", next_id);
  write(dfd, line, strlen(line));
  close(dfd);
  return;
}

/* reads the next id from disk */
extern void
  get_nextid(void)
{
  char            line[20];
  int             red;
  int             dfd;
  
  if ((dfd = open(NEXTID_FILE, O_RDONLY)) <= 0){
    printf("error getting next id\n");
    return;
  }
  red = read(dfd, line, 20);
  if (red <= 0){
    printf("error reading next id\n");
    return;
  }
  line[red] = '\0';
  next_id = atoi(line);
  close(dfd);
  return;
}

static void conv_textfile(FILE *ifile, char *filnam)
{
  FILE        *ofile;
  char        line[NLINE_MAXLEN];
  char        *cp;

  ofile = fopen(filnam, "w");
  if (ofile == NULL) {
    fprintf (stderr, "Unable to open temp nlist file\n");
    perror(filnam);
    return;
  }
  while (fgets (line,NLINE_MAXLEN-1,ifile) != NULL) {
    cp = rindex (line, '\r');
    if (cp) 
      if (*(cp+1) == '\n') {
	*cp = '\n';
	*(cp+1) = 0;
      }
    /* gopher protocol: text file ends with a . on a line by itself */
    if (strcmp(line, ".\n") == 0)
      break;
    else
      fprintf (ofile, "%s", line);
  }
  fclose (ofile);
}


static void child_punt (char gophertype, long nodeid, int sock, int flgsfmt)
{
  char *cp;

  if (gophertype == GOPHTYP_TEXT || gophertype == GOPHTYP_GIF ||
      gophertype == GOPHTYP_IMAGE) {
    send_empty_document(sock);
  }
  else if (gophertype == GOPHTYP_MENU) {
    /* send this-
       2:
       0:nodeinfo of node we were working on
       1:nodeinfo of reserved node that says "menu not available"
       */
    cp = (char *) malloc (2*BUFSIZ);
    sprintf (cp, "2:\n0:%s\n", nodeinfostr(nodeid, NULL, flgsfmt));
    strcat (cp, "1:");
    strcat (cp, nodeinfostr(MENU_NOT_AVAILABLE, NULL, flgsfmt));
    strcat (cp, "\n.\r\n");
    sendbuf(cp, strlen(cp), sock);
    /* sendbuf will free cp */
  }
  else {
    send_empty_menu (sock);
  }
}

static int create_cached_nlist (FILE *ifile, char *tempname)
{
  FILE *tempfile;
  char line[BUFSIZ];
  char *gophfields[NUMFIELDS_GOPHER];
  int numf;
  struct s1 *node;
  int numlines;

  tempfile = fopen (tempname, "w");
  if (tempfile == NULL) {
    fprintf (stderr, "Unable to write temp output nlist file\n");
    perror (tempname);
    return 0;
  }

  /* ifile is an already open FILE */
  for (rewind (ifile), numlines = 0;
       fgets(line, sizeof(line)-1, ifile) != NULL; ) {
    if (*line != GOPHTYP_DUPSRV) {
      parsefields (line, gophfields, &numf, GOPHER_DLM, NUMFIELDS_GOPHER);
      if (numf >= NUMFIELDS_GOPHER) {
	node = getnode_bygopherinfo(gophfields);
	if (node == NULL) {  /* create a new node in tables */
	  get_nextid();
	  node = init_gopher_node (gophfields, next_id, todaysdate, 0);
	  inc_nextid();
	  addednewnodes++;
	  tables_changed++;
	}

	/*    gopher info is already known, therefore just use the existing nodeid */

	fprintf (tempfile, "%ld\n", node->nodeid);
	numlines++;
      }   /* enough gopher fields */
    }     /* it wasn't a dupl server line */
  }    /* foreach line of ifile */
  fclose (tempfile);
  return 1;
}





static int find_helper_conn (pid_t pid)
{
  int found;
  int con;
  
  for (found = 0, con = 0; con < FD_SETSIZE && !found;) {
    if (conn_established(con) && (conntab[con].c_hptr) &&
	(conntab[con].c_hptr)->pid == pid) {
      found = 1;
    }
    else
      con++;
  }
  if (!found)
    return -1;
  else
    return con;
}


extern void catch_child (void)
{
  pid_t pid;
  union wait status;
  struct rusage rusage;
  int c, found;
  FILE *childfile;
  WAITFORHELPER *hptr;
  char *filename2;
  sigset_t currmask;
  
  pid = wait3 (&status, WNOHANG, &rusage);
  
  if (pid == 0) {
    fprintf (stderr, "Funny, wait3 returned pid 0!\n");
    return;  /* don't know if there's a connect out there waiting */
  }
  else if (pid < 0) {
    fprintf (stderr, "wait3 returned %d\n", pid);
    perror ("wait3");
    return;   /* don't know if there's a connect out there waiting */
  }
  
  /* (pid > 0) */

  fprintf (stderr, "\nT=%d.  Caught pid %d, exitstat = %d.  ",
	   time(0), pid, WEXITSTATUS(status));

  if ((c=find_helper_conn (pid)) < 0) {
    if (pid != savedatastruct_pid)
      fprintf (stderr, "Unknown what connection %d went with\n", pid);
    else {
      savedatastruct_pid = 0;
      fprintf(stderr, "save_datastruct child.\n");
    }
    return;
  }
  else
    fprintf (stderr, "Goes with conn #%d, sock %d\n",  c, conntab[c].c_socket);


  hptr = conntab[c].c_hptr;
  /* Assumption: c_hptr->node still points to a valid node in a1[] */

  /* if child exited with non-zero status, must be an error */
  if (WEXITSTATUS(status) != 0) {
    child_punt((hptr->node)->gophertype, (hptr->node)->nodeid,
	       conntab[c].c_socket, conntab[c].c_output_fmt);
    unlink (hptr->file);
    return;
  }

  childfile = fopen (hptr->file,"r");
  if (childfile == NULL) {  /* File wasn't there ??? */
    fprintf (stderr, "Could not open child's output file\n");
    perror (hptr->file);
    child_punt ((hptr->node)->gophertype, (hptr->node)->nodeid,
		conntab[c].c_socket, conntab[c].c_output_fmt);
    return;
  }
  else {
    filename2 = tempnam(CACHEDIR, "convert");
    /* has to be the same dir as the cache because rename() needs that */
    
    switch ((hptr->node)->gophertype) {
    case GOPHTYP_IMAGE:
    case GOPHTYP_GIF: 
      /* No conversion is needed for GIF files */
      fclose (childfile);
      rename (hptr->file, hptr->cachefile);
      nio_send_file(conntab[c].c_socket, hptr->cachefile,
		    hptr->startpoint, hptr->requested, 0);
      remove_helper(hptr);
      do_free(filename2);
      break;
      
    case GOPHTYP_TEXT:
      conv_textfile (childfile, filename2); 
      rename (filename2, hptr->cachefile);
      nio_send_file(conntab[c].c_socket, hptr->cachefile,
		    hptr->startpoint, hptr->requested, 0);
      fclose (childfile); /* conv_... did NOT close file */
      unlink (hptr->file); /* remove child's file */
      remove_helper(hptr);
      do_free (filename2);
      break;
      
    case GOPHTYP_MENU:
    case GOPHTYP_SEARCH:
      if (!create_cached_nlist (childfile, filename2)) {
	child_punt((hptr->node)->gophertype,
		   (hptr->node)->nodeid,
		   conntab[c].c_socket, conntab[c].c_output_fmt);
      }
      else {
	rename (filename2, hptr->cachefile);
	send_cached_nlist (hptr->node, conntab[c].c_socket, hptr->cachefile,
			   conntab[c].c_output_fmt);
      }
      fclose (childfile);
      unlink (hptr->file);  /* remove child's file from the filesystem */
      remove_helper(hptr);  /* remove the buf associated with child */
      do_free(filename2);
      break;

    default:
      fprintf (stderr, "catch_child() doesn't handle type '%c'.",
	       (hptr->node)->gophertype);
      child_punt ((hptr->node)->gophertype, (hptr->node)->nodeid,
		  conntab[c].c_socket, conntab[c].c_output_fmt);
      fclose (childfile); /* close file */
      unlink (hptr->file); /* remove child's file */
      remove_helper(hptr);
      do_free (filename2);
      break;
    }
  } /* childfile was okay */
}
