/*Last Modified:  15-JAN-1992 08:44:19.59, By: MARK */
#include "vmsgopherd.h"

void
GSinit(gs)
  GopherStruct *gs;
{
     GSsetType(gs, '\0');

     gs->sPath[0] = '\0';
     gs->sTitle[0] = '\0';
     gs->sHost[0] = '\0';
     gs->iPort = 0;
     gs->iItemnum = 0;

}

void
GStoNet(gs, sockfd)
  GopherStruct *gs;
  int sockfd;
{
     static char buf[1024];

     buf[0] = GSgetType(gs);

     sprintf(buf + 1, "%s\t%s\t%s\t%d\r\n",
	     GSgetTitle(gs),
	     GSgetPath(gs),
	     GSgetHost(gs),
	     GSgetPort(gs));

     writestring(sockfd, buf);
     
     if (DEBUG)
	  fprintf(stderr, buf);

}

/** Copy a gopherstruct ***/

void
GScpy(dest, orig)
  GopherStruct *dest, *orig;
{
     dest->sFileType = orig->sFileType;
     dest->iPort     = orig->iPort;
     dest->iItemnum  = orig->iItemnum;

     strcpy(dest->sTitle, orig->sTitle);
     strcpy(dest->sPath,  orig->sPath);
     strcpy(dest->sHost,  orig->sHost);

}

/** Compare two GopherStructs ***/

int
GScmp(gs1, gs2)
  GopherStruct *gs1, *gs2;
{

     return(strcmp(GSgetTitle(gs1), GSgetTitle(gs2)));
}


/***********************************************************************
** Stuff for GopherDirObjs
**
***********************************************************************/

/** This proc adds a Gopherstruct to a gopherdir. **/

void
GDaddGS(gd, gs)
  GopherDirObj *gd;
  GopherStruct *gs;
{
     int x;
     int Top;

     Top = GDgetTop(gd);

     if (DEBUG)
	  fprintf(stderr, "Adding %s, Top=%d, Num=%d\n",
		  GSgetTitle(gs), Top, GSgetNum(gs));

     if ((x = GSgetNum(gs)) != 0) {

	  /** someone wants this to be the nth item. **/
	  if ((x-1) <= gd->Top) {

	       while (GSgetNum(GDgetEntry(gd, Top)) !=0)
		    Top++;

	       GScpy(GDgetEntry(gd, Top), GDgetEntry(gd, x-1));
	       GDsetTop(gd, ++Top);
	  }
	  
	  GScpy(GDgetEntry(gd, x-1), gs);
	  
     } else {
	  /*** First make sure a user-ordered object isn't there ***/

	  while (GSgetNum(GDgetEntry(gd, Top)) !=0)
	       Top++;

	  /*** Now tack it on the end ***/
	  GScpy(GDgetEntry(gd, Top), gs);
	  GDsetTop(gd, ++Top);
     }
}


void
GDsort(gd)
  GopherDirObj *gd;
{
     int i;

     /*** Find first non-numbered entry ***/

     for (i=0; ; i++) {
	  if (GSgetNum(GDgetEntry(gd, i)) == 0)
	       break;
     }

     /*** Everything up to i is already sorted by user-defined ordering ***/

     if (GDgetTop(gd) <= i)
	  /** No more sorting needed ***/
	  return;

     
     qsort((char *) GDgetEntry(gd, i), gd->Top-i, 
	   sizeof(GopherStruct),GScmp);

}


void
GDinit(gd)
  GopherDirObj *gd;
{
     int i;

     for (i=0; i<GDgetTop(gd); i++) {
	  GSsetType(GDgetEntry(gd, i), '\0');
     }

     GDsetTop(gd, 0);
}


GDtoNet(gd, sockfd)
  GopherDirObj *gd;
  int sockfd;
{
     int i;

     for (i=0; i< GDgetTop(gd); i++) {
	  GStoNet(GDgetEntry(gd, i), sockfd);
     }	  

     writestring(sockfd, ".\r\n");
}

