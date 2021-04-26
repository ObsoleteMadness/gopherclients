/*
 *------------------------------------------------------------------
 *
 * $Source: /afs/net.mit.edu/dapps/project/techinfodev/src/srv_ti/RCS/server.c,v $
 * $Revision: 1.6 $
 * $Date: 92/08/28 16:09:47 $
 * $State: Exp $
 * $Author: ark $
 * $Locker: ark $
 *
 * $Log:	server.c,v $
 * Revision 1.6  92/08/28  16:09:47  ark
 * Summer 1992 final version
 * 
 * Revision 1.5  92/08/12  14:18:26  ark
 * New version of 'o' transaction tells what output format to use;
 * this is a hack to allow old (but broken) clients to still work.
 * 
 * Revision 1.4  92/08/11  13:46:54  ark
 * Bug fixes: 'n' transaction leaving file descriptors open;
 *            'b' transaction now disallows searches on empty strings.
 * 
 * Revision 1.3  92/08/06  18:44:32  ark
 * Admin command "B" no longer crashes when there are many connections.
 * 
 * Revision 1.2  92/08/04  16:28:12  ark
 * Test production version 8/4/92
 * 
 * Revision 1.1  92/07/22  11:09:29  ark
 * Saber loads quietly; ANSI use standardized; command line options; no behavioral changes
 * 
 * Revision 1.0  92/07/10  12:34:00  ark
 * Initial revision
 * 
 * Revision 1.1  91/07/15  10:40:39  thorne
 * Initial revision
 * 
 *------------------------------------------------------------------
 */

#ifndef lint
#ifndef SABER
static char *rcsid_foo_c = "$Header: /afs/net.mit.edu/dapps/project/techinfodev/src/srv_ti/RCS/server.c,v 1.6 92/08/28 16:09:47 ark Exp Locker: ark $";
#endif
#endif  lint

/*
 * server.c 
 * S. Thorne
 * This is the entry point of the Techinfo server. It loads the web and gets
 * connections. It contains the main server loop.
 */
/*
  Copyright (C) 1989 by the Massachusetts Institute of Technology

   Export of this software from the United States of America is assumed
   to require a specific license from the United States Government.
   It is the responsibility of any person or organization contemplating
   export to obtain such a license before exporting.

WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
distribute this software and its documentation for any purpose and
without fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright notice and
this permission notice appear in supporting documentation, and that
the name of M.I.T. not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.  M.I.T. makes no representations about the suitability of
this software for any purpose.  It is provided "as is" without express
or implied warranty.

*/

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <signal.h>
#include <strings.h>
#include "network.h"
#include "inet.h"
/*#include "web.h" */
#include "pdb.h"  
/* #include <unistd.h>  Ulriix ??? */

CONN            conntab[FD_SETSIZE];

fd_set   read_fds;
fd_set   write_fds;
fd_set   except_fds;

#define	sock_readable(s)	FD_ISSET(s, &read_fds)

#define	conn_writeable(cn)	FD_ISSET(conntab[cn].c_socket,&write_fds)
#define	conn_except(cn)	        FD_ISSET(conntab[cn].c_socket,&except_fds)
#define	conn_readable(cn)	sock_readable(conntab[cn].c_socket)
#define	conn_established(cn)	(conntab[cn].c_socket != -1)
#define conn_wait_write(cn)      (conntab[cn].c_ptr) /* is the connection 
							waiting for data? */
#define conn_wait_recv(cn)      (conntab[cn].c_recptr) /* is the connection 
							sending a file over? */
#define	conn_timedout(cn)	(conntab[cn].c_flags & C_TIMEOUT) 
#define	conn_provider(cn)	(conntab[cn].c_flags & C_PROVIDER) 

#define	POLL	&tv
#define BLOCK	0

#define ADAY 86400  /* this should go in a .h file, but which? -- lam */

static int      num_connection;
static int      listen_sock;
int             PENDING_SHUTDOWN  = FALSE;
int             ALT_BANNER = FALSE;
char            *shutdown_msg;
char            *alt_banner;
short           todaysdate = 0;  /* will be properly initialized later */
static int      alarmwentoff;

