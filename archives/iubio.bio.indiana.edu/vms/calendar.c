/*Last Modified:  30-APR-1992 08:44:44.42, By: MARK */
/*
 * The following routines display events in a really pretty fashion.
 *
 * They also allow one to easily enter an event
 */

#include "gopher.h"
#include "calendar.h"

#define EVENTFIELDS 10

static char * Eventtags[] = {
     "Time",
     "Month",
     "Day",
     "Year",
     "Length",
     "to",

     "Recurs (y/n)",
     "Recur length",
     
     "What",
     "Where"
     };

static int Eventxloc[] = {
     8, 7, 9, 8, 26, 38, 26, 26, 8, 7
     };

static int Eventyloc[] = {
     5, 6, 7, 8, 5, 5, 7, 8, 10, 11
     };

static Form EventForm = {
     EVENTFIELDS,
     "Make a New Event",
     Eventtags,
     NULL,
     NULL,
     Eventxloc,
     Eventyloc
     };
	  

/** TODO there should be some way to pass in the date and time to be
    the default ***/

static char* Months[] = {
     "jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"
     };
static char* Dates[] = {
     "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12"
     };


BOOLEAN
createevent(When)
  time_t When;
{
     char *EventResponses[EVENTFIELDS];
     int i;
     FILE *TmpFile;
     char filename[40];
     char *ZeTime;
     char *cp;

     for (i=0 ; i<EVENTFIELDS; i++) {
	  EventResponses[i] = (char *)malloc(MAXSTR);
	  bzero(EventResponses[i], MAXSTR);
	  EventResponses[i][0] = '\0';	  
     }

     /** Load up the given date into the date fields **/

     ZeTime = asctime(localtime(&When));

     strncpy(EventResponses[0], ZeTime + 11, 5); /** The time **/
     EventResponses[0][6] = '\0';
     strncpy(EventResponses[1], ZeTime + 4, 3);  /** The month **/
     EventResponses[1][4] = '\0';
     strncpy(EventResponses[2], ZeTime + 8, 2);  /** The Day
     EventResponses[2][3] = '\0';
     strncpy(EventResponses[3], ZeTime + 20, 4);  /** The Year **/
     EventResponses[3][5] = '\0';
     
     EventForm.values = EventResponses;

     if (NewForm(&EventForm) == FALSE)
	  return(FALSE);

     /*** Put the stuff in a file in /tmp  In the proper format***/

     sprintf(filename, "/tmp/gopher.%d", getpid());

     if ( (TmpFile = fopen(filename, "w")) == NULL) {
	  /*** Something's really screwed...... **/
	  printf("Arrrrrrrrrrrrrrrrrrrghhhhh!!!!!!\n");
	  return(FALSE);
     }


     /*** Output the year ***/
     
     fprintf(TmpFile, "Time: %s", EventResponses[3]);

     /*** Map the month names to numbers ***/
     /*** TODO, gotta check if the month is hosed! ***/

     for (i=0; i < 12; i++) {
	  if (strncasecmp(EventResponses[1], Months[i], 3) == 0) {
	       fprintf(TmpFile, "%s", Dates[i]);
	       break;
	  }
     }

     /*** Output the day of the month ***/

     switch (strlen(EventResponses[2])) {
     case 2:
	  fprintf(TmpFile, "%s", EventResponses[2]);
	  break;
     case 1:
	  fprintf(TmpFile, "0%s", EventResponses[2]);
	  break;
     case 3:
	  /*** Error! ***/
	  /*** TODO display a message ***/
	  return(FALSE);
	  break;
     }

     /** Yes, the elusive time! **/
     
     /** Make sure the hour is correct **/
     if (isdigit(EventResponses[0][0]) && (EventResponses[0][1] == ':')) {
	  fputc('0', TmpFile);
	  fputc(EventResponses[0][0], TmpFile);
     }
     else if (isdigit(EventResponses[0][0]) && isdigit(EventResponses[0][1])) {
	  fputc(EventResponses[0][0], TmpFile);
	  fputc(EventResponses[0][1], TmpFile);
     }
     
     else
	  /*** Error!!! ***/
	  return(FALSE);


     /** Now for the minutes **/
     cp = EventResponses[0];
     while (*cp != ':')
	  cp++;
     
     cp++;
     
     fputc(*cp, TmpFile);
     fputc(*(cp+1), TmpFile);

     /** Put out the seconds 00 **/
     fputc('0', TmpFile);
     fputc('0', TmpFile);

     /*** Length min and length max ***/
     fprintf(TmpFile, " %s %s\n", EventResponses[4], EventResponses[5]);


     /*** TODO need to convert USERCAP to the real username ***/
     fprintf(TmpFile, "From: %s\n", USERCAP);
     fprintf(TmpFile, "Summary: %s\n", EventResponses[8]);
     fprintf(TmpFile, "Location: %s\n", EventResponses[9]);
     
     /*** Write out the recurs line ***/
     if (EventResponses[6][0] == 'y' ||EventResponses[6][0] == 'Y') {
	  fprintf(TmpFile, "Recurs: %s\n", EventResponses[7]);
     }

     fclose(TmpFile);

     return(TRUE);
}




