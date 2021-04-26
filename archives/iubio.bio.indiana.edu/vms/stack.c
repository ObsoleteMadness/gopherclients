/*
 * Try to implement a stack-object
 */
#include "gopher.h"

int 
STnew(st, max)
  Stack *st;
  int max;
{
     char *space;

     space = malloc(sizeof(char*) * max);
     /** allocated some pointers **/
     STsetItemptr(st, space);
     STsetMax(st, max);
     STsetPtr(st, 0);
}


void
STdestroy(st)
  Stack *st;
{
     char *moo;

     moo = STgetItem(st, 0);

     if (moo != NULL) {
	  free(moo);
     }
}


int
STpop(st, datum)
  Stack *st;
  char *datum;
{
     int level;

     level = STgetPtr(st);

     if (level ==0)
	  return(-1);
     else
	  level--;

     STsetPtr(st, level);

     datum = STgetItem(st, level);
     
     return(0);
}

int 
STpush(st, ptr)
  Stack *st;
  char *ptr;
{
     int level;

     level = STgetPtr(st);

     if (STgetMax(st) <= level)
	  return(-1);

     /* Add the pointer to the list */
     STsetItem(st, STgetPtr(st), ptr);

     /* Increase the level */
     level++;
     STsetPtr(st, level);
}
