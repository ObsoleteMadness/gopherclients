/*Last Modified:  30-APR-1992 08:45:25.44, By: MARK */
/* ourutils.c
 * 
 * Part of the Internet Gopher program, copyright (C) 1991
 * University of Minnesota Microcomputer Workstation and Networks Center
 *
 */

#include "gopher.h"

int readkey_row, readkey_col;

/*
** This procedure exits out of the curses environment and
** displays the file indicated by pathname to the screen
** using a pager command of some sort 
*/

void
display_file(Filename)
  char *Filename;
{
     FILE *tmpfile, *fNew;
     char command[MAXSTR];
     char SaveName[MAXSTR];
     short int ch;
     int ret_status;

     strncpy(command, "\0", MAXSTR );
     strcpy(command, PagerCommand);

     /** Execute the PAGER command **/
     strcat(command, " ");
     strcat(command, Filename);

     
     if (strcmp(PagerCommand, "builtin") != 0)
	  system(command);
     else
	  Ourpager(Filename);

     printf("Press <RETURN> to continue");

     if (!SecureMode)
	  printf(", <s> to save, or <p> to print:");
     else
	  printf(":");

     fflush(stdout);

     ch = 0;
     while (ch == 0) {
          ret_status = smg$read_keystroke( &KeyboardId, &ch );

	  if (SecureMode) {
	       switch (ch) {
	       case SMG$K_TRM_LF:
	       case SMG$K_TRM_CR:
	       case SMG$K_TRM_SPACE:
		    break;
	       default:
		    ret_status = smg$ring_bell( &DisplayId );
		    fflush(stdout);
		    ch=0;
		    break;
	       }
	  }
	  else {

	       switch(ch) {
	       case SMG$K_TRM_LF:
	       case SMG$K_TRM_CR:
	       case SMG$K_TRM_SPACE :
		    if (strcmp(Filename,GOPHERHELP)!=0) {
			if (remove(Filename) != 0 )
			  fprintf(stderr, "Couldn't remove %s!!!\n",Filename);
			}
		    
		    break;

	       case SMG$K_TRM_LOWERCASE_S:
	       case SMG$K_TRM_UPPERCASE_S:
                    strncpy(command, "\0", MAXSTR );
                    strncpy(SaveName, "\0", MAXSTR );
		    sprintf(command, "Enter save file name: ");
		    GetOneOption(command, SaveName);
		    if (strlen(SaveName) == 0)
			 break;
		    while ((fNew = fopen(SaveName, "w")) == NULL) {
			 bzero(SaveName, MAXSTR);
                         strncpy(command, "\0", MAXSTR );
			 sprintf(command, "Error opening %s:  Enter new name or <Enter> to cancel: ", SaveName);
			 GetOneOption(command, SaveName);
			 if (SaveName[0] == 0)
			      break;
		    }
              
		    if (fNew == NULL)
			 break;
		    
		    if ((tmpfile = fopen(Filename, "r")) == NULL)
			 fprintf(stderr, "%s cannot be opened.\n", Filename), exit(-1);
		    while (!feof(tmpfile)) fputc(fgetc(tmpfile), fNew);
		    
		    fclose(tmpfile);
		    fclose(fNew);

		    if (strcmp(Filename,GOPHERHELP)!=0) {
			if( remove(Filename) != 0 )
			  fprintf(stderr, "Couldn't remove %s!!!\n",Filename);
			}
		    
		    break;

	       case SMG$K_TRM_LOWERCASE_P:
	       case SMG$K_TRM_UPPERCASE_P:
   	 	    bzero(SaveName, MAXSTR);
                    strncpy(SaveName,"sys$scratch:gopher.print",MAXSTR);
   		    while ((fNew = fopen(SaveName, "w")) == NULL) {
		      bzero(SaveName, MAXSTR);
                      strncpy(command, "\0", MAXSTR );
		      sprintf(command, "Error opening %s:  Enter new name or <Enter> to cancel: ", SaveName);
		      GetOneOption(command, SaveName);
		      if (SaveName[0] == 0)
 		         break;
  		    }

   		    if (fNew == NULL)
   			 break;

   		    if ((tmpfile = fopen(Filename, "r")) == NULL)
   			 fprintf(stderr, "%s cannot be opened.\n", Filename), exit(-1);
   		    while (!feof(tmpfile)) fputc(fgetc(tmpfile), fNew);
   		    
   		    fclose(tmpfile);
   		    fclose(fNew);
                    strncpy(command, "\0", MAXSTR );
		    sprintf(command, "%s %s", PrinterCommand, Filename);
		    system(command);
		    break;

	       default:
		    ret_status = smg$ring_bell( &DisplayId );
		    fflush(stdout);
		    ch=0;
		    break;
	       }
	  }
     }
     
     ret_status = smg$erase_display( &DisplayId );
     fflush(stdout);
}


