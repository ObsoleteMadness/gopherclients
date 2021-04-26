#include <stdio.h>
#include <errno.h>

/* Techinpher-Helper program

  Copyright 1993 by the University of Pennsylvania.

  Export of this software from the United States of America is assumed
  to require a specific license from the United States Government.  It
  is the responsibility of any person or organization contemplating
  export to obtain such a license before exporting.  Within that
  constraint, permission to use, distribute, or copy this software is
  granted, provided that this copyright notice appear on all copies.
  This software is provided as is, with no explicit nor implicit
  warranty.

*/

/* HISTORY:

Jan 1993: version 0.1 created --lam

*/

#define END_LINE           "\r\n"
#define HELPER_ERROR       "3: techinpher-helper error"
FILE *outfp;


errexit (int exitstat, char *msg)
{
  fprintf (outfp, "%s%s.%s", msg, END_LINE, END_LINE);
  fclose (outfp);
  exit (exitstat);
}

main (int argc, char *argv[])
{
  char *host, *port, *path, *outfile;
  int gophsock;
  char buf[32000];
  int cc;

  outfp = stdout;

  if (argc < 4) {
    fprintf (stderr, "Usage: %s GopherHost GopherPort \"GopherPath\" [Output File]%s",
	     argv[0], END_LINE);
    sprintf (buf, "%s (usage)", HELPER_ERROR);
    errexit (1, buf);
  }
  host = argv[1];
  port = argv[2];
  path = (char *) malloc (strlen(argv[3]) + 10);
  strcpy (path, argv[3]);
  strcat (path, "\r\n");
  outfile = argv[4];

  if (argc > 4) {
    outfp = fopen (outfile, "w");
    if (outfp == NULL) {
      outfp = stdout;
      fprintf (stderr, "Unable to write %s%s", outfile, END_LINE);
      perror (outfile);
      sprintf (buf,
	       "%s (can't write %s, errno %d)", HELPER_ERROR, outfile, errno);
      errexit (2, buf);
    }
  }

  gophsock = inet_establish_connection (host, port, 1);
  if (gophsock < 0) {
    fprintf (stderr,"Unable to connect to %s at %s%s", host,port,END_LINE);
    sprintf (buf, "3:Unable to connect to Gopher %s", host);
    errexit (3, buf);
  }
  else {
    /* Send path to gopher */
    if (write (gophsock, path, strlen(path)) != strlen(path)) { 
      sprintf (buf, "3:Unable to send path \"%s\" (errno %d)", argv[3], errno);
      errexit (4, buf);
    }
    
    while ((cc = read (gophsock, buf, sizeof(buf))) > 0) {
      write (fileno(outfp), buf, cc);
    }
  }
  exit (0);
}

