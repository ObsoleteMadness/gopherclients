/********************************************************************
 * lindner
 * 3.16
 * 1994/04/13 04:26:35
 * /home/mudhoney/GopherSrc/CVS/gopher+/gopherd/Waisindex.c,v
 * Exp
 *
 * Paul Lindner, University of Minnesota CIS.
 *
 * Copyright 1991, 1992 by the Regents of the University of Minnesota
 * see the file "Copyright" in the distribution for conditions of use.

Wais_sf_index.c
unless you modify gopherd's Makefile, replace the gopherd/Waisindex.c
with this file to use freeWAIS-sf.  

dgg's hack to use with freeWAIS-sf version including field features
5 aug 94


 * MODULE: Waisindex.c
 * Routines to translate wais indexes on disk to gopher
 *********************************************************************
 * Revision History:
 * Waisindex.c,v
 * Revision 3.16  1994/04/13  04:26:35  lindner
 * Fix for freewais (again)
 *
 * Revision 3.15  1994/04/13  03:40:43  lindner
 * Remove logfile definitions so the code works with freeWAIS 0.3... sigh..
 *
 * Revision 3.14  1994/03/31  22:43:35  lindner
 * Fix for max hit params
 *
 * Revision 3.13  1994/03/08  15:55:22  lindner
 * gcc -Wall fixes
 *
 * Revision 3.12  1993/11/02  06:02:39  lindner
 * HTML Mods
 *
 * Revision 3.11  1993/09/30  16:56:49  lindner
 * Fix for WAIS and $ requests
 *
 * Revision 3.10  1993/09/11  04:41:43  lindner
 * Don't fork for localhost mindex databases
 *
 * Revision 3.9  1993/08/23  18:33:35  lindner
 * Fix bug with DGs patch
 *
 * Revision 3.8  1993/08/23  02:34:58  lindner
 * Limit # of hits
 *
 * Revision 3.7  1993/07/27  05:27:37  lindner
 * Mondo Debug overhaul from Mitra
 *
 * Revision 3.6  1993/07/26  15:31:10  lindner
 * mods for application/gopher-menu
 *
 * Revision 3.5  1993/07/20  23:55:38  lindner
 * LOGGopher mods
 *
 * Revision 3.4  1993/06/11  16:49:10  lindner
 * Fix for freeWAIS and Gopher+ requests
 *
 * Revision 3.3  1993/04/09  16:24:03  lindner
 * Hacks to support WAIS GIF, TIFF, DVI types
 *
 * Revision 3.2  1993/03/26  19:46:52  lindner
 * First crack at gopherplussing Indexing
 *
 * Revision 3.1.1.1  1993/02/11  18:02:50  lindner
 * Gopher+1.2beta release
 *
 * Revision 1.5  1993/01/30  23:55:36  lindner
 * Better error messages, changed a uchdir to a rchdir
 *
 * Revision 1.4  1993/01/05  02:41:28  lindner
 * .cap files are now ignored by the indexer
 *
 * Revision 1.3  1993/01/01  00:12:41  lindner
 * Fixed parameters to GDnew()
 *
 * Revision 1.2  1992/12/21  20:36:44  lindner
 * Added #include for cutil.h (from dgg)
 *
 * Revision 1.1  1992/12/10  23:13:27  lindner
 * gopher 1.1 release
 *
 *
 *********************************************************************/

#if defined(WAISSEARCH)

	/* freewais-sf define -- fix this in Makefile !? */
#define FIELDS

#define _search_c

int ShowDate = 0;

#include "gopherd.h"
#include "Debug.h"

#if defined(_AIX)
#define ANSI_LIKE
#endif


#include "../ir/irext.h"
#include "../ir/irsearch.h"
#include "../ir/docid.h"
#include "../ir/irtfiles.h"
#include "../ir/irfiles.h"
#include "../ir/cutil.h"   
#include <math.h>

#ifdef FIELDS /* tung, 1/94 */
#include "../ir/field_search.h"
#include "../ir/analyse_str.h"
#endif


