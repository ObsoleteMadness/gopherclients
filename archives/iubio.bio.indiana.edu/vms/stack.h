struct g_Stack {
     char **data; /* A pointer to an array of pointers. */
     int  Sptr;   /* Current stack pointer */
     int  Max;    /* Maximum size of the stack */
};

typedef struct g_Stack Stack;

/*** Access functions ***/

#define STgetItem(st,x)  (*(st->data))[x]  /** Does no bound checking! **/
#define STgetMax(st)     (st->Max)
#define STgetPtr(st)     (st->Sptr)

/*** Modification functions. ***/
#define STsetItem(st,x,y)   ((*(st->data))[x]=(y))
#define STsetItemptr(st,x)  (st->data=((char*)x))
#define STsetMax(st,x)      (st->Max=(x))
#define STsetPtr(st,x)      (st->Sptr=(x))

/*
 * Functions defined in stack.c
 */

int STnew();
int STpop();
int STpush();