#define EVDSPFIELDS 9

static char * Evdsptags[] = {
     "When",
     "Length",
     "to",

     "Recurs",
     
     "What",
     "Where",
     
     "Created By",
     "Attendees",
     "Invited",
     };


static int Evdspxloc[] = {
     8, 38, 50, 6, 8, 7, 2, 3, 5, 3
     };

static int Evdspyloc[] = {
     5, 5, 5, 6, 7, 8, 9, 10, 11, 12
     };

static char Evdsptitle[80];

static Form EvdspForm = {
     EVDSPFIELDS,
     Evdsptitle,
     Evdsptags,
     NULL,
     NULL,
     Evdspxloc,
     Evdspyloc
     };


/** 
 ** This guy loads up the EvdspForm with values from a file.
 */

EventFileToForm(tmpfile, Title) 
  FILE *tmpfile;
{
     char inputline[MAXLINE];
     char *cp, *acp;
     static char *values[EVDSPFIELDS];
     int i;

     strcpy(EvdspForm.Title, Title);

     for (i=0 ; i<EVDSPFIELDS; i++) {
	  values[i] = (char *)malloc(MAXSTR);
	  bzero(values[i], MAXSTR);
     }

     while (fgets(inputline, 512, tmpfile) != NULL) {
	  ZapCRLF(inputline);
     
	  if (strncasecmp("Time: ", inputline, 6) == 0) {
	       cp = inputline+5;
	       while(*(cp++) == ' ');
	       cp--;

	       /** cp is now at the start of the time **/
	       ISOasctime(cp, values[0]);

	       cp = cp + 14;
	       while(*(cp++) == ' ');
	       
	       /** cp is now at the length min **/
	       
	       acp = cp;
	       while(*(acp++) == ' ');
	       
	       strncpy(values[1], cp-1, acp-cp+1);

	       cp =acp;
	       while(*(cp++) == ' ');
	       
	       strcpy(values[2], cp-1);
	  }

	  else if (strncasecmp("Recurs: ", inputline, 8)==0) {
	       cp = inputline + 8;
	       
	       /** Should test that this value isn't m/y **/
	       i = atoi(cp);
	       
	       if (i==7)
		    strcpy(values[3], "Weekly");
	       else if (i==1)
		    strcpy(values[3], "Daily");
	       else if (i==14)
		    strcpy(values[3], "Biweekly");
	       else 
		    sprintf(values[3], "Every %d Days", i);
	  }

	  else if (strncasecmp("Summary: ", inputline, 9) == 0)
	       strcpy(values[4], inputline + 9);
	  
	  else if (strncasecmp("Location: ", inputline, 10) == 0)
	       strcpy(values[5], inputline + 10);
	  
	  else if (strncasecmp("From: ", inputline, 6) == 0)
	       strcpy(values[6], inputline + 6);
	  
	  else if (strncasecmp("Invited: ", inputline, 9) == 0)
	       strcpy(values[8], inputline + 9);

	  else if (strncasecmp("Attending: ", inputline, 11) == 0)
	       strcpy(values[7], inputline +11);

     }

     EvdspForm.values = values;
}