/* #ifdef FREEWAIS_SF */

FILE *waislogfile = NULL;
char *wais_log_file_name = NULL;

#ifndef HAVE_ALPHASORT
int
  alphasort(d1, d2)
struct dirent **d1;
struct dirent **d2;
{
  return strcmp((*d1)->d_name, (*d2)->d_name);
}
#endif /* HAVE_ALPHASORT */



static char *DefaultDB = "index";

#if defined(void)
#undef void
#endif

char *log_file_name;


int
Process_Veronica(besthit, gs)
  hit *besthit;
  GopherObj *gs;
{
     FILE *ZeFile;
     char veronicabuf[1024];
     char *data, *cp;

     /*** Open up the file and seek to the right position ***/

     ZeFile = ufopen(besthit->filename, "r");

     if (ZeFile == NULL)
	  return(-1);

     fseek(ZeFile, besthit->start_character, 0);

     bzero(veronicabuf, sizeof(veronicabuf));
     fread(veronicabuf, 1, besthit->end_character - besthit->start_character,
	   ZeFile);
     veronicabuf[besthit->end_character - besthit->start_character+1] = '\0';
     
     
     data = veronicabuf;
     GSsetType(gs, *data);
     
     ZapCRLF(data);
     
     cp = strchr(data, '\t');
     *cp = '\0';
     GSsetTitle(gs, data+1);
     
     data = cp+1;
     cp = strchr(data, '\t');
     *cp = '\0';
     GSsetPath(gs, data);
     
     data = cp + 1;
     cp = strchr(data, '\t');
     *cp = '\0';
     GSsetHost(gs, data);
     
     GSsetPort(gs, atoi(cp+1));

     fclose(ZeFile);
     return(0);
}

