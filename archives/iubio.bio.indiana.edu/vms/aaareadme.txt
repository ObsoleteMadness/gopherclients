         A VMS Gopher client

Here is a first pass at a VMS Gopher client.  Some changes will need to be made
in the conf.h file.  The default host needs to be set to your local or closest
gopher server.  The default port needs to be set to the port number of your
default gopher server.  

The VMS Gopher client was developed with Version 2.2 of the Multinet TCP/IP
package.  It was compiled with VAX C.  There are some problems that I have not
tried to work out building with GCC.  There are a number of errors that are
reported during the link process.  These errors can be safely ignored for now.  
You will get a usable client.

The VMS Gopher client looks at the command line to see if you have specified a
host and port number.  If you have, it will use this host and port number
rather than the default host and port.  It will not allow you to specify only a
host name or a port number.  Both must be specified on the command line or
nothing should be specified on the command line.

The VMS Gopher sources can be found on boombox.micro.umn.edu in the directory
/pub/gopher/VMS/vms_client_v0.6.  To build the client, use compile.com and then
link with link_client.com.  Then create a symbol such as

 gopher :== $disk:[dir]gopher.exe

to use to invoke the client.  Replace disk and dir with the disk name and
directory name where the gopher image will reside.  Mine looks like this:

 gopher :== $umm$system:gopher.exe

I have tested most things and it seems to run ok.  If any problems are found,
please report them to me at the addresses given in the signature at the end.
Happy Gophering!!
---------------------------------------------------------------------------

new fixes and changes.......
10-Mar-1992
1) The client now returns help when you press the '?'.

2) To move up the gopher heirarchy, you can now use the <- (left arrow) as well
as 'u'.  You can also use -> (right arrow) to view the current item.

3) Stack.c has been removed.  None of the procedures in stack.c were called
from anywhere in the code.

7-APR-1992
1) The client no longer deletes the help file when run from a privileged
account. (boy, was that dumb!)

2) A link options file was added for running with UCX.  #ifdef statements were
added for net calls that needed to be modified for UCX.

30-APR-1992
1) No luck with UCX yet.

2) fixed a problem with bolding an item that was found in a search when the
file was displayed to the screen.  We used to get a stack dump.  Now things
appear to work correctly.

3) All link errors have hopefully been removed.

4) There are still problems with the ftp searches.  The client goes into an
infinite loop.
-----------------------------------------------------------------------------

Mark Van Overbeke                    Systems Software Programmer
Behmler Hall, Room 10C               (612) 589-6378
E. 4th Street and College Ave.       BITNET:    Mark@UMNMOR.BITNET   (VMS)
University of Minnesota, Morris      INTERNET:  Mark@caa.mrs.umn.edu (VMS)
Morris, MN   56267                              mark@cda.mrs.umn.edu (Ultrix)
