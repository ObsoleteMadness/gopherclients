/* cso.c
 *
 * Part of the Internet Gopher program, copyright (C) 1991
 * University of Minnesota Microcomputer Workstation and Networks Center
 *
 * Option to conduct simple queries of cso nameservers.
 */
 
#include "gopher.h"

/* Options are what gets displayed, sFields are the names used for queries */
char *Option[] = { "Name", "Phone", "E-Mail", "Address", 0};
char *sField[] = { "name", "phone", "email", "address", 0 };

void do_cso(ZeGopher)
  GopherStruct *ZeGopher;
{
     int i, iLen, iCount;           /* Index variables */
     int iSock, iFieldCount;        /* Socket number, number of fields */
     char sQuery[MAXSTR];           /* Build query string here */
     char sErrMsg[MAXSTR];          /* Error line on screen */
     char Response[MAXRESP][MAXSTR];/* Receive responses here */

     bzero(sQuery, MAXSTR);         /* initialize */
     bzero(sErrMsg, MAXSTR);
     for (i = 0; i < MAXRESP; bzero(Response[i++], MAXSTR));
     (ZeGopher->sPath)[0] = '\0';

     iLen = 1;
     iCount = 0;
     i = strlen(sField[0]);

     iFieldCount = 4; /* need to make a counter to count the fields in sField */
     
     while (iCount == 0) {
	  Get_Options(ZeGopher->sTitle, sErrMsg, iFieldCount, Option, Response);

	  for (i = 0;i < MAXRESP; i++)   /* count responses */
	       if (Response[i]) iCount++;

	  if (iCount == 0)               /* if none, do nothing */
	       return;
	  else
	       iCount = 0;               /* if some, reset for next check */

	  for(i = 0; i < 3; i++)         /* kludgy: lines 1-3 are mandatory*/
	       if (Response[i][0] != '\0') iCount++;
	  
	  if (iCount == 0) 
	       strcpy(sErrMsg, "Data for Lines 1, 2, or 3 REQUIRED");
	  else
	       bzero(sErrMsg, MAXSTR);
     }

     strcpy(sQuery, "query");

     for (i = 0; i < iFieldCount; i++)   /* If there's a response to */
	  if (Response[i][0] != '\0') {  /* process, add it to the query */
	       sprintf(sErrMsg, " %s=%s", sField[i], Response[i]);
	       strcat(sQuery, sErrMsg);
	  }

     strcat(sQuery, "\n"); 

     strcpy(ZeGopher->sPath, sQuery);

     showfile(ZeGopher, NULL);      /* Receive response as a file, exit */

     return;
}