/*
** This mini pager is intended for people worried about shell escapes from
** more or less or whatever
*/

Ourpager(filename)
  char *filename;
{
     FILE *InFile;
     int i;
     char inputline[512], *cp;
     int Done = FALSE;
     char ZeTypedChar;
     int ret_status;
     
     if ((InFile = fopen(filename, "r")) == NULL)
	  return;

     while (Done == FALSE) {
	  ret_status = smg$erase_display( &DisplayId );

	  for (i=0 ; i < LINES-1; i++) {
	       cp = fgets(inputline, 512, InFile);
	       ZapCRLF(inputline);
	       puts(inputline);
	  }

	  printf("----Press <ENTER> for next page, q to exit------");
	  
          ret_status = smg$read_keystroke( &KeyboardId, &ZeTypedChar );
	  
	  if ((ZeTypedChar == 'q') || (cp == NULL)) {
	       printf("\n");
	       Done = TRUE;
	  }
     }

     fclose(InFile);
}

/*
** Non System V getstr's all seem to be broken in some way.  Anyways
** a normal getstr wouldn't do for us.  This one displays a current value
** That the user can edit.
**
** Also, if a non-printable character is pressed we stop and return it.
** (tab, esc etc. )
*/

char
Mygetstr(inputline)
  char *inputline;
{
     int pointer = 0;
     short int ch;
     int ret_status, read_pos;
     char *typedch, *outch;

     $DESCRIPTOR( d_outch, outch );
     $DESCRIPTOR( d_ch, typedch );
     $DESCRIPTOR( d_space, " " );

     typedch = ( char * ) malloc( 1 );
     outch = ( char * ) malloc( 1 );

     d_ch.dsc$a_pointer = typedch;
     d_ch.dsc$w_length = 1;

     /*** Check to see if there's something in the inputline already ***/
     
     d_outch.dsc$w_length = 1;
     while ( inputline[pointer] != '\0' ) {
	  d_outch.dsc$a_pointer = &inputline[pointer];
	  ret_status = smg$put_chars( &DisplayId, &d_outch, &readkey_row,
			&readkey_col );
	  readkey_col = readkey_col + 1;
	  pointer ++;
     }

     for (;;) {
          ret_status = smg$read_keystroke( &KeyboardId, &ch );
	  *typedch = ch;

	  switch (ch) {

	  case SMG$K_TRM_LF:
	  case SMG$K_TRM_CR:
	  case SMG$K_TRM_HT:
	       inputline[pointer] = '\0';
	       return;
	       break;

	  /**  Backspace and delete **/

	  case SMG$K_TRM_BS:
	  case SMG$K_TRM_DELETE:
	       if (pointer > 0) {
		    readkey_col = readkey_col - 1;
		    ret_status = smg$set_physical_cursor( &PasteId,
			&readkey_row, &readkey_col );
		    ret_status = smg$put_chars( &DisplayId, &d_space,
				&readkey_row, &readkey_col );
		    ret_status = smg$set_physical_cursor( &PasteId,
			&readkey_row, &readkey_col );
		    inputline[--pointer] = '\0';
	       }
	       break;
		    
	  default:
	       if (isprint(ch)) {
		    inputline[pointer++]= ch;
		    ret_status = smg$put_chars( &DisplayId, &d_ch, 
			&readkey_row, &readkey_col );
		    readkey_col = readkey_col + 1;
	       }
	       else
		    return(ch);
	  }
     }
}

