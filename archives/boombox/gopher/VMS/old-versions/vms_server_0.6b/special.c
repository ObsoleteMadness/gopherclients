/*Last Modified:   6-MAR-1992 15:10:12.45, By: MARK */
/*
 * This file contains routines to deal with special types of files.
 * including compressed text and shell scripts.
 */


#include "vmsgopherd.h"


/* Check to see if this file needs special treatment before heading
 * back to the client... We will check for:
 *	Compressed files	if so, zcat first
 *	Shellscript		if so, "do it"
 * Note: it would be somewhat non-portable to check of a binary
 *  (we'd need to check for so many different magic numbers; the
 *  shell script designation should be sufficient, since the script
 *  can call an executable anyway
 * Recognized elsewhere:
 *	.snd			needs special processing on client
 *	uuencoded		needs special processing on client
 * Other filetypes we could check for:
 *	GIF		->	Bring up GIF previewer
 *	Postscript	->	Bring up Postscript previewer
 */

static int ispipe;

FILE *
specialfile(fp, pathname)
  FILE *fp;
  char *pathname;
{
     FILE *pp;
     char buf[256], s[256], outfile[256];
     long i;
     unsigned int pid;
     int ret_status, iosb[2], itmlst;

     $DESCRIPTOR( d_input, buf );
     $DESCRIPTOR( d_output, outfile );
     $DESCRIPTOR( d_error, "umm$manager:gopher.error" );
     $DESCRIPTOR( d_image, "sys$system:loginout.exe" );

     itmlst = 0;
     ispipe = 0;
     strncpy( buf, "\0", 256 );
     strncpy( s, "\0", 256 );
     strncpy( outfile, "\0", 256 );

     /* Keep track of where we are */
     i = ftell(fp);
     rewind(fp);
     
     /* Grab the first three bytes, and rewind */
     if (fgets(s, 255, fp) == NULL)
	  return (FILE *)0;

     fseek(fp, i, 0);
     
     /* Compressed? */
/*     if (iscompressed(s)) {
	  sprintf(buf, "/bin/zcat \"%s\"\n", pathname);
	  if (! (pp = popen(buf, "r")))
	       return (FILE *)0;
	  ispipe = 1;
	  return pp;
     }     */

     /* Script? */
     if (isshellscript(s)) {
	  s[strlen(s)-1] = '\0';
	  strcpy( buf, pathname );
	  if (DEBUG)
	       fprintf(stderr, "Executing %s", buf);

	  sprintf(outfile, "gopher_root:[000000]comfile.%d",(clock()/CLK_TCK));
	  d_output.dsc$w_length = strlen( outfile );
	  d_input.dsc$w_length = strlen( buf );
	  ret_status = sys$creprc(&pid,&d_image,&d_input,&d_output,&d_error,
		NULL, NULL, NULL, 5, NULL, NULL, NULL );
	  ret_status = sys$getjpiw(0,&pid,0,&itmlst,&iosb,0,0);
	  sleep(1);
	  while( ret_status != SS$_NONEXPR ) {
	     ret_status = sys$getjpiw(0,&pid,0,&itmlst,&iosb,0,0);
	     sleep(1);
	  }
	  if (! (pp = fopen( outfile, "r" )))
	    return (FILE *)0;
	  ispipe = 1;
	  strcpy( pathname, outfile );	  
	  if (DEBUG)
	       fprintf(stderr, "Zcat popen is okay\n");
	  
	  return pp;
     }

     return (FILE *)0;
}


iscompressed(s)
  char *s;
{
     if (s[0] == '\037' && s[1] == '\235')
	  return 1;
     else
	  return 0;
}


isshellscript(s)
  char *s;
{
     if (! strncmp(s, "$!", 2))
	  return 1;
     else
	  return 0;
}


int
Specialclose(fp)
  FILE *fp;
{
     if (ispipe)
	  return(fclose(fp));
     else
	  return(fclose(fp));
}