void
WaisIndexQuery(sockfd, index_directory, SearchWords, new_db_name, INDEXHost, INDEXPort, INDEXPath, isgplus, view)
  int sockfd;
  char *index_directory;
  char *SearchWords;
  char *new_db_name;
  char *INDEXHost;
  int  INDEXPort;
  char *INDEXPath;
  boolean isgplus;
  char *view;
{ 
     database*            db;
     double               maxRawScore, normalScore;
     long		  i;
     char                 *cp, *Selstrout;
     query_parameter_type parameters;
     boolean              search_result;
     char                 score[6];
     char                 ReturnLine[512];
     boolean              plusdirs=FALSE;
     char 		  					newpath[512];
     char                 * sidename;    /* mtm 11-23-92 */
     FILE                 * SideFile = NULL;  /* mtm 11-23-92 */
     GopherDirObj         *gd;
     GopherObj            *gs;
    
#ifdef FIELDS /* tung, 1/94 */
      extern long 		number_of_operands;
      extern char* 		field_name_array[];
      char 						field_name[MAX_FILENAME_LEN + 1];
      char* 					SeedWords = NULL;
      long 						local_number_of_operands = 1;
      long 						number_of_fields = 0;
      int 						global_dictionary_exists = 0;
#endif
 

     gs = GSnew();
     gd = GDnew(32);

     if (view != NULL) {
	  		if (strcasecmp(view, "application/gopher+-menu")==0)
	      	plusdirs = TRUE;
     		}

     Debug("WaisIndexQuery: IndexPath: %s\n", INDEXPath);

     if (new_db_name == NULL)  new_db_name = DefaultDB;
      
     if (view != NULL) {
	  		if (strcasecmp(view, "application/gopher+-menu")==0)
	       plusdirs = TRUE;
     		}

     if (uchdir(index_directory)) {
	  		sprintf(ReturnLine, "Couldn't change to index directory '%s'",
					index_directory);
	  		Abortoutput(sockfd, ReturnLine);
	  		return;
     		}

     if (SearchWords != NULL && strlen(SearchWords) == 0) {
	  		EveryWAISdocument(new_db_name);
	  		return;
     		}
			
		parameters.max_hit_retrieved = 256;

/* check query string for ">max_retrieve" number */
/* dgg - changed this to "maxrec=#max" for use with field-wais, 
   since fieldname>#num is a legit query */
{
     char *hitcp, *numcp;
     int  ihit;

     hitcp = strstr( SearchWords, "maxrec=");
     if ( hitcp && hitcp > SearchWords) {
				numcp = hitcp + 7; 
				while (isspace(*numcp)) numcp++;
				if (isdigit(*numcp) ) {
	  			ihit= atoi( numcp);
	  			if (ihit>0) {
	       		parameters.max_hit_retrieved = ihit;
	       		while (hitcp < numcp) *hitcp++= ' ';
	       		while (isdigit(*hitcp)) *hitcp++= ' ';
	       		/* drop rest of string */
 	       		}
	  			}
     	}
}

/* dgg - ? also need to look for optional key to substitute grouping delimiters ? */
/* e.g.,  query:  fld=(bob or joe) and (date>910101 and date<940531) groupset=() */



#ifdef FIELDS
  if ((SearchWords = analyse_string(SearchWords,
                              &number_of_fields,
                              &global_dictionary_exists)) == NULL) {
       sprintf(ReturnLine, "query syntax error: '%s'",SearchWords);
       Abortoutput(sockfd, ReturnLine);
       return;
       }
     db = openDatabase(new_db_name, false, true, (number_of_fields > 0)?true:false);
#else
     db = openDatabase(new_db_name, false, true);
#endif
     
     if (db == NULL) {
	  		sprintf(ReturnLine, "Failed to open database %s in index dir %s", new_db_name,
			 			index_directory);
	  		Abortoutput(sockfd, ReturnLine);
	  		return;
     		}

#ifdef FIELDS /* tung, 1/94 */
  	if(number_of_fields > 0) {
    	insert_fields(field_name_array, number_of_fields, db);
    	if (!open_field_streams_for_search(field_name, global_dictionary_exists, db)){
       	sprintf(ReturnLine, "missing field: '%s'",field_name);
       	Abortoutput(sockfd, ReturnLine);
       	return;
       	}
  		}
/*
  	local_number_of_operands = number_of_operands;
  	number_of_operands = 1;
    number_of_operands = local_number_of_operands;
*/
#endif
     
#ifdef BIO            /* dgg */
{
     char *cp= read_delimiters( db);  /* use data-specific delim, available */
     if (cp != NULL) {
	  		strcpy( gDelimiters, cp);
	  		wordDelimiter= wordbreak_user;
     		}
     else
	  		wordDelimiter= wordbreak_notalnum;
}
#endif

     set_query_parameter(SET_MAX_RETRIEVED_MASK, &parameters);

     search_result = search_for_words(SearchWords, db, 0);
 
     if (isgplus)  GSsendHeader(sockfd, -1);

     if (search_result == true) {
	  		/* the search went ok */
	  		hit best_hit;

	  		finished_search_word(db);
      	init_best_hit(db);

/* print_best_hits(); - debug */

	 	 		Debug("WaisIndexQuery:After finished_search\n",0);

	  		uchdir(Data_Dir); /* necessary to find side files */

	  		for (i = 0; i < parameters.max_hit_retrieved; i++){ 
	       int hiterr;

	       hiterr =  next_best_hit(&best_hit, db);
	       if (0 != hiterr) {
		    		break;		/* out of hits */
		    		}
	       if (i == 0)  maxRawScore = best_hit.weight;
	       if (best_hit.weight > 0 && 
		   			strstr(best_hit.filename, ".cache")==NULL &&
		   			strstr(best_hit.filename, ".cap/")==NULL){

		    normalScore = floor(( best_hit.weight / maxRawScore) *	
					      (MAX_NORMAL_SCORE + 1));

				if (normalScore > MAX_NORMAL_SCORE)	normalScore = MAX_NORMAL_SCORE;
		    

		    /*** Strip off the first part of the path in the filename*/
		    /*** Plus it gets rid of weird automount things... ***/
#if 0
/* ----- original */
		    Selstrout =strstr(best_hit.filename, INDEXPath);
		    if (Selstrout == NULL)
			 		Selstrout = "Error in Hostdata!";
		    else
			 		Selstrout += strlen(INDEXPath);
		    
#else
     /* "PLEEZ, OH PLEEZ, let me use wais's real pathnames" -- sez dgg ! */
     /* let INDEXPath = "/real/path/to/wais-src;/gopher/path/to/wais-src" */
     /* and after cutting out /real/path; prepend /gopher/path */
{
 		    char *prefix= strchr(INDEXPath, ';');
  		  if (prefix != NULL) {
   				*prefix++= '\0'; /* terminate /real/path */
  				cp= strstr(best_hit.filename, INDEXPath);
  				if (cp==NULL)  
						sprintf( newpath, 
							"wasisearch error, besthit.filename '%s', indexpath '%s'", 
							best_hit.filename, INDEXPath);
 			 		else {
			 			cp += strlen(INDEXPath);
  		 			strcpy( newpath, prefix);
  		 			strcat( newpath, cp);
			 			}
  				cp= newpath;
  				*(--prefix)= ';';    /* put it back for next user */
  				}
    	else {
  			cp =strstr(best_hit.filename, INDEXPath);
  			if (cp == NULL) cp = "Error in Hostdata!";
  			else cp += strlen(INDEXPath);
  			}
 		 	Selstrout= cp;
}
#endif

       	sprintf(score,"lf ",best_hit.weight);
		    
		    /** Make the outgoing string **/

		    ZapCRLF(best_hit.headline);
		    
		    /*** Remove the gopher data directory pathname if
		         it's there from the headline
		    ***/

		    if ((cp = strstr(best_hit.headline, INDEXPath)) != NULL) {
			 		/*** Dangerous.... ***/
			 		strcpy(cp, cp+strlen(INDEXPath));
		    	} 
			 
		    if ((strstr(best_hit.type, "PS") != NULL)
						|| (strstr(best_hit.type, "DVI") != NULL)
						|| (strstr(best_hit.type, "GIF") != NULL)
						|| (strstr(best_hit.type, "TIFF") != NULL))
			 				GSsetType(gs, A_IMAGE);
		    else
			 				GSsetType(gs, A_FILE);
		    /* ? add g+views info for wais types (including doc sizes) ? */

#ifdef ADD_DATE_AND_TIME
		    {
		    char  header[512];
		    /* date is in "941231" format - change to "31Dec94" ? */
		    sprintf(header, "%s [%s, %dk]",best_hit.headline, best_hit.date, 
							(best_hit.document_length / 1024));
		    GSsetTitle(gs, header);
		    }
#else
		    GSsetTitle(gs, best_hit.headline);
#endif
		    GSsetHost(gs, INDEXHost);
		    GSsetPort(gs, INDEXPort);
		    if (GSisGplus(gs)) GSsetModDate(gs, best_hit.date);/* dgg */

		     /* removed "/" from following line (before %s) . 
			    Was getting double slash at least with w8b5bio; 
			    mtm 11-23-92 */

		    sprintf(ReturnLine, "R%d-%d-%s",
			    best_hit.start_character, best_hit.end_character,
			    Selstrout);
		    
		    if (!MacIndex)
					GSsetPath(gs, ReturnLine);
		    else
					GSsetPath(gs, Selstrout);
		    GSsetWeight(gs, best_hit.weight);
		    
         /* 
		     * Find and process sidefile. 
		     * Allow worst case name length. 
		     */
#ifdef CAPFILES
		    if((sidename = (char *) malloc((unsigned) 
		        strlen(Selstrout) + 
	                strlen("/.cap/") + 1)) != NULL) {
		      if((cp = mtm_basename(Selstrout)) != Selstrout) {
			/*  turn "/foo/bar/baz" into "/foo/bar/.cap/baz" */
			strncpy(sidename,Selstrout,(cp - Selstrout));
			*(sidename + (cp - Selstrout)) = '\0';
			strcat(sidename,".cap/");
			strcat(sidename,cp);
		      }
		      else {
		      /* root of the gopher tree, this is easier... */
			strcpy(sidename,"/.cap/");
			strcat(sidename,Selstrout);
		      }
		      if ((SideFile = rfopen(sidename, "r")) != NULL) {
			   Debug("Side file name: %s\n", sidename);
			Process_Side(SideFile, gs);
		      }
		      free(sidename);
		    }
#endif
		    
		    Debug("Doc type is %s\n", best_hit.type);
        if (strcmp(best_hit.type, "GOPHER")==0) {
            Debug("Got a veronica style thing %s\n",best_hit.headline);
            Process_Veronica(&best_hit, gs);
            }
		    
		    if (plusdirs)
			 		GSplustoNet(gs, sockfd, NULL);
		    else
			 		GStoNet(gs,sockfd, GSFORM_G0);

	      }
	
	       Debug("WaisIndexQuery ReturnLine=%s\r\n", ReturnLine);
	       Debug("WaisIndexQuery: End=%d;", best_hit.end_character);
	       Debug("Length=%d;", best_hit.document_length);
	       Debug("lines=%d\r\n", best_hit.number_of_lines);
	     }
     }
     else {
	     /* something went awry in the search */
	     LOGGopher(sockfd, "Something went wrong in the search!");
	     return;
        }

     finished_best_hit(db);
     closeDatabase(db);/* free everything */
     return;
}


