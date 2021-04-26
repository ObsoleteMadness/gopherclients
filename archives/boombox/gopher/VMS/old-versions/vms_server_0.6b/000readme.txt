Version 0.5
                      A Gopher server for VMS

This is currently a very basic Gopher server for VMS.  In its current version,
it will only return file and directory names and the contents of files.  Links
seem to be working.

What has been done so far?  If one looks at the code for VMS and for a Unix
machine, it should be very similar.  Why?  I took the Unix code only made the
changes I had to to make it run under VMS.  There are probably cleaner ways of
doing many of the things that I have done.  All I was after was to get
something running.  Other capabilities will be added as I learn more about
Gopher.  Essentially what I know about Gopher is what I have picked up from the
doc/server.doc file and the code.  A better understanding of the capabilities
of the unix server would make it easier to make the VMS version do the right
things.

The code was developed using GCC.  The code has been build once with vaxc. The
TCP/IP part is Multinet on our system.  I believe it should also work with
Wollongong since I have seen other software move from one to the other with no
code changes.  I don't know what will happen with UCX or CMU tcp/ip software.

There is a server running on UMMVXM.MRS.UMN.EDU at port 70.  The server there
is the code I am working on.  The source and command procedures for building
the server will be placed on BOOMBOX.MICRO.UMN.EDU and will be available for
anonymous ftp by Jan. 2, 1992.  I guess we can call this version 0.1.  At least
for the time being, I will take care of all source fixes and updates.  They can
be sent to me at mark@caa.mrs.umn.edu.  At some point, I will probably rewrite
some of the code to clean it up and make it more presentable.  To run a server
on a system, use the following command, preferably from SYSTEM:

$ run/detach/proc=<process_name> vmsgopherd.exe

This will create two files in the directory where the run command was executed. 
They are "SYS$ERROR." and "SYS$OUTPUT.".  These can be sent to files with the
appropriate command line qualifiers.  At some point soon, I will do something
to allow the specification of a log and security file.  These might be done
with system logical names.  I'm not real sure at this point.  The GOPHER_DATA
directory is found by a system logical name.  The GOPHER_DATA directory should
probably be set up as a rooted logical pointed to by the logical name
GOPHER_ROOT.
e.g.:
$ assign/system/exec/trans=(conc,term) $1$dua2:[gopher_data.] gopher_root
This will be similar to doing the "chroot" statement in the Unix version.
Enjoy!

Mark Van Overbeke                    Systems Software Programmer
Behmler Hall, Room 10C               (612) 589-6378
E. 4th Street and College Ave.       BITNET:    Mark@UMNMOR.BITNET   (VMS)
University of Minnesota, Morris      INTERNET:  Mark@caa.mrs.umn.edu (VMS)
Morris, MN   56267                              mark@cda.mrs.umn.edu (Ultrix)

------- ------- -------- -------- --------- ---------- --------- ---------
Version 0.6b

Changes from the previous version:

- The server can now be started with a command procedure.  In this procedure
several logical names are defined which the server translates to determine
where the log file goes, if any; where the security file is, if it is to be
used; the internet port number on which to listen so you can have more than one
server running on different ports; where the data is for this server; and the
definition of the root directory for the data.

- The server will now sort the directory listing in alphabetical order and then
send it to the client.  It can also follow an internal ordering scheme set up
by the administrator.

- The server will no longer die if it is fed an incorrect directory
specification to get a directory listing.

- The server executes a command procedure and returns the result to the client
if one of the items in the menu selection is a command procedure.

- Some other general cleaning up has to be done yet also.

- Probably others to be added as I remember them

- When compiling with GCC, in the gnu_cc_include:[vms] directory, modify the
line rmsdef.h in rms.h to be <vms/rmsdef.h> instead of <rmsdef.h>.

- Now support UCX 