extern int  datastruct_load(int unloadfirst);
extern void save_datastruct(void);
extern void remove_send(TOSEND *ptr);
extern void remove_rec(TOREC *ptr);
extern void remove_helper(WAITFORHELPER *ptr);
extern void log_trans(char *str);
extern void nio_rec_more(TOREC *ptr); 
extern void catch_child(void);
extern void catch_hup(void);

extern int      PROV; /* the socket # of the current provider */
char *msglist[100];
int	debug = 0; /* no debugging is default */


void catch_alarm(void)
{
  alarmwentoff = 1;
}



/* load the file of error messages. error file has structure of
   <error #>:<error text> */
int
load_msgs(void) 
{

	FILE           *fopen(), *msgfile;
        char      msgline[101];
        int pos = 0;
        int len;

	msgfile = fopen(MSG_FILE, "r");
	if (!msgfile) {
		perror(MSG_FILE);
		puts("Could not find the message file");
		exit(1);
	}
	while (fgets(msgline, 100, msgfile) != '\0') {
	  len = strlen(msgline);
	  msgline[len - 1] = '\0';
	  msglist[pos] = (char *) domalloc((unsigned int) len);
	  strcpy(msglist[pos],msgline);
	  pos++;
	}
	fclose(msgfile);
	return 0;		/* close file */

}

/* write the transaction to the log file */
void
log_session(char *line)		    
{
	int dfd;
	if ((dfd = open(LOG_FILE, O_APPEND | O_CREAT | O_WRONLY, 0644)) == 0)
		printf("error logging transaction\n");
	write(dfd, line, strlen(line));
	close(dfd);
	return;
}

void
dump_conntab(void)
{
  int i,fd;
  char line[200];
  
  fd = open("conntab",O_CREAT|O_TRUNC|O_WRONLY, 0644);
  for (i = 0; i < FD_SETSIZE; i++) {
    if (conn_established(i)){
      sprintf(line,"conn # %d, sock = %d, hostname = %s\n",i,conntab[i].c_socket,
	      conntab[i].c_hostname);
      write(fd,line,strlen(line));}
  }
  close(fd);
}

/* Close the connection on socket c, saving the web if it was connected to
   a provider.
   Note: Should there be a modified flag in the provider's socket info to
         determine whether or not we need to save the web? 
*/
void
close_connection(int c)
{
	time_t          secs;
	char       line[120];
	int rc;

	time(&secs);
	if (debug) {
	  printf("Closing connection to %s, port %d, socket #%d.\n",
		 conntab[c].c_hostname,
		 conntab[c].c_portnum,
		 conntab[c].c_socket);
	  fflush(stdout);
	}

	sprintf(line,"%d %d %s  %.24s, active %.1f minutes - SERVER\n",
		conntab[c].c_portnum,
		conntab[c].c_socket,
		conntab[c].c_hostname,
		ctime(&secs),
		(float) ((secs - conntab[c].c_made) /60.0));
	log_session(line);
	if (conntab[c].c_ptr != NULL)
	  remove_send(conntab[c].c_ptr);
	if (conntab[c].c_recptr != NULL)
	  remove_rec(conntab[c].c_recptr);
	if (conntab[c].c_hptr != NULL)
	  remove_helper(conntab[c].c_hptr); /* send KILL to child */

	do_free(conntab[c].c_hostname);
	do_free(conntab[c].c_uid);
	do_free(conntab[c].c_type);
	FD_CLR(conntab[c].c_socket, &read_fds); /* not needed now ? */
	FD_CLR(conntab[c].c_socket, &write_fds);
	FD_CLR(conntab[c].c_socket, &except_fds);
	rc = close(conntab[c].c_socket);
	if (rc < 0) {
	  sprintf(line,"Connection close failed on socket %d\n",conntab[c].c_socket);
	  printf(line); fflush(stdout);
	  log_session(line);
	}
	conntab[c].c_socket = -1;
	conntab[c].c_ptr = NULL;
	conntab[c].c_recptr = NULL;
	conntab[c].c_hptr = NULL;
	num_connection--;
}


/*
 * Find the connection using the given socket, and mark it as having
 * timed out, so the server can close it off next time around.
 */

void
sock_timeout(int sock)
{
	int	i;

	for (i = 0; i < FD_SETSIZE; i++) {
		if (conntab[i].c_socket == sock) {
			conntab[i].c_flags |= C_TIMEOUT;
			break;
		}
	}
	printf("Sock #%d marked for timeout termination.\n",
		conntab[i].c_socket);
	fflush(stdout);
}

