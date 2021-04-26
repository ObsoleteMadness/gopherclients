/*Last Modified:  20-MAR-1992 15:44:06.03, By: MARK */
#include "gopher.h"

/*
 * This creates a new object on the server, The object can be, 
 *    a directory
 *    a file,
 *    a sound, *not supported yet*
 *    a event
 *    a calendar
 *    etc.
 */


static char *ObjTypes[] = {
     "A Directory",
     "A File",
     "A Calendar/User",
     "An Event"
     };

   

void
createobject()
{
     static char Title[HALFLINE];
     static char TmpFilestr[WHOLELINE];
     static char Command[WHOLELINE];
     static char Netstr[MAXLINE];
     static char inputline[MAXLINE];
     char Objtype[5];
     char Public[5];
     char sTmp[5];
     FILE *NewFile = NULL;
     int sockfd, length;
     GopherStruct ThisDir;

     Title[0] = '\0';

     GetOneOption("What would you like to name the new object: ", Title);

     if (Title[0] == '\0')
	  return;

     GetOneOption("What Kind of object is this: ", Objtype);

     popgopher(&ThisDir);
     sprintf(TmpFilestr, "sys$scratch:gopher.%d", getpid());

     switch (Objtype[0]) {
     case '0':
	  /** Make a generic file **/
	  strcpy(Command, "eve ");
	  strcat(Command, TmpFilestr);
	  system(Command);
	  break;

     case 'e':
	  /** Make a new event **/
	  if (createevent(time(NULL)) == FALSE)
	       return;
	  break;

     case '1':
	  /** Make a generic directory **/
	  /** However, we don't need any further data **/
	  break;

     }

     sockfd = connect_to_gopher(ThisDir.sHost, ThisDir.iPort);
     
     if (sockfd < 0) {
	  pushgopher(&ThisDir);
	  return;
     }
     /*** get the version line ***/
	  
     length = readline(sockfd, inputline, MAXLINE);
     
     /*** Send out the create request ***/

     sprintf(Netstr, "%s\tcreateandlink\t%s%s\r\n", ThisDir.sPath,Objtype,Title);
     writestring(sockfd, Netstr);
     

     /* Send out the data field depending on the command */
     switch (Objtype[0]) {

     case 'e':
     case '0':
	  if ( (NewFile = fopen(TmpFilestr, "r")) == NULL) {
	       pushgopher(&ThisDir);
	       CursesErrorMsg("Couldn't open tmp file!");
	       return;
	  }


	  /** Send the file across a line at a time**/
	  while (fgets(inputline, MAXLINE, NewFile) != NULL) {
	       ZapCRLF(inputline);
	       writestring(sockfd, inputline);
	       writestring(sockfd, "\r\n");
	  }

	  writestring(sockfd, ".\r\n");
	  break;


     case '1':
	  break;
     }
     /*** Get the result of the command ***/

     length = readline(sockfd, inputline, MAXLINE);

     if (length == 0) {
	  CursesErrorMsg("Couldn't do it!");
	  pushgopher(&ThisDir);
	  return;
     }
     if (inputline[0] == '-') {
	  CursesErrorMsg(inputline + 2);
	  pushgopher(&ThisDir);
	  return;
     }

     /*** Now retrieve the file capability that the server sends off ***/

     length = readline(sockfd, Netstr, MAXLINE);
     ZapCRLF(Netstr);

     close(sockfd);

     /*** Find out if the user wants the object "public" ***/

     Public[0] = '\0';

     while (Public[0] != 'y' && Public[0] != 'n') {
	  GetOneOption("Make this object public (y/n)? ", Public);
     }


     /*** Now add our name to the access list for the object ***/

     sockfd = connect_to_gopher(ThisDir.sHost, ThisDir.iPort);
     
     if (sockfd < 0) {
	  CursesErrorMsg("Couldn't do it");
	  return;/*** Run and HIDE! ***/
     }

     writestring(sockfd, ThisDir.sPath);
     writestring(sockfd, "\tgrant\t");
     writestring(sockfd, Title);
     writestring(sockfd, "\t");
     writestring(sockfd, Netstr);
     writestring(sockfd, "\t");
     writestring(sockfd, USERCAP);
     writestring(sockfd, "\t");
     writestring(sockfd, Netstr);

     /*** Find out whether we were successful or not. ***/
     
     length = readline(sockfd, inputline, MAXLINE);
     
     if (length == 0) {
	  CursesErrorMsg("Couldn't do it!");
	  pushgopher(&ThisDir);
	  return;
     }
     if (inputline[0] == '-') {
	  CursesErrorMsg(inputline + 2);
	  pushgopher(&ThisDir);
	  return;
     }

     close(sockfd);


     /*** If this event is public, then we add a capability for
       normal folk. **/

     if (Public[0] == 'y') {
	  sockfd = connect_to_gopher(ThisDir.sHost, ThisDir.iPort);
     
	  if (sockfd < 0) {
	       CursesErrorMsg("Couldn't do it");
	       return;/*** Run and HIDE! ***/
	  }

	  writestring(sockfd, ThisDir.sPath);
	  writestring(sockfd, "\tgrant\t");
	  writestring(sockfd, Title);
	  writestring(sockfd, "\t");
	  writestring(sockfd, Netstr);
	  writestring(sockfd, "\t\t");
	  writestring(sockfd, Netstr);
	  
	  /*** Find out whether we were successful or not. ***/
	  
	  length = readline(sockfd, inputline, MAXLINE);
	  
	  if (length == 0) {
	       CursesErrorMsg("Couldn't do it!");
	       pushgopher(&ThisDir);
	       return;
	  }
	  if (inputline[0] == '-') {
	       CursesErrorMsg(inputline + 2, sTmp);
	       pushgopher(&ThisDir);
	       return;
	  }
	  
	  close(sockfd);
     }


     /*** TODO make a way for the user to say that an event is "public" **/

     /*** Clean up our stack ***/
     pushgopher(&ThisDir);

     /*** And our file droppings ***/
     if (NewFile != NULL)
	  fclose(NewFile);
     if (inputline[0] != '\0')
	  remove(TmpFilestr);
}

