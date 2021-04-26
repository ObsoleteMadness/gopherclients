/*
 *------------------------------------------------------------------
 *
 * $Source: /afs/net.mit.edu/dapps/project/techinfodev/src/srv_ti/RCS/netio.c,v $
 * $Revision: 1.5 $
 * $Date: 92/08/28 16:09:26 $
 * $State: Exp $
 * $Author: ark $
 * $Locker: ark $
 *
 * $Log:	netio.c,v $
 * Revision 1.5  92/08/28  16:09:26  ark
 * Summer 1992 final version
 * 
 * Revision 1.4  92/08/27  17:11:11  ark
 * Retropatched error message file to allow old clients to work
 * 
 * Revision 1.3  92/08/06  18:43:47  ark
 * Admin command "B" no longer crashes when there are many connections.
 * 
 * Revision 1.2  92/08/04  16:27:31  ark
 * Test production version 8/4/92
 * 
 * Revision 1.1  92/07/22  11:09:18  ark
 * Saber loads quietly; ANSI use standardized; command line options; no behavioral changes
 * 
 * Revision 1.0  92/07/10  12:32:28  ark
 * Initial revision
 * 
 * Revision 1.1  91/07/15  10:40:20  thorne
 * Initial revision
 * 
 *------------------------------------------------------------------
 */

#ifndef lint
#ifndef SABER
static char *rcsid_foo_c = "$Header: /afs/net.mit.edu/dapps/project/techinfodev/src/srv_ti/RCS/netio.c,v 1.5 92/08/28 16:09:26 ark Exp Locker: ark $";
#endif SABER
#endif  lint
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
/* These routines handle most of the non-blocking IO for the server. This does
   not use the LWP package. Its pupose is to allow the server to go ahead and
   do something else if a connection blocks. 
   S. Thorne 3/28/91 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/file.h>
#include "pdb.h"
#include "network.h"
#include "messages.h"

TOSEND *firstrec = NULL;
TOREC  *first = NULL;

extern CONN conntab[];

extern int ActiveJobs;



char	_sendbuf[BUFSIZ];
char	*_sendbuf_pos = _sendbuf;
extern char *msglist[]; 
extern void log_trans(char *str);
extern int  is_directory(int fd);
extern int cur_sock;
extern int debug;

int dosend(char *str, char *buf, char **curpos);
int dodeliver(char *buf, char **curpos, int sock);
void nio_send_file(int sock, char *filename, long startpoint, long requested);
void add_send(char *start, char *ptr, int size, int sock);
void remove_send(TOSEND *ptr);
void add_recv(int fd, char *file_name, int sock);
void remove_rec(TOREC *ptr);
int sendbuf(char *buf, int size, int sock);
int send_more(TOSEND *ptr);
void nio_rec_more(TOREC *ptr); 
void send_msg(char *str, int sock);
void close_recfile(TOREC *ptr);


/* macros and all that junk were too damn kludgy for me --lam */

void send_msg (char *str, int sock)
{
  char *ptr;
#define NOMSG "99:There is no error msg text assigned here"

  if (!str) {
    ptr = domalloc (sizeof(NOMSG) + EOLM_LEN);
    strcpy (ptr, NOMSG);
  }
  else {
    ptr = domalloc (strlen(str)+ 1 + EOLM_LEN);
    strcpy (ptr, str);
  }
  strcat (ptr, EOLM);
  sendbuf (ptr, strlen(ptr), sock);
}

void
nio_send_file(int sock, char *filename, long startpoint, long requested)
{
	long            fd, red,len, should_send;
	char *iobuf, *cp;
	struct stat	st;

	if ((fd = open(filename, O_RDONLY, 0644)) < 0) {
	  /* 
	   * This error message has no code because of the format of the
	   * header returned below; in order to be backward compatible with
	   * old clients, atoi() on this error message must return 0.
	   */
	  send_msg(msglist[ SENDFILE_CANT_ACCESS ], sock);
	  return;
	}
	fstat(fd, &st);
	if (st.st_mode & S_IFDIR)
	  {
	    send_msg(msglist[ SENDFILE_IS_DIRECTORY ], sock);
	    close(fd);
	    return;
	  }

	lseek(fd, startpoint, L_SET);
	
	should_send = min(st.st_size - startpoint + EOLM_LEN,requested + EOLM_LEN);
	if (should_send < 0)
	  should_send = EOLM_LEN;
	iobuf =  domalloc((unsigned int) should_send + 200);
	bzero(iobuf,should_send +200);
	sprintf(iobuf,"%ld Total Characters :%ld sent:This document was last modified on %s\n",
		st.st_size, should_send, ctime(&st.st_mtime));
	len = strlen(iobuf);
	cp = index(iobuf,'\0');
	red = read(fd,cp,should_send - EOLM_LEN);
	if (red < 0)
	  {
	    send_msg(msglist[ SENDFILE_CANT_ACCESS ], sock);
	    close(fd);
	    return;
	  }
	cp = cp + red;
	bcopy(EOLM,cp,EOLM_LEN);
	sendbuf(iobuf,len + red + EOLM_LEN, sock);
	close(fd);
	return;

}

/* If a write would have blocked, save it away so that we can send the 
   remaining text later */
void
add_send(char *start, char *ptr, int size, int sock) 
{
  TOSEND *rec;
  int	i;
  
  if (debug)
    printf("Starting blocked send to socket #%d.\n", sock);
  ActiveJobs++;
  rec = (TOSEND *) domalloc((unsigned int) sizeof(TOSEND));
  rec->sock = sock;
  rec->ptr = ptr;
  rec->buf = start;
  rec->size = size;
  
  for (i = 0; i < FD_SETSIZE; i++) { 
    if (conntab[i].c_socket == sock) {
      conntab[i].c_ptr = rec;
      break;
    }
  }
  return;
}

