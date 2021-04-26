/*
 * This is the gopher back end structures.
 * 
 * eventually this will provide a nice pseudo o-o interface to
 * gopher.  It will make writing gopher programs even easier.
 *
 * I expect to use this in version .6 or .7
 */

struct GophObj {
     char Type;
     char Name[256];
     char Selector[256];
     char Host[128];
     int  Port;
};

typedef (struct GophObj *)  GophObj;

struct DirGophObj {
     GophObj GophDir;
     /*
     LinkedList Direntries;
     */
};

typedef (struct DirGophObj *) DirGophObj;

struct TextGophObj {
     GophObj GophText;
     FILE *TheFile;
     char filename[128];
};

typedef (struct TextGophObj *) TextGophObj;

struct SoundGophObj {
     GophObj GophSound;
     int soundsock;
};

typedef (struct SoundGophObj *) SoundGophObj;

     

/*
 * This is how normal programs access GophObjs.
 */

#define GetGophObjType(x) ((x)->Type)
#define GetGophObjName(x) ((x)->Name)
#define GetGophObjPath(x) ((x)->Selector)
#define GetGophObjHost(x) ((x)->Host)
#define GetGophObjPort(x) ((x)->Port)

/*
 * Member functions
 */ 

GopherObj *NewGophObj();