/** TODO, this needs to be changed for the new deletion semantics.
**/


void
deleteobject(ZeGoph)
  GopherStruct *ZeGoph;
{
     int sockfd, length;
     char inputline[MAXLINE];
     char sTmp[5];
     GopherStruct ThisDir;
     BOOLEAN objlink =FALSE;

     sTmp[0] = '\0';

     if ((sockfd = connect_to_gopher(ZeGoph->sHost, ZeGoph->iPort)) < 0)
	  /*** TODO Panic ***/
	  ;
     
     /*** Get rid of the damn header line ***/
     
     length = readline(sockfd, inputline, MAXLINE);

     /*** Find out if the object is a link or not... ***/
     popgopher(&ThisDir);
     
     if (strcmp(ThisDir.sHost, ZeGoph->sHost) == 0)
	  /*** It's not a link ***/
	  objlink = FALSE;
     else
	  objlink = TRUE;

     /** Send off the command **/

     if (objlink == FALSE) {
	  writestring(sockfd, ZeGoph->sPath);
	  writestring(sockfd, "\tdelete\r\n");
     }
     else {
	  writestring(sockfd, ThisDir.sPath);
	  writestring(sockfd, "\trmlink\t");
	  writestring(sockfd, ZeGoph->sPath);
	  writestring(sockfd, "\t");
	  writestring(sockfd, ZeGoph->sTitle);
	  writestring(sockfd, "\r\n");
     }

     /** get the result of the command **/

     length = readline(sockfd, inputline, MAXLINE);
     
     if (inputline[0] == '-') {
	  CursesErrorMsg(inputline + 2);
     }

     pushgopher(&ThisDir);
	  
}



CreateLink(ZeGopher)
  GopherStruct *ZeGopher;
{
     GopherStruct TmpGopher;
     GopherStruct *Gopherp;
     char LinkName[WHOLELINE];
     char Hostname[WHOLELINE];
     char objcapstr[MAXLINE];
     char Command[MAXLINE];
     char ObjType;
     int Port, sockfd, length;
     char inputline[MAXLINE];

     /**
      ** We know the object that the user wants to make a link to.
      ** But we have to be careful, cruise_dirs can nuke a lotta stuff.
      **/
     
     ObjType = ZeGopher->sFileType;
     strcpy(LinkName, ZeGopher->sTitle);
     strcpy(Hostname, ZeGopher->sHost);
     strcpy(objcapstr, ZeGopher->sPath);
     Port = ZeGopher->iPort;

     /*
      * Now we must find out where they want to create it.
      */

     popgopher(&TmpGopher);

     Gopherp = 
	  cruise_dirs(&TmpGopher, "Find me a new directory to link in");
     /** Gopherp is the directory where we want to put the link in. **/

     /* for now make the link the same name as the original **/

/*     GetOneOption("What shall the name of the link be? ", LinkName);*/

     /*
      * Now we encode the arguments for the pRPC call semantic.
      */

     sprintf(Command, "%s\tmklink\t%c%s\t%s\t%s\t%d\r\n",
	     Gopherp->sPath, ObjType, LinkName, objcapstr, Hostname, Port);

     /** Now connect to the host with the directory **/

     sockfd = connect_to_gopher(Gopherp->sHost, Gopherp->iPort);
     
     /** Get rid of that darn message **/
     length = readline(sockfd, inputline, MAXLINE);

     /** Send out the command ***/
     writestring(sockfd, Command);
     
     /** Get the result **/
     length = readline(sockfd, inputline, MAXLINE);

     if (inputline[0] == '-')
	  CursesErrorMsg(inputline + 2);

     close(sockfd);

}
     
