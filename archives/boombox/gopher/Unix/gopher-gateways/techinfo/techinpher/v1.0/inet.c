/*
 *------------------------------------------------------------------
 *
 * $Source: /afs/net.mit.edu/dapps/project/techinfodev/src/srv_ti/RCS/inet.c,v $
 * $Revision: 1.3 $
 * $Date: 92/08/28 16:09:10 $
 * $State: Exp $
 * $Author: ark $
 * $Locker: ark $
 *
 * $Log:	inet.c,v $
 * Revision 1.3  92/08/28  16:09:10  ark
 * Summer 1992 final version
 * 
 * Revision 1.2  92/08/04  16:26:38  ark
 * Test production version 8/4/92
 * 
 * Revision 1.1  92/07/22  11:09:01  ark
 * Saber loads quietly; ANSI use standardized; command line options; no behavioral changes
 * 
 * Revision 1.0  92/07/10  12:32:11  ark
 * Initial revision
 * 
 * Revision 1.1  91/07/15  10:40:15  thorne
 * Initial revision
 * 
 *------------------------------------------------------------------
 */

#ifndef lint
#ifndef SABER
static char *rcsid_foo_c = "$Header: /afs/net.mit.edu/dapps/project/techinfodev/src/srv_ti/RCS/inet.c,v 1.3 92/08/28 16:09:10 ark Exp Locker: ark $";
#endif SABER
#endif  lint
/*
 * inet.c:	Code for internet socket manipulations.
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <ctype.h>
#include <arpa/inet.h> /* for rt */

#include "inet.h"

/*
 * Create a socket and establish a connection over it to the given host and
 * port.  Returns the (connected) socket created, or -1 in case of error.
 */
static	struct sockaddr_in	lsname; /* the listener */
/* ARGSUSED */
int
inet_establish_connection(char *hostname, char *portname, int quiet)
{
	struct sockaddr_in	sname;
	struct hostent		*hp;
	struct servent		*sp;
	int	sock, portnum;

	/*
	 * resolve hostname
	 */

	hp = gethostbyname(hostname);
	if (hp == NULL) {
		fprintf(stderr, "unknown host %s\n", hostname);
		return -1;
	}

	/*
	 * resolve portname
	 */

	if (isdigit(*portname))
		portnum = htons(atoi(portname));/***/
	else {
		sp = getservbyname(portname, "tcp");
		if (sp == NULL) {
			fprintf(stderr, "unknown port %s\n", portname);
			return -1;
		}
		portnum = sp->s_port;
		portname = sp->s_name;
	}

	/*
	 * build server socket name
	 */

	bzero(&sname, sizeof(sname));
	sname.sin_family = AF_INET;
	bcopy(hp->h_addr, &sname.sin_addr, hp->h_length);
	sname.sin_port = portnum;

	/*
	 * create a network socket
	 */

	sock = socket(PF_INET, SOCK_STREAM, 0);

	if (sock < 0) {
		perror("socket");
		return -1;
	}

	/*
	 * make a connection to the specified server on the socket
	 */

	if (connect(sock, &sname, sizeof(sname)) < 0) {
	  /*
	    if (!quiet)
	      perror("connect in inet.c"); 
	  */
		return -1;
	}
      /*  if (fcntl(sock, F_SETFL, O_FSYNC) == -1)
		perror("fcntl");  test for write synch */
	/*
	if (!quiet)
		printf("Connected to %s, port %s (%d).\n",
		       hp->h_name, portname, ntohs(portnum)), fflush(stdout);
		       */
	return sock;
}

/*
 * Create a socket bound to the given local port, and assign it
 * a listening condition.
 */

int
inet_make_listner(int port)
{

	int			sock, length, i,rc;

	sock = socket(PF_INET, SOCK_STREAM, 0);

	if (sock < 0) {
		perror("creating socket");
		return -1;
	}

	/*
	 * build name for local socket 
	 */

	bzero(&lsname, sizeof(lsname));
	lsname.sin_family = AF_INET;
	lsname.sin_addr.s_addr = INADDR_ANY;
	lsname.sin_port = ntohs(port);

	/*
	 * set name of local socket - ie. assign &sname to the socket name
	 * space 
	 */
	i = 1; /* the ultrix code needs this for REUSEADDR to work */

        rc = setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&i,sizeof(i)); /* */
	if (rc == -1)
	  perror("setting sockopt");
	if (bind(sock, &lsname, sizeof(lsname)) < 0) {
		perror("binding socket");
		return -1;
	}

	/*
	 * check the name assignment; a zero is returnd if call succeeds.
	 * &sname is altered.  &length indicates the amount of space pointed
	 * by name 
	 */

	length = sizeof(lsname);
	if (getsockname(sock, &lsname, &length)) {
		perror("getting socket name");
		return -1;
	}

	listen(sock, 5);
/*
	printf("Listening at local address %s, port %d, over socket #%d.\n\n",
		inet_ntoa(lsname.sin_addr), ntohs(lsname.sin_port),
		sock); 
*/
/*****/
	return sock;
}

/*
 * Accept a connection coming in over the given listen_sock (a socket
 * that is being listend on, perhaps created by make_listner()).
 * Return a new connected socket, and the contacting hostname and
 * portnumber if desired and available.
 */

int
inet_accept_connection(int listen_sock, char **hname, int *portnum)
{
	struct sockaddr_in	sname;
	struct hostent		*host;
	int			socket, length;
	
	socket = accept(listen_sock, 0, 0);
	if (socket < 0) {
		perror("accept");
		return -1;
	}

	/*
	 * get name of other end of this connection; returns zero if call
	 * successful.  &sname is altered by call, &length is space pointed
	 * to by name 
	 */

	length = sizeof(sname);
	if (getpeername(socket, &sname, &length))
		perror("getpeername");

	if (portnum)
		*portnum = ntohs(sname.sin_port);

	/*
	 * return a pointer to the structure hostent, which contains host
	 * information ie. name of host, host address type (AF_INET), length
	 * of address	
	 */

	if (hname) {
		host = gethostbyaddr(&sname.sin_addr, sizeof(sname.sin_addr), AF_INET);
		if (!host) {
			perror("gethostbyaddr");
 			*hname = inet_ntoa(sname.sin_addr);
			/* *hname = "<unknown>"; */
		} else
			*hname = host->h_name;
	}

	return socket;
}




