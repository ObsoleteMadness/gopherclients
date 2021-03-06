/*Last Modified:   9-MAR-1992 14:51:25.06, By: MARK */
/* manager.c
 *
 * Part of the Internet Gopher program, copyright (C) 1991
 * University of Minnesota Microcomputer Workstation and Networks Center
 */

#include "gopher.h"


#define MENULINE(x)   (x)+3

/* If any gophers to display (screen can be blank), count the number
   of pages.  If there is a remainder greater than zero, add one page */
#define PAGECALC(x,y) (y) ? (x/y) + ((x%y)>0) : 1

#define MIN(x,y)      ((x) <= (y) ? (x) : (y))


/*
** Draw the title on the top
*/

Draw_Banner()
{

     int ret_status, row, col;
     int rend = SMG$M_REVERSE;
     int rend_comp = 0;

     $DESCRIPTOR( d_vers, VERSIONline );
     d_vers.dsc$w_length = strlen( VERSIONline );
     row = 1;
     col = ( 80 - strlen( VERSIONline ) ) / 2;

     ret_status = smg$put_chars( &DisplayId, &d_vers, &row, &col, &rend_comp,
                    &rend );

}     


/*
** Draw the status line
*/

Draw_Status(textline)
  char *textline;
{
     int ret_status, stat_line, stat_col;
     $DESCRIPTOR( d_qhelp, "Press '?' for Help" );
     $DESCRIPTOR( d_text, textline );

     d_text.dsc$w_length = strlen( textline );
     stat_line = 24;
     stat_col = 1;

     ret_status = smg$erase_line( &DisplayId, &stat_line, &stat_col );
     ret_status = smg$put_chars(&DisplayId,&d_qhelp,&stat_line,&stat_col);
     stat_col = 80 - strlen(textline);
     ret_status = smg$put_chars(&DisplayId,&d_text,&stat_line,&stat_col);
}


/*
** Man is this ugly.
*/

Display_Dir_Page(gophers, iNewLine, nNewPage, nMaxPages, iPageLen, iLastPageLen)
  GopherStruct *gophers;
  int nNewPage, nMaxPages, iPageLen, iLastPageLen;
{
     int i, iLoop, iOffset, ret_status, mline, mcol, d_str_len, offlen;
     int outlen;
     char num_string[10], out_string[255];

     $DESCRIPTOR( d_str, num_string );
     $DESCRIPTOR( d_space, " " );
     $DESCRIPTOR( d_outstring, out_string );
     $DESCRIPTOR( d_slash, "/" );
     $DESCRIPTOR( d_cso, " <CSO>" );
     $DESCRIPTOR( d_tel, " <TEL>" );
     $DESCRIPTOR( d_question, "> <?<" );
     $DESCRIPTOR( d_speaker, " <)" );
     $DESCRIPTOR( d_period, "." );
     $DESCRIPTOR( d_pound, "#" );

     strncpy( out_string, "\0", 255 );

     /*** Clear the screen and redraw the top line **/

     ret_status = smg$erase_display( &DisplayId );
     Draw_Banner();

     /** Draw the menu **/
     iLoop = (nNewPage == nMaxPages) && iLastPageLen ? iLastPageLen : iPageLen;

     for (i= 0, iOffset = (nNewPage-1) * iPageLen; i <iLoop; i++, iOffset++) {

          sprintf(num_string, "%d.\0",iOffset+1);
          d_str.dsc$w_length = strlen( num_string );
          mline = MENULINE( i+1 );
          mcol = 10;
	  ret_status = smg$put_chars( &DisplayId, &d_str, &mline, &mcol );
	  if (iOffset + 1 < 10)
	     ret_status = smg$put_chars( &DisplayId, &d_space);

     {
	  char *c, *d = gophers[iOffset].sTitle;

	  c=d;

	  while (strlen(c) > COLS - 16) {
	       c = strchr(d, '/');
	       if (c == NULL) {
		    c = d;
		    break;    /*** No more slashes ***/
	       }
	       d = c+1;
	  }

	  if (strlen(c) > COLS-16)
	       c[COLS-17] = '\0';    /*** Gack, strip off extra cruft **/

          strncpy( out_string, "\0", 255 );

	  if (c != d) {
             sprintf( out_string, " ..%s", c+1 );
	     ret_status = smg$put_chars( &DisplayId, &d_outstring );
          }
	  else {
             sprintf( out_string, " %s", c );
	     ret_status = smg$put_chars( &DisplayId, &d_outstring );
          }
     }
	  switch(gophers[iOffset].sFileType)
	     {
	     case A_DIRECTORY:
	       ret_status = smg$put_chars( &DisplayId, &d_slash );
	       break;
             case A_CSO:
	       ret_status = smg$put_chars( &DisplayId, &d_cso );
	       break;
	     case A_TELNET:
	       ret_status = smg$put_chars( &DisplayId, &d_tel );
	       break;
	     case A_INDEX:
	       ret_status = smg$put_chars( &DisplayId, &d_question );
	       break;
             case A_SOUND:
	       ret_status = smg$put_chars( &DisplayId, &d_speaker );
	               /** It's supposed to look like a speaker! **/
	       break;
	     case A_FILE:
	       ret_status = smg$put_chars( &DisplayId, &d_period );
	       break;
	     case A_EVENT:
	       ret_status = smg$put_chars( &DisplayId, &d_pound );
	       break;

             }
     }
}


