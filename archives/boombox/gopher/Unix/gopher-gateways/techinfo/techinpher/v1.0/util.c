#include <string.h>
char *domalloc(unsigned long bytes);
void parsefields(char *str, char **fields, int *numfields, char delim, int max);

unsigned long	_allocated = 0;

/* parsefields() changes delim and end of line into '\0' ,
   putting the args into array of char pointers */

void
parsefields (char *str, char **fields, int *numfields, char delim, int max)
{
  char *cp;
  
  cp = index (str, '\r');
  if (!cp) cp = index (str, '\n');	
  if (cp) *cp = 0;

  if (strlen(str) < 1) {
    *numfields = 0;
    return;
  }
  
  fields[0] = str;
  for (*numfields = 1; *numfields < max ; ) {
      fields[*numfields] = index (fields[(*numfields)-1], delim);
      if (fields[*numfields]) {
	*(fields[*numfields]) = 0;      /* wipe out delim */
	fields[*numfields] += 1;        /* move one char past delim */
	(*numfields)++;
      }
      else 
	break;
    }
}


char *
domalloc(unsigned long	bytes)
{
  char *cp;
  
  if (bytes == 0) 
    bytes = 1;

  cp = (char *) malloc(bytes);
  
  _allocated += bytes;
  if (!cp)
    printf("Malloc failed to get %lu bytes.\n", bytes);
  return cp;
}


/**************************************************************************/
/* Count # of occurences of char c in string str */
int
strcnt(char *str, char c)
{
  register int	cnt = 0;
  
  while (*str)
    if (*str++ == c)
      cnt++;
  return cnt;
}

/**************************************************************************/
/* tolower a string, returning the length of the string */
int strlower(char *str)
{
  int	cnt = 0;
  
  while (*str) {
    if (isupper(*str))
      *str = tolower(*str);
    str++, cnt++;
  }
  return cnt;
}


substi_char (char *istr, char *ostr, char ch_frm, char ch_to)
/* Convert ch_frm into ch_to where ever it occurs in istr, write
   results in ostr */
{
  char *cp;
  
  strcpy (ostr, istr);  /* copy istr to ostr */
  for (cp = ostr; *cp; cp++) {
    if (*cp == ch_frm)
      *cp = ch_to;
  }
}
