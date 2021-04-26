/*
 *------------------------------------------------------------------
 * $Source: /afs/net.mit.edu/dapps/project/techinfodev/src/srv_ti/RCS/inet.h,v $
 * $Revision: 1.1 $
 * $Date: 92/08/04 16:27:02 $
 * $Author: ark $
 *------------------------------------------------------------------
 */
/*
 * inet.h
 */

int	inet_establish_connection(char *hostname, char *portname, int quiet);
int	inet_make_listner(int port);
int	inet_accept_connection(int listen_sock, char **hname, int *portnum);