BOOLEAN
GetEventFile(ZeGopher, tmpfile)
  GopherStruct *ZeGopher;
  FILE *tmpfile;
{
     char Attendingstr[MAXLINE], Invitedstr[MAXLINE];
     int sockfd, length;
     static char inputline[MAXLINE];
     char *cp;

     /*** Initialize variables ***/
     inputline[0] = '\0';

     /*** Get the event from the net ***/
     sockfd = OpenGopherConn(ZeGopher);

     Draw_Status("Receiving Event...");   
     refresh();

     /** Send out the request **/
     writestring(sockfd, ZeGopher->sPath);
     writestring(sockfd, "\tfetch\r\n");

     /** Check the result of the command.  See if we screwed up... **/
     length = readline(sockfd, inputline, MAXLINE);
     if (inputline[0]=='-') {
	  CursesErrorMsg(inputline + 2);
	  return;
     }
     

     for(;;) {
	  length = readline(sockfd, inputline, 512);
	  
	  if (length == 0)
	       break;

	  ZapCRLF(inputline);
	  
	  if ((inputline[0] == '.') && (inputline[1] == '\0'))
	       break;
	  else {
	       fprintf(tmpfile, "%s\n", inputline);
	  }
     }

     close(sockfd);
     
     /** Now let's find out how many people are going to this meeting **/
     sockfd = OpenGopherConn(ZeGopher);

     /** Send out the request **/
     writestring(sockfd, ZeGopher->sPath);
     writestring(sockfd, "\twhosat\r\n");

     /** Check the result of the command.  See if we screwed up... **/

     length = readline(sockfd, inputline, MAXLINE);
     if (inputline[0]=='-') {
	  CursesErrorMsg(inputline + 2);
	  fclose(tmpfile);
	  close(sockfd);
	  return(FALSE);
     }

     /** Okay, now we'll just put the list of users in the form
       Invited: <user1> <user2>  .... and
       Attending: <user1> <user2> ....
     ***/

     bzero(Attendingstr, MAXLINE);
     bzero(Invitedstr, MAXLINE);

     for(;;) {
	  length = readline(sockfd, inputline, 512);
	  
	  if (length == 0)
	       break;

	  ZapCRLF(inputline);
	  
	  if ((inputline[0] == '.') && (inputline[1] == '\0'))
	       break;
	  else {
	       cp = strchr(inputline, '\t');
	       if (cp != NULL) {
		    cp++;
		    if (strncasecmp(cp, "invited", 7) == 0) {
			 strncat(Invitedstr, inputline, cp-inputline-1);
			 strcat(Invitedstr, ",");
		    }
		    else if (strncasecmp(cp, "attending", 9) == 0) {
			 strncat(Attendingstr, inputline, cp-inputline-1);
			 strcat(Invitedstr, ",");
		    }
	       }
	  }
     }
     /*** TODO Take off the last comma from the invited string. ***/


     fprintf(tmpfile, "Invited: %s\n", Invitedstr);
     fprintf(tmpfile, "Attending: %s\n", Attendingstr);

     return(TRUE); /*** Success ***/
}