void
server_shutdown()
{
	int             i;

	save_datastruct();    /* do this for production only? */
	log_trans("Shuting down the server");
	puts("\nServer shutdown, closing connections:");

	for (i = 0; i < FD_SETSIZE; i++)	/* Close Connections */
		if (conn_established(i))
			close_connection(i);

	close(listen_sock);  
	puts("Server has shut down -- Exiting.");
	exit(0);
}

int
hdl_connect(int c)
{
	CONN           *conn = &conntab[c];
	int             quit;

	if (debug) {
	  printf("\nHandling transaction to %s, port %d, socket #%d.\n",
		 conn->c_hostname,
		 conn->c_portnum,
		 conn->c_socket);
	  fflush(stdout);
	}

	quit = hdl_transact(conn);
	time(&conn->c_last);

	return quit;
}

void
make_new_connection(int listen_sock)
{
	CONN           *conn;
	char           *hostname,line[100],*cp;
	int            i;
	time_t    secs;
	time(&secs);
	for (i = 0; i < FD_SETSIZE; i++)
	  if (!conn_established(i))
	    break;
	if (i == FD_SETSIZE) {
	  printf("TechInfo server: reached connection limit of %d.\n",
		 FD_SETSIZE);
	  fflush(stdout);
	  return;
	}
	conn = &conntab[i];

	conn->c_socket = inet_accept_connection(listen_sock, &hostname,
						&conn->c_portnum);

	if (conn->c_socket == -1) {
		puts("accept_connection failed");
		return;
	}
	if (PENDING_SHUTDOWN) /* don't accept new connections */
	  {  /* send alternate message and close connection down */
	    if (shutdown_msg) 
	      {
		write(conn->c_socket, shutdown_msg, strlen(shutdown_msg));
		write(conn->c_socket, EOM, EOM_LEN);
	      }
	    else 
	      {
		write(conn->c_socket, NAK_BANNER, strlen(NAK_BANNER));
		write(conn->c_socket, EOM, EOM_LEN);
	      }
	    /* possibly sleep ?? */
	    close(conn->c_socket);
	    conn->c_socket = -1;
	    return;
	  }
	conn->c_hostname = (char *) domalloc((unsigned int) strlen(hostname) + 1);
	strcpy(conn->c_hostname, hostname);
	conn->c_uid = (char *) domalloc((unsigned int) 25);
	*conn->c_uid = '\0';
	conn->c_flags = 0;
	conn->c_type = NULL;
	conn->c_last = time(&conn->c_made);
	conn->c_ptr = NULL;
	conn->c_recptr = NULL;
	conn->c_hptr = NULL;
	conn->c_trans_cnt = 0;
	/* Added 8/12/92 ark -- Set initial output format to that defined in network.h
	   Change with the 'o' transaction */
	conn->c_output_fmt = DEFAULT_OUTPUT_FORMAT;

	num_connection++;
	if (debug) {
	  printf("Accepted new connection to %s, thru port %d, over socket #%d.\n",
	       conn->c_hostname,
	       conn->c_portnum,
	       conn->c_socket);
	  fflush(stdout);
	}
	if (ALT_BANNER) {
	  write(conn->c_socket, alt_banner, strlen(alt_banner));
	  write(conn->c_socket, EOM, EOM_LEN);
	}
	else {
	  write(conn->c_socket, BANNER, strlen(BANNER));
	  write(conn->c_socket, EOM, EOM_LEN);
	}

	if (fcntl(conn->c_socket, F_SETFL, FNDELAY) == -1)
		perror("fcntl");

	/*
	 * We should turn blocking on BEFORE write the banner!  Those
	 * writes shouldn't be direct syscalls either.
	 */
	if (conn->c_type == NULL)
	  cp = "unknown";
	else
	  cp = conn->c_type;
	sprintf(line,"%d %d %s %s %.24s - SERVER\n",
		conn->c_portnum,
		conn->c_socket,
		conn->c_hostname,
	        cp,
		ctime(&secs));
	log_session(line);
		
}

int             Standalone = 1;

static	int             i, handled;
static	time_t          secs;

int	ActiveJobs = 0;		/* Number of pending sends & recvs 
				 set int netio.c */

