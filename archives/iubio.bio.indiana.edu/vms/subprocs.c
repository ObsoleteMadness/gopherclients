/*Last Modified:   5-FEB-1992 09:01:05.16, By: MARK */
#include "gopher.h"

#if __GNUC__
#include <sys/ioctl.h>
#include <sys/wait.h>
#endif

#include <signal.h>

#ifdef SIGTSTP    /* True on a BSD system */
#include <sys/file.h>
#include <sys/ioctl.h>
#endif



/* A little signal handler that handles those damn zombies */


sig_child()
{
     /*
      * Use the wait3() system call with the WNOHANG option
      */

     int pid;
/*     union wait  status;  */

/*     while ( (pid = wait3(&status, WNOHANG, (struct rusage *) 0)) > 0)
          ;
*/

}


#ifdef SIGCHLD
#ifndef SIGCLD
#  define SIGCLD SIGCHLD
#endif
#endif


setsighandler() 
{
          
     int sig_child();

#ifdef SIGTSTP
     signal(SIGCLD, sig_child);  /* BSD */
#else
/*     signal(SIGCLD, SIG_IGN);  */
#endif


}
