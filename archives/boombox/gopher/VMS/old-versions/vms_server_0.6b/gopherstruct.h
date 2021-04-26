/** Our Gopher Object Type **/

struct g_struct
{
     char    sFileType;     /* The type of object (A_FILE, A_CSO, etc)*/
     char    sTitle[256];   /* User-displayed title of object         */
     char    sPath[256];    /* Internal pathname to object on host    */
     char    sHost[100];    /* Internet name of host                  */
     int     iPort;         /* Port number on host                    */
     int     iItemnum;      /* The number of the item in the directory*/
};

typedef struct g_struct GopherStruct;
typedef struct g_struct GopherObj;

#define GSgetType(a) ((a)->sFileType)
#define GSsetType(a,b) (a)->sFileType=(b)

#define GSgetTitle(a) ((a)->sTitle)
#define GSsetTitle(a,b) strncpy((a)->sTitle,(b),255)

#define GSgetPath(a) ((a)->sPath)
#define GSsetPath(a,b) strncpy((a)->sPath,(b),255)

#define GSgetHost(a) ((a)->sHost)
#define GSsetHost(a,b) strncpy((a)->sHost,(b),99)

#define GSgetPort(a) ((a)->iPort)
#define GSsetPort(a,b) (a)->iPort=(b)

#define GSgetNum(a) ((a)->iItemnum)
#define GSsetNum(a,b) (a)->iItemnum=(b)


/*** Real live functions defined in gopherstruct.c ***/

void GSinit();
void GStoNet();


/****************************************************************
** A Gopher directory structure  You don't want to have too many
** of these defined in a program (they're biiiiiiggggg!)
**
*****************************************************************/

#define MAXGOPHERS 256


struct g_dir_struct {
     GopherStruct Gophers[MAXGOPHERS];
     
     int Top;
};

typedef struct g_dir_struct GopherDirObj;
typedef struct g_dir_struct *GopherDirObjp;

#define GDgetEntry(a,b)  (&((a)->Gophers[b]))
#define GDgetTop(a)  ((a)->Top)
#define GDsetTop(a,b)  ((a)->Top = (b))

/*** Real live functions declared in gopherstruct.c ***/

void GDaddGS();
void GDsort();