/*
** This routine will allow the user to change a whole bunch of fields.
** 
** The maximum number of options is hard set at 9 right now.  It may be
** different in the future.
**
** The space for storing stuff is provided by the caller.  This routine
** will present it to the user in this fashion:
**
**           Option1 : responses1
**           Option2 : responses2
**           .....
**
** It would be wise to keep the length of the options and responses
** below 38 characters.
**
*/


Get_Options(Title, Err, numOptions, Options, Responses)
  char *Title;
  char Err[MAXSTR];
  int    numOptions;
  char **Options;
  char Responses[MAXRESP][MAXSTR];
{
     int         availlines;
     char printstring[WHOLELINE];
     char inputline[WHOLELINE];
     int         optionlen, ret_status, row, col;
     int         maxoptionlen;
     int         i,j;          /** Acme Buggy whips and integers **/
     BOOLEAN     Done = FALSE;
     char        ch, fooch;
     char	 *outstr;

     $DESCRIPTOR( d_title, Title );
     $DESCRIPTOR( d_err, Err );
     $DESCRIPTOR( d_outstr, outstr );
     $DESCRIPTOR( d_options, Options );
     $DESCRIPTOR( d_colon, ": " );
     $DESCRIPTOR( d_resp, Responses );
     $DESCRIPTOR( d_prstr, printstring );

     outstr = ( char * )malloc( 5 );

     while (Done == FALSE) {

          ret_status = smg$erase_display( &DisplayId );
	  Draw_Banner();
	  Draw_Status("Press <RETURN> to exit");
          row = 4;
          col = (80 - strlen( Title )) / 2;
	  d_title.dsc$w_length = strlen( Title );
          ret_status = smg$put_chars( &DisplayId, &d_title, &row, &col );
          row = 5;
          col = (80 - strlen( Err )) / 2;
	  d_err.dsc$w_length = strlen( Err );
          ret_status = smg$put_chars( &DisplayId, &d_err, &row, &col );

	  availlines = LINES - 6;
	  
	  /** Find the longest width of the options strings **/
	  
	  maxoptionlen = 0;
	  
	  for (i=0; i<numOptions; i++) {
	       j= strlen(Options[i]);
	       maxoptionlen = (maxoptionlen > j) ? maxoptionlen:j;
	  }
	  
	  
	  /*** Print out the options in a nice looking fashion ***/
	  
	  for (i=0; i<numOptions; i++) {
	       optionlen = strlen(Options[i]);
	       row = i+6;
	       col = 3;
	       sprintf(outstr,"%d. ",i+1);
	       d_outstr.dsc$w_length = strlen( outstr );
	       d_outstr.dsc$a_pointer = outstr;
	       ret_status = smg$put_chars( &DisplayId, &d_outstr, &row, &col );
	       col = 3 + strlen( outstr );
	       d_options.dsc$w_length = strlen( Options[i] );
	       d_options.dsc$a_pointer = Options[i];
	       ret_status = smg$put_chars( &DisplayId, &d_options, &row, &col );
	       col = maxoptionlen + 6;
	       ret_status = smg$put_chars( &DisplayId, &d_colon, &row, &col );
	       col = maxoptionlen + 8;
	       d_resp.dsc$w_length = strlen( Responses[i] );
	       d_resp.dsc$a_pointer = Responses[i];
	       ret_status = smg$put_chars( &DisplayId, &d_resp, &row, &col );
	  }

	  sprintf(printstring, "Press 1-%d to change a field, Return to accept fields and continue", numOptions);

	  row = LINES - 2;
	  col = (80 - strlen(printstring)) / 2;
	  d_prstr.dsc$w_length = strlen( printstring );
	  ret_status = smg$put_chars( &DisplayId, &d_prstr, &row, &col );

	  /*** Now get some user input ***/

          ret_status = smg$read_keystroke( &KeyboardId, &ch );

	  
#ifdef STUPIDTERM
	  /*** Suck off a return ***/
	  if (ch != '\n' && ch != '\r')
	       
	       do {
                    ret_status = smg$read_keystroke( &KeyboardId, &fooch );
	       }
               while (fooch != '\n' && fooch != '\r');

#endif

	  if (ch == '\n' || ch == '\r' || ch == 'Q' || ch=='q')
	       Done = TRUE;

	  else if (isdigit(ch)) {
	       i = ch - '1' ;
	       Draw_Status("Press Return when finished");
	       readkey_row = i + 6;
	       readkey_col = maxoptionlen + 8;
	       row = i + 6;
	       col = maxoptionlen + 8;
	       ret_status = smg$set_physical_cursor( &PasteId, &row, &col );
	       inputline[0] = '\0';
	       Mygetstr(Responses[i]);
	  }
	  else 
	       ret_status = smg$ring_bell( &DisplayId );
     }
}