EventInvite(ZeGopher, username)
  GopherStruct *ZeGopher;
  char *username;
{
     int sockfd, length;
     char inputline[MAXLINE];

     /** Open a connection to the server **/
     sockfd = OpenGopherConn(ZeGopher);

     /*** Send out the request ***/
     writestring(sockfd, ZeGopher->sPath);
     writestring(sockfd, "\tinvite\t");
     writestring(sockfd, username);
     writestring(sockfd, "\r\n");

     /*** get the response from the net.. ***/
     length = readline(sockfd, inputline, MAXLINE);

     if (inputline[0] == '-') {
	  CursesErrorMsg(inputline + 2);
	  return;
     }
}


HandleEvent(ZeGopher)
  GopherStruct *ZeGopher;
{
     BOOLEAN Done = FALSE;

     int i=0, length, sockfd;
     char tmpfilename[HALFLINE];
     FILE *tmpfile;
     char inputline[512];
     char sTmp[5];
     char ch, *cp;
     GopherStruct TmpGopher, *Gopherp, SaveGoph;


     SaveGoph.sFileType = ZeGopher->sFileType;
     strcpy(SaveGoph.sTitle, ZeGopher->sTitle);
     strcpy(SaveGoph.sHost, ZeGopher->sHost);
     strcpy(SaveGoph.sPath, ZeGopher->sPath);
     SaveGoph.iPort = ZeGopher->iPort;


     /** Open a temporary file **/

     sprintf(tmpfilename, "/tmp/gopher.%d",getpid());

     if ((tmpfile = fopen(tmpfilename, "w")) == NULL) {
	  CursesErrorMsg("Couldn't make a tmp file!");
	  return;
     }
     
     GetEventFile(ZeGopher, tmpfile);

     /*** Reopen the file as read-only ***/
     fclose(tmpfile);

     if ((tmpfile = fopen(tmpfilename, "r")) == NULL) {
	  CursesErrorMsg("Whoa!!!!!");
	  return;
     }

     /*** Load the event into the form ***/
     EventFileToForm(tmpfile, ZeGopher->sTitle);
     fclose(tmpfile);


     while (!Done) {

     
	  /** Display the form **/
	  DisplayForm(EvdspForm);
/*	  Centerline("I - Invite, D - Details, M - Modify, ESC - exit", LINES - 4);
	  Centerline("A - Accept Invitation, C - Decline Invitation", LINES - 3);
*/
	  refresh();

	  ch = getch();
	  switch (ch) {

	  case '\033':
	       Done = TRUE;
	       break;
	  case 'i':
	  case 'I':
	       /*** Call the inviter here ***/
	       /**** Hack alert !!***/

	       CursesErrorMsg("Please show me the calendar of the person you want to invite,");
	       /** Save IT!!! ***/
	       
	       CreateLink(ZeGopher);

	       /*** Find out who we this user is ***/

	       inputline[0] = '\0';
	       GetOneOption("What is this person's name? ", inputline);
	       
	       /*** Now invite them to the event ***/
	       EventInvite(&SaveGoph, inputline);

	       /*** Now we must get the form again ***/

	       sprintf(tmpfilename, "/tmp/gopher.%d",getpid());
	       if ((tmpfile = fopen(tmpfilename, "w")) == NULL) {
		    CursesErrorMsg("Couldn't make a tmp file!");
		    return;
	       }
     
	       GetEventFile(&SaveGoph, tmpfile);
	       
	       /*** Reopen the file as read-only ***/
	       fclose(tmpfile);
	       
	       if ((tmpfile = fopen(tmpfilename, "r")) == NULL) {
		    CursesErrorMsg("Whoa!!!!!");
		    return;
	       }

	       /*** Load the event into the form ***/
	       EventFileToForm(tmpfile, SaveGoph.sTitle);
	       fclose(tmpfile);

	       break;

	  case 'D':
	  case 'd':
	       /*** Decline the invitation ***/
	       
	       sockfd = OpenGopherConn(ZeGopher);
	       
	       /** Send out the request **/
	       writestring(sockfd, ZeGopher->sPath);
	       writestring(sockfd, "\tmodpers\t");
	       writestring(sockfd, USERCAP);
	       writestring(sockfd, "\r\n");

	       /** Modify our status field ***/
	       writestring(sockfd, "Status: Absent\r\n");
	       writestring(sockfd, ".\r\n");

	       /*** Get the result of the modpers command ***/
	       inputline[0] = '-';
	       length = readline(sockfd, inputline, MAXLINE);
	       if (inputline[0] == '-') {
		    CursesErrorMsg(inputline + 2);
	       }
	       
	       close(sockfd);

	       /*** Now we must get the form again ***/

	       sprintf(tmpfilename, "/tmp/gopher.%d",getpid());
	       if ((tmpfile = fopen(tmpfilename, "w")) == NULL) {
		    CursesErrorMsg("Couldn't make a tmp file!");
		    return;
	       }
     
	       GetEventFile(&SaveGoph, tmpfile);
	       
	       /*** Reopen the file as read-only ***/
	       fclose(tmpfile);
	       
	       if ((tmpfile = fopen(tmpfilename, "r")) == NULL) {
		    CursesErrorMsg("Whoa!!!!!");
		    return;
	       }

	       /*** Load the event into the form ***/
	       EventFileToForm(tmpfile, SaveGoph.sTitle);
	       fclose(tmpfile);



	       break;

	  case 'a':
	  case 'A':
	       /*** Accept the invitation ***/
	       sockfd = OpenGopherConn(&SaveGoph);
	       
	       /** Send out the request **/
	       writestring(sockfd, SaveGoph.sPath);
	       writestring(sockfd, "\tmodpers\t");
	       writestring(sockfd, USERCAP);
	       writestring(sockfd, "\r\n");

	       /** Modify our status field ***/
	       writestring(sockfd, "Status: attending\r\n");
	       writestring(sockfd, ".\r\n");

	       /*** Get the result of the modpers command ***/
	       inputline[0] = '-';
	       length = readline(sockfd, inputline, MAXLINE);
	       if (inputline[0] == '-') {
		    CursesErrorMsg(inputline + 2);
	       }
	       
	       close(sockfd);

	       /*** Now we must get the form again ***/

	       sprintf(tmpfilename, "/tmp/gopher.%d",getpid());
	       if ((tmpfile = fopen(tmpfilename, "w")) == NULL) {
		    CursesErrorMsg("Couldn't make a tmp file!");
		    return;
	       }
     
	       GetEventFile(&SaveGoph, tmpfile);
	       
	       /*** Reopen the file as read-only ***/
	       fclose(tmpfile);
	       
	       if ((tmpfile = fopen(tmpfilename, "r")) == NULL) {
		    CursesErrorMsg("Whoa!!!!!");
		    return;
	       }

	       /*** Load the event into the form ***/
	       EventFileToForm(tmpfile, SaveGoph.sTitle);
	       fclose(tmpfile);



	       break;
	       

	  case 'm':
	  case 'M':
	       break;
	  }
     }

     /** Good little clients clean up after themselves..**/

     if (remove(tmpfilename)!=0)
	  fprintf(stderr, "Couldn't remove %s!!!\n");
}



/*
 * This takes the iso standard format time and puts it into a more
 * palatable format
 */

ISOasctime(isotime, chartime)
  char *isotime;
  char *chartime;
{
     /** Snarf the time off **/
     
     /**     1991 01 01 23 00 00**/

     /** the time **/

     int month;

     chartime[0] = isotime[8];
     chartime[1] = isotime[9];
     chartime[2] = ':';
     chartime[3] = isotime[10];
     chartime[4] = isotime[11];
     chartime[5] = ' ';

     month = (isotime[4]-'0') * 10 + (isotime[5] - '0');
     
     strncpy(chartime +6, Months[month-1], 3);

     chartime[9] = ' ';

     /** The day **/

     chartime[10] = isotime[6];
     chartime[11] = isotime[7];

     chartime[12] = ' ';

     strncpy(chartime+13, isotime, 4);

     chartime[17] = '\0';
}