/* remove a pending send */
void
remove_send(TOSEND *ptr) 
{
  int	i;

  ActiveJobs--;
  for (i = 0; i < FD_SETSIZE; i++) {
    if (conntab[i].c_ptr == ptr) {
      if (debug)
	printf("Finished blocked send to socket #%d.\n", conntab[i].c_socket);
      break;
    }
  }
  if (ptr->buf != _sendbuf)
    do_free(ptr->buf); /* do_free the text buffer */
  do_free(ptr);
  conntab[i].c_ptr = NULL;

  return;
}

/* add a pending receive */
void
add_recv(int fd, char *file_name, int sock) 
{
  TOREC *rec;
  int	i;
  
  ActiveJobs++;
  rec = (TOREC *) domalloc((unsigned int) sizeof(TOREC));
  rec->sock = sock;
  rec->the_fd = fd;
  rec->filename = file_name;
  for (i = 0; i < FD_SETSIZE; i++) { 
    if (conntab[i].c_socket == sock) {
      conntab[i].c_recptr = rec;
      break;
    }
  }
  return;
}

void
remove_rec(TOREC *ptr) /* remove a pending receive from the list */
{
  int	i;
  
  ActiveJobs--;
  for (i = 0; i < FD_SETSIZE; i++) 
    if (conntab[i].c_recptr == ptr)
      break;

  do_free(ptr->filename);
  do_free(ptr);

  conntab[i].c_recptr = NULL;
  return;
}

int
sendbuf(char *buf, int size, int sock)
{
  int len = BUFSIZ,rc;
  char *startbuf;
  
  startbuf = buf;
  
  while (size)
    {
      if (size <= BUFSIZ)
	len = size;

      rc = write(sock,buf,len);
      if (rc < 0)
	{
	  if (errno == EWOULDBLOCK) /* would block, so save info, and leave it 
				       will be done later */
	    {
	      if (debug)
		printf("Write blocked on socket #%d.\n", sock);
	      add_send(startbuf,buf,size,sock);
	      return 1;
	    }
	  if (startbuf != _sendbuf)
	    do_free(startbuf);
	  return -1;
	}
      size = size - rc;
      buf = buf + rc;
    }
  if (startbuf != _sendbuf)
    do_free(startbuf);
  return 0;
}

int
send_more(TOSEND *ptr)
{
  int size,len = BUFSIZ,rc,sock;
  char *buf;
  
  if (ptr == NULL) { 
    return 0;
  }

  size = ptr->size;
  buf = ptr->ptr;
  sock = ptr->sock;
  if (debug)
    printf("Continuing send to socket #%d.\n",sock);
  while (size)
    {      
      if (size <= BUFSIZ)
	len = size;
      
      rc = write(sock,buf,len);
      if (rc <= 0){
	ptr->size = size;	  /* update the TOSEND record */
	ptr->ptr = buf;
	if (errno == EWOULDBLOCK) /* would block, so save info, and leave it 
				     will be done later */
	  {
	    if (debug)
	      printf("Write blocked again on socket #%d.\n", sock);
	    return 1;
	  }
	return -1;
      }
      size = size - rc;
      buf = buf + rc;
    }
  remove_send(ptr);
  return 0;    /* Added for consistency 7/15/92 ark */
}

/* does the work of receiving a file */
void
nio_rec_more(TOREC *ptr) 
{
  
  int  red, done;
  char in_buff[BUFSIZ];
  
  errno = 0;
  done = FALSE;
  for (;;) {
    red = read(ptr->sock, in_buff, BUFSIZ);
    if (debug)
      printf("Received %d bytes from socket #%d.\n", red, ptr->sock);
    while (red > 0) 
      {
	if (!strncmp(&in_buff[red - EOM_LEN], EOM, EOM_LEN)) 
	  {
	    write(ptr->the_fd, in_buff, red - EOM_LEN);
	    done = TRUE;
	    break;
	  } 
	else
	  write(ptr->the_fd, in_buff, red);
	red = read(ptr->sock, in_buff, BUFSIZ);
      }		/* end of while */
    
    if (done) {
      close_recfile(ptr);
      remove_rec(ptr);
      break;
    }
    else  /*  EWOULDBLOCK  */
      return; /* no settings to update, so just leave  */
  } /* end of forever */
}

/* Try to back up destination file with a ~ on the end of its
   filename.  Then move our temporary file, ending with .tmp, to the
   destination file. */
void
close_recfile(TOREC *ptr)
{
  char tempfile[MAXPATHLEN],backup_fname[MAXPATHLEN],in_buff[BUFSIZ];
  int rc;
  
  sprintf(tempfile,"%s.tmp",ptr->filename);
  sprintf(backup_fname,"%s~",ptr->filename);
  if (close(ptr->the_fd))
    printf("Couldn't close file descriptor %d on receipt of file!\n", 
	   ptr->the_fd);

  /* We really shouldn't try to do this rename if the file doesn't exist! */
  rc = rename(ptr->filename, backup_fname);
  if (rc == 0)
    if (debug)
      printf("Backed up existing file %s to %s.\n", 
	     ptr->filename, backup_fname);

  rc = rename(tempfile,ptr->filename);
  if (rc)
    printf("Temporary file %s couldn't be renamed!\n", tempfile);
  else 
    printf("Wrote file %s.\n", ptr->filename);

  chmod(ptr->filename,0644);

  /* Tell client if final rename worked */
  if (rc)
    sprintf(in_buff, "%s\n%s", msglist[ CANT_OPEN ], EOM);
  else
    sprintf(in_buff, "%s\n%s", msglist[ OK ], EOM);
  write(ptr->sock, in_buff,strlen(in_buff));
}