/*
 * This allows the user to add or change data in a form.
 *
 * It returns true if the user wants to keep the changes
 * It returns false if the user wants to abort
 *
 */

BOOLEAN
NewForm(ZeForm)
  Form *ZeForm;
{
     int         availlines;
     static char printstring[WHOLELINE];
     int         optionlen;
     int         maxoptionlen;
     int         i,j;          /** Acme Buggy whips and integers **/
     char        ch;

     while (1) {

	  DisplayForm(ZeForm);

	  sprintf(printstring, "Press <ESC> to accept fields and continue");

/*	  Centerline("Press <ESC> to accept fields and continue", LINES-3);
	  Centerline("or press <CTRL-A> to abort", LINES-2);
*/
	  /*** Now get some user input ***/

	  for (i=0; i< ZeForm->numfields; i++) {

	       move(ZeForm->yloc[i],ZeForm->xloc[i] + strlen(ZeForm->tags[i])+2);
	       echo();

	       ch = Mygetstr(ZeForm->values[i]);

	       /*** ESC key ***/
	       if (ch == '\033') {
		    noecho();
		    return(TRUE);
	       }

	       /*** CTRL-A ***/
	       else if (ch == '\001') {
		    noecho();
		    return(FALSE);
	       }
	       noecho();
	  }
     }
}



/*
 * This Displays a form and a "menu".  It displays the Form
 * and then it waits for a keypress.  It returns the value of the
 * key that is pressed.
 */

void
DisplayForm(ZeForm)
  Form *ZeForm;
{
     int         availlines;
     static char printstring[WHOLELINE];
     int         optionlen;
     int         maxoptionlen;
     int         i,j;          /** Acme Buggy whips and integers **/
     BOOLEAN     Done = FALSE;
     char        ch;

     Draw_Banner();
/*     Centerline( ZeForm->Title, 2);
*/     
     availlines = LINES - 6;
     
     /*** Print out the options according to the Form structure ***/
     
     for (i=0; i<ZeForm->numfields; i++) {
	  optionlen = strlen((ZeForm->tags)[i]);
	  mvaddstr(ZeForm->yloc[i],ZeForm->xloc[i], 
		   ZeForm->tags[i]);
	  addch(':');
	  addch(' ');
	  
	  /*** Now add the current value ***/
	  addstr((ZeForm->values)[i]);
     }
}