static	CONN		*conn;

extern int max_stale_time; /* in shutdown mode wait until all conn have been 
			      inactive for this many minutes */
void
server_loop(void)
{
  int		active_socks, show_connect = 0, do_shutdown;
  char line[200];
  struct timeval	tv;
  
  tv.tv_sec = 0;		/* For our select when it only polls */
  tv.tv_usec = 0;

  alarmwentoff = 1;
  signal (SIGALRM, catch_alarm);

  for (;;) {
    if (alarmwentoff) {
      alarmwentoff = 0;
      save_datastruct();
      alarm(60);   /* set alarm for 1 minute */
    }

    if (PENDING_SHUTDOWN)
      do_shutdown = TRUE;
    if (!ActiveJobs)
      show_connect++;
    if (show_connect && debug && num_connection) {
      printf("\nCurrent connections:");
      fflush(stdout);
    }
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    FD_SET(listen_sock, &read_fds);
    time(&secs);
    for (i = 0; i < FD_SETSIZE; i++) {
      if (!conn_established(i))
	continue;
      conn = &conntab[i];
      if (conn_wait_write(i))
	FD_SET(conn->c_socket, &write_fds); 
      else
	FD_SET(conn->c_socket, &read_fds);
      FD_SET(conn->c_socket, &except_fds);
      
      if (!show_connect)
	continue;
      if (debug) {
	printf("\n#%2d  %s.%d\tactive %.1f, last %.1f",
	       conn->c_socket,
	       conn->c_hostname,
	       conn->c_portnum,
	       (float) (secs - conn->c_made) / 60.0,
	       (float) (secs - conn->c_last) / 60.0);
	fflush(stdout);
      }
      if (conn->c_ptr != NULL | conn->c_recptr != NULL) {
	if (((secs - conn->c_last) / 60.0) > 30.0)
	  close_connection(i);   /* this conn asked for something 30 minutes
				    ago and still hasn't gotten it... */
      } 
      if (PENDING_SHUTDOWN)
	if  (((secs - conn->c_last) / 60.0) < max_stale_time)
	  do_shutdown = FALSE;
      if (((secs - conn->c_last) / 60.0) > 700.0)
	 close_connection(i);  /* close connections which
				  timed out */
    }
    
    if (PENDING_SHUTDOWN && do_shutdown)
      server_shutdown();
    if (show_connect && debug) {
      if (num_connection) {
	printf("\n%d connection%s currently established.\n",
	       num_connection, num_connection == 1 ? "" : "s");
	fflush(stdout);
      }
      /*      else
	      puts(" None"); */
    }
    if (debug) {
      if (ActiveJobs)
	printf("%s for activity...",
	       ActiveJobs ? "Checking" : "Waiting");
      if (ActiveJobs)
	printf("(%d asleep)...", ActiveJobs);
      fflush(stdout);
    }
    show_connect = 0;
    
    
    active_socks = select(FD_SETSIZE,&read_fds, &write_fds, 
			  &except_fds,
			  BLOCK);
    
    if (active_socks < 0) {
      if (errno != EINTR)
	perror("select");
      continue;
    }
    if (debug) {
      if (!active_socks && ActiveJobs) {
	printf("none...continuing our %d active processes...\n",
	       ActiveJobs);
	continue;
      }
      
      
      printf("%d socket%s reported with new activity.\n",
	     active_socks, active_socks == 1 ? "" : "s");
      fflush(stdout);
    }
    handled = 0;
    
    /*
     * First, see if we can establish any new connections. Takes
     * only first one off queue even if there are more. 
     */
    
    if (sock_readable(listen_sock))
      {
	make_new_connection(listen_sock);
	handled++;
	show_connect++;
      }
    
    /*
     * Now handle requests from all connections that have sent
     * them. 
     */

    todaysdate = time(0) / ADAY;  /* outside loop; used by other modules */

    for (i = 0; i < FD_SETSIZE; i++) {
      if (!conn_established(i))
	continue;
      if (conn_timedout(i)) {
	if (conn_readable(i))
	  handled++;
		close_connection(i), show_connect++;
	continue;
      }
      if (conn_except(i)) {
	printf("Exceptional condition on socket %d, closing connection.\n", i);
	fflush(stdout);
	handled++;
	close_connection(i);
	continue;
      }
      
      if (conn_wait_recv(i)){ /* if we're in the middle of
				 getting a file */
	nio_rec_more(conntab[i].c_recptr);
	handled++;
	continue;
      }
      else if (conn_writeable(i)) 
	{ /* finish writing */
	  handled++;
	  send_more(conntab[i].c_ptr);
	}
      else if (conn_readable(i))
	{
	  handled++;

	  if (hdl_connect(i) || conntab[i].c_trans_cnt > MAX_TRANS)
	    {
	    close_connection(i), show_connect++;
	    if (conntab[i].c_trans_cnt > MAX_TRANS) {
	      sprintf(line,"MAX TRANS REACHED... closing sock %d - %s\n",
		      conntab[i].c_socket,conntab[i].c_hostname);
	      log_session(line);
	    }
	  }
	  continue;
	}
    }
    
    /*
     * We have a very major problem if we thought we needed to do
     * something, and nothing was done, cause this can mean we're
     * headed into an infinte loop.  So we close everyone off if
     * this happens (although it never should...) 
     */
    
    if (!handled) {
      fprintf(stderr, "TechInfo server: Very major client plexing problem.\n");
      for (i = 0; i < FD_SETSIZE; i++)
	if (conn_established(i))
	  close_connection(i);
    }
  }
}

