/*Last Modified:   7-APR-1992 14:35:20.44, By: MARK */
/* util.c
 *
 * Part of the Internet Gopher program, copyright (C) 1991
 * University of Minnesota Microcomputer Workstation and Networks Center
 *
 */

#include "gopher.h"


/* Read "n" bytes from a descriptor.
 * Use in place of read() when fd is a stream socket
 *
 * Returns the number of total bytes read.
 */

int readn(fd, ptr, nbytes)
int fd;
char *ptr;
int nbytes;
{
	int nleft, nread;

	nleft = nbytes;
	while (nleft > 0) {

#ifdef WOLLONGONG
		nread = netread(fd, ptr, nleft);
#endif /* Wollongong */
#ifdef MULTINET
		nread = socket_read(fd, ptr, nleft);
#endif /* MULTINET */
#ifdef UCX
		nread = read(fd, ptr, nleft);
#endif /* UCX */

		if (nread < 0)
			return(nread);	/* error, return <0 */
		else if (nread == 0)	/* EOF */
			break;
	
		nleft 	-= nread;
		ptr 	+= nread;
	}
	return(nbytes - nleft);	/* return >= 0) */
}



/*
 * Write "n" bytes to a descriptor.
 * Use in place of write() when fd is a stream socket
 *
 * We return the number of bytes written
 */

int writen(fd, ptr, nbytes)
int	fd;
char	*ptr;
int	nbytes;
{
	int nleft, nwritten;

	nleft = nbytes;
	while(nleft > 0) {

#ifdef WOLLONGONG
		nwritten = netwrite(fd, ptr, nleft);
#endif /* Wollongong */
#ifdef MULTINET
		nwritten = socket_write(fd, ptr, nleft);
#endif /* Multinet */
#ifdef UCX
		nwritten = write(fd, ptr, nleft);
#endif /* UCX */

		if (nwritten <= 0)
			return(nwritten);	/* error */

		nleft	-= nwritten;
		ptr	+= nwritten;
	}
	return(nbytes - nleft);
}


/*
 * Writestring uses the writen and strlen calls to write a
 * string to the file descriptor fd.  If the write fails
 * a -1 is returned. Otherwise zero is returned.
 */

int writestring(fd, stringptr)
int	fd;
char	*stringptr;
{
	int length;

	length = strlen(stringptr);
	if (writen(fd, stringptr, length) != length)
		return(-1);
	else
		return(0);
}


/*
 * Read a line from a descriptor.  Read the line one byte at a time,
 * looking for the newline.  We store the newline in the buffer,
 * then follow it with a null (the same as fgets(3)).
 * We return the number of characters up to, but not including,
 * the null (the same as strlen(3))
 */

int readline(fd, ptr, maxlen)
int	fd;
char	*ptr;
int 	maxlen;
{
	int n;
	int rc;
	char c;

	for (n=1; n < maxlen; n++) {
#ifdef WOLLONGONG
   		if ( (rc = netread(fd, &c, 1)) == 1) {
#endif /* Wollongong */
#ifdef MULTINET
   		if ( (rc = socket_read(fd, &c, 1)) == 1) {
#endif /* Multinet */
#ifdef UCX
   		if ( (rc = read(fd, &c, 1)) == 1) {
#endif /* UCX */
			*ptr++ = c;
			if (c == '\n')
				break;
		}
		else if (rc == 0) {
			if (n == 1)
				return(0);	/* EOF, no data read */
			else
				break;		/* EOF, some data was read */
		}
		else
			return(-1);		/* error */
	}

	*ptr = 0; 				/* Tack a NULL on the end */
	return(n);
}

/*
 * Readfield reads data up to a tab, (like readline above)
 */

int readfield(fd, ptr, maxlen)
int	fd;
char	*ptr;
int 	maxlen;
{
	int n;
	int rc;
	char c;

	for (n=1; n < maxlen; n++) {
#ifdef WOLLONGONG
   		if ( (rc = netread(fd, &c, 1)) == 1) {
#endif /* Wollongong */
#ifdef MULTINET
   		if ( (rc = socket_read(fd, &c, 1)) == 1) {
#endif /* Multinet */
#ifdef UCX
   		if ( (rc = read(fd, &c, 1)) == 1) {
#endif /* UCX */
			*ptr++ = c;
			if (c == '\t') {
			     *(ptr - 1) = '\0';
				break;
			}
		}
		else if (rc == 0) {
			if (n == 1)
				return(0);	/* EOF, no data read */
			else
				break;		/* EOF, some data was read */
		}
		else
			return(-1);		/* error */
	}

	*ptr = 0; 				/* Tack a NULL on the end */
	return(n);
}


int
sreadword(input, output, maxlen)
  char *input;
  char *output;
  int maxlen;
{
     int n;
     char c;
     
     for (n=0; n < maxlen; n++) {
	  c = *input++;
	  *output++ = c;
	  if (isspace(c)) {
	       *(output - 1) = '\0';
	       break;
	  }
	  
	  if (c == NULL) {
	       break;
	  }
     }
     
     *output = '\0'; 				/* Tack a NULL on the end */
     return(n);
}