void
CursesErrorMsg(Message)
  char *Message;
{
     short int ch;
     int i, row, col, ret_status;

     $DESCRIPTOR( d_message, Message );
     $DESCRIPTOR( d_hitcr, " <press RETURN> " );

     d_message.dsc$w_length = strlen( Message );
     row = LINES;
     col =1;
     ret_status = smg$erase_line( &DisplayId, &row, &col );
     ret_status = smg$put_chars( &DisplayId, &d_message, &row, &col );

     col = strlen( Message ) + 2;
     ret_status = smg$put_chars( &DisplayId, &d_hitcr, &row, &col );
     while (ch != SMG$K_TRM_CR )
	  ret_status = smg$read_keystroke( &KeyboardId, &ch );
}
     
GetOneOption(OptionName, Response)
  char *OptionName, *Response;
{
     int ret_status, row, col, resp_len, maxlen;
     char *resp_p1;

     $DESCRIPTOR( d_option, OptionName );
     $DESCRIPTOR( d_resp, Response );

     d_resp.dsc$w_length = MAXSTR-1;
     d_option.dsc$w_length = strlen( OptionName );
     maxlen = MAXSTR - 1;

     row = LINES;
     readkey_row = LINES;
     readkey_col = strlen( OptionName ) + 1;

     col = 1;
     ret_status = smg$erase_line( &DisplayId, &row, &col );
     ret_status = smg$put_chars( &DisplayId, &d_option, &row, &col );

     Mygetstr(Response);
}


/*
 ** This strips off  CR's and LF's leaving us with a nice pure string.
 */

void
ZapCRLF(inputline)
char *inputline;
{
     char *cp;
     
     cp = strchr(inputline, '\r');    /* Zap CR-LF */
     if (cp != NULL)
          *cp = '\0';
     else {
          cp = strchr(inputline, '\n');
          if (cp != NULL)
               *cp = '\0';
     }
}


/*
 * This routine allows the user to "find" a specific event.
 * It doesn't allow them to look at documents, etc.  It just lets
 * them cruise around directories
 */

GopherStruct* 
cruise_dirs(ZeGopher, Message)
  GopherStruct *ZeGopher;
  char *Message;
{
     int Line;
     char SavedTitle;
     short int TypedChar = 0;
     GopherStruct TmpGopher;

     process_request(ZeGopher);

     while (1) {

	  Line = GetMenu(iMenuLines, Gopher, Message, &TypedChar);
	  
	  switch (TypedChar)
	  {

	  case SMG$K_TRM_CR:
	  case SMG$K_TRM_LF:

	       if (Gopher[Line-1].sFileType == '1')
		    process_request(&(Gopher[Line - 1]));
	       else
		    CursesErrorMsg("Cannot select these at this time");
	       break;

	  case SMG$K_TRM_SPACE:
	       /*** Use this directory/calendar ***/

	       return(&(Gopher[Line -1]));
	       break;
	       
	  case SMG$K_TRM_LOWERCASE_U:
	  case SMG$K_TRM_UPPERCASE_U:
	       /*** Go up a directory level ***/
	       popgopher(&TmpGopher);
	       popgopher(&TmpGopher);
	       process_request(&TmpGopher);
	       break;

	  }
     }
}



int
OpenGopherConn(ZeGopher)
  GopherStruct *ZeGopher;
{
     int sockfd;
     static char inputline[MAXLINE];
     int length;

     inputline[0] = '\0';

     if ((sockfd = connect_to_gopher(ZeGopher->sHost, ZeGopher->iPort)) <0) {
	  check_sock(sockfd, ZeGopher->sHost);
	  return(-1);
     }


     /*** Get rid of that darn Banner line ***
     length = readline(sockfd, inputline, MAXLINE); */

     return(sockfd);
     /*** TODO check to see if the server version is cool. ***/

}

#ifdef UCX
 
int bzero(ptr, len)
char *ptr;
int len;
{
  long count;
  for (count = 0; count < len; count++){
    *(char*)(ptr + count) = 0;
    }
}
 
char *bcopy(src, dest, len)
char *dest, *src;
int len;
{
  return((char*)memmove(dest, src, len));
}
 
#endif