EveryWAISdocument(sockfd, db, INDEXHost, INDEXPort, INDEXPath)
  int sockfd;
  char *db;
  char *INDEXHost;
  int  INDEXPort;
  char *INDEXPath;
{
     FILE         *dbcatalog;
     char         db_name[MAXPATHLEN];
     char         inputline[512];
     String       *Headline;
     String       *Filename;
     int          StartByte, EndByte;
     GopherObj    *gs;
     GopherDirObj *gd;
     boolean      Headlineset = FALSE;
     boolean      DocIDset    = FALSE;

     gs = GSnew();
     gd = GDnew(32);
     Headline = STRnew();
     Filename = STRnew();

     strcpy(db_name, db);
     strcat(db_name, ".cat");

     dbcatalog = rfopen(db_name, "r");
     
     while (fgets(inputline, sizeof(inputline), dbcatalog) != NULL) {
	  if (strncmp(inputline, "Headline: ", 10)==0) {
	       STRset(Headline, inputline +10);
	       Headlineset = TRUE;
	  }
	  else if (strncmp(inputline, "DocID: ", 7)==0) {
	       char *cp;

	       StartByte = atoi(inputline);
	       cp = strchr(inputline+7, ' ');
	       if (cp == NULL) break;

	       cp++;
	       EndByte = atoi(cp);

	       cp = strchr(inputline+7, ' ');
	       cp++;
	       if (cp == NULL) break;

	       cp =strstr(cp, INDEXPath);
	       if (cp == NULL) break;
	       
	       STRset(Filename, cp);

	       DocIDset = TRUE;
	  }
	  
	  if (DocIDset == TRUE && Headlineset == TRUE) {
	       char tmppath[512];
	       Extobj *ext = EXnew();

	       if (GDCBlockExtension(Config, STRget(Filename), ext)) {
		    GSsetType(gs, EXgetObjtype(ext));
	       } else
		    GSsetType(gs, '0');
		    
	       EXdestroy(ext);

	       sprintf(tmppath, "R%d-%d-%s", StartByte, EndByte, STRget(Filename));

	       GSsetTitle(gs, STRget(Headline));
	       GSsetHost(gs, INDEXHost);
	       GSsetPort(gs, INDEXPort);
	       GSsetPath(gs, tmppath);

	       GDaddGS(gd, gs);

	       DocIDset = FALSE;
	       Headlineset = FALSE;
	  }
     }	  
}

#endif /** WAISSEARCH **/