void
broken_pipe()
{
	puts("SIGPIPE - No one to read our write.");
	close_connection(i);
}

main(int argc, char **argv)
{

	extern int	menu;
	extern int	disptype;
	int pips_port = PIPS_PORT;

	char c;
	int errorflag = 0, temp_port = 0;
	extern int optind;
	extern char *optarg;

	
	while ((c = getopt(argc, argv, "dp:")) != EOF)
	  switch (c) {
	  case 'd':
	    debug++;
	    break;
	  case 'p':
	    if (optarg)	      
	      temp_port = atoi(optarg);
	    break;
	  default:
	    errorflag++;
	  }
	if (errorflag)
	  {
	    printf("Usage: %s [-p port] [-d]\n", argv[0]);
	    exit(1);
	  }

	if (temp_port)
	  if (temp_port > 1000)
	    pips_port = temp_port;
	  else 
	    {
	      puts("Must use a port number greater than 1000");
	      exit(1);
	    }
	

  	menu = NOMENU;
	disptype = SIMPLE;
	setuid(SERVER_UID); /* ti_serve */
	printf("Uid set to %d\n",getuid());
	if (access(PROV_FILE, F_OK)) {
		puts("Could not find the provider file");
		exit(1);
	}
	if (access(ADMIN_FILE, F_OK)) {
		puts("Could not find the admin file");
		exit(1);	
	      }
	if (access(NEXTID_FILE, F_OK)) {
		puts("Could not find the next_id file");
		exit(1);	
	      }
	if (access(VER_FILE, F_OK)) {
		puts("Could not find the version file");
		exit(1);	
	      }
	if (access(SOURCE_FILE, F_OK)) {
		puts("Could not find the source file");
		exit(1);	
	      }
	if (access(SERVER_FILE, F_OK)) {
		puts("Could not find the server file");
		exit(1);	
	      }
	for (i = 0; i < FD_SETSIZE; i++)
		conntab[i].c_socket = -1;	
	signal(SIGINT, server_shutdown);
	signal(SIGTERM, server_shutdown);
	signal(SIGCHLD, catch_child);
	signal(SIGHUP, catch_hup);
	SERVER = TRUE;
	printf("\n%s\n", BANNER); fflush(stdout);
	time(&secs);
	load_msgs(); /* load the message list from disk */
	printf("Started %.24s, pid = %d\n\n", ctime(&secs), getpid());
	fflush(stdout);

	listen_sock = inet_make_listner(pips_port);
	if (listen_sock < 0) {
		puts("Unable to accept connections!");
		exit(1);
	}
	if (!(int) datastruct_load(0)) {
		puts("Fatal error: datastruct_load failed");
		exit(1);
	}

	FD_ZERO(&read_fds);
	log_trans("Starting server");
	printf("TechInfo server ready.\n"); fflush(stdout);
	num_connection = 0;
	signal(SIGPIPE, broken_pipe);
	umask(022);

	fflush(stdout);
	server_loop();
}
