/* log file name */ 
#define DEBUGLOG "/usr/users/murphy/gopher/gophtech/log"

/* These are TechInfo nodeids that are valid for LOCALTI_SERVER */
#define SOURCES_MSG_NODEID         "9080"
#define LOCAL_SOURCES_NODEID       "876"

/* If there's a local TechInfo server, you'll probably want to
   define it below so that it's main menu will appear by default
   when a gopher client sends just "" */

#define LOCALTI_NAME         "PennInfo"
#define LOCALTI_SERVER       "penninfo-srv.upenn.edu"
#define LOCALTI_PORT         "9000"
#define LOCALTI_MAINMENU     "0"

/* Which host is the keeper of the TechInfo servers list worldwide? */
#define TISERVERS_HOST  "penninfo-srv.upenn.edu"
#define TISERVERS_PORT  "9000"


/*
  LOCAL GOPHER SERVER??? 
  If there is a particular Gopher server that you want
  the gateway to point to, then add it below.
  If you don't have a gopher you want to point at, then
  leave LOCALGOPHTITLE blank.
  */

/*#define LOCALGOPHTITLE "" */

#define LOCALGOPHTITLE       "UPENN Gophers"
#define LOCALGOPHERSERVER    "gopher.upenn.edu" 
#define LOCALGOPHERPORT      "70"
#define LOCALGOPHERPATH      ""

