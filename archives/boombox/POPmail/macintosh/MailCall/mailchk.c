#include <stdio.h>
#include <syslog.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#define MAILBUFSIZE  (64)
main(argc, argv)
int argc;
char **argv;
  {
    int i,j;
    char fbp[MAILBUFSIZE];  
    FILE *pfile;
    int fromlen;
    struct sockaddr_in from;
 char *MAILCK ="/bin/grep -c '^From ' /usr/mail/.                                                                                 ";

    
fromlen = sizeof( from );
    if( recvfrom( 0, fbp, MAILBUFSIZE, 0, (struct sockaddr *) &from, &
fromlen ) == -1 )
      {
        exit( 1 );
      }

/* Put user name into command */
/* find the . in the string */
for (i=0; MAILCK[i] !='.';i++);

for (j=0; fbp[j] !='\0';){
	if (isalnum(fbp[j]))	MAILCK[i]=fbp[j];
	i++;
	j++;
}
/* Open a pipe and look in mail file for To: user lines */
/* Each To: line is one mail message. Pretty simple */
 pfile = popen(MAILCK, "r" );
    if( !pfile )
      {
        exit( 1 );
      }
  /*  for (i=0; MAILBUFSIZE;)  */
	for (i=0; i<MAILBUFSIZE;)
      {
        fbp[i] = getc( pfile ); 
      /*  if( fbp[i] == EOF ) */
	if (fbp[i]== '\n')
          {
            fbp[i] = '\0'; 
            break;
          }
                i++;
      }
   if (pclose( pfile ))
	fbp[0]='\0'; 
if (fbp[0]=='\0'){
fbp[0]='0';
fbp[1]='\0';
i=1;
}
      
 sendto( 0,fbp,i,  0, &from, fromlen );
exit(0);
  }