/* scline - Screen line relocator.
 *          Returns the line resulting from choice */
int 
scline(iMaxGophers, iOldGopher, iNewGopher, gophers, MenuTitle)
  int iMaxGophers;    /* Total number of gophers on all pages */
  int iOldGopher;     /* Which gopher previously displayed */
  int iNewGopher;     /* New gopher to be displayed */
  GopherStruct *gophers;
  char *MenuTitle;
{
     int iPageLen, iLastPageLen;        /* Length of normal, final pages */
     int nMaxPages, nNewPage, nOldPage; /* Natural numbers */
     int iOldLine, iNewLine;            /* Screen locations */
     int ret_status, row, col, rend;
     char sPagenum[40];

     $DESCRIPTOR( d_3spaces, "   " );
     $DESCRIPTOR( d_arrow, "-->" );
     $DESCRIPTOR( d_menutitle, MenuTitle );
     d_menutitle.dsc$w_length = strlen( MenuTitle );
     
     if ((iNewGopher < 0) || (iNewGopher > iMaxGophers))
	  return(iOldGopher);
     
     iPageLen = LINES-6;    /* Number of menu lines possible per page */
     
     nMaxPages = PAGECALC(iMaxGophers, iPageLen);    /* Total number of pages */
     nOldPage =  PAGECALC(iOldGopher, iPageLen); 
     nNewPage =  PAGECALC(iNewGopher, iPageLen);
     
     if ((nNewPage < 1) || (nNewPage > nMaxPages))   /* It won't work , make*/
	  return(iOldGopher);                          /* no changes */
     
    iLastPageLen = iMaxGophers % iPageLen;

    /* Lines on last page */

     iOldLine = iOldGopher - ((nOldPage-1)*iPageLen);/* Old Screen location */
     iNewLine = iNewGopher - ((nNewPage-1)*iPageLen);/* New Screen location */
     
     if ((iNewLine < 0) || (iNewLine > iPageLen))
	  return(iOldGopher);
     
     if (nOldPage != nNewPage)    {
	Display_Dir_Page(gophers,
                        iNewLine, nNewPage, nMaxPages, iPageLen, iLastPageLen);
        row = 2;
        col = (80 - strlen( MenuTitle )) / 2;
        rend = SMG$M_REVERSE;
        ret_status = smg$put_chars(&DisplayId,&d_menutitle,&row,&col);
   }
	
     sprintf(sPagenum, "  Page: %d/%d", nNewPage, nMaxPages);
     Draw_Status(sPagenum);

     row = MENULINE( iOldLine );
     col = 4;
     ret_status = smg$put_chars( &DisplayId, &d_3spaces, &row, &col );

     row = MENULINE( iNewLine );
     col = 4;
     ret_status = smg$put_chars( &DisplayId, &d_arrow, &row, &col );
     col = 7;
     ret_status = smg$set_physical_cursor( &PasteId, &row, &col );

     return(iNewGopher);
}

/*
** This routine draws a numbered menu
** from an array of GopherStructs.
** 
** It returns the number that the user selected, or it returns
** zero if the user decided to cancel.
**
*/

int GetMenu(numitems, items, MenuTitle, typedchar, incomingLine)
  int numitems;
  GopherStruct *items;
  char *MenuTitle;
  int *typedchar;
  int incomingLine;
{
     short int ch;              /* Input character */
     int iItem; /* Display line */
     int ret_status, row, col, re                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         