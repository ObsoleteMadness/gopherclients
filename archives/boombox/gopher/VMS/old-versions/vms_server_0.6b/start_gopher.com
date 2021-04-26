$! This command procedure will start a Gopher server.  This will have to do
$! until the server is fixed to start from inetd, or multinet_server, or 
$! whatever you want to call the network process.
$!
$! @start_gopher p1 p2 p3 p4 p5
$!
$! Parameters:
$!    p1 : data directory root declaration
$!    p2 : root directory
$!    p3 : port
$!    p4 : logfile
$!    p5 : security file
$!
$!    example:
$! @start_gopher $1$dua2:[gopher_data.] gopher_root:[000000] 70 -
$!     sys$manager:gopher.log sys$manager:secur.
$!
$! if Data_Directory_Def is not defined, we will assume there is system logical
$! name defined.  We will then translate that logical name and place it into
$! the job logical name table of the server.  We want to have each server
$! have a private definition for the data directory so that more than one
$! server can reside on a machine.  This way, the data for each sever can be
$! in very different locations and neither will know about the data that the
$! other is serving.
$!
$ If ( p1 .nes. "" )
$  Then
$   Assign/system/exec 'p1 Data_Directory_Def
$ Endif
$!
$ If ( p2 .nes. "" )
$  Then
$   Assign/system/exec 'p2 Gopher_DataDir
$  Else
$   Assign/system/exec Gopher_root:[000000] Gopher_DataDir
$ Endif
$!
$ If ( p3 .nes. "" )
$  Then
$   Assign/system/exec 'p3 Gopher_TCPPort
$  Else
$   Assign/system/exec 70 Gopher_TCPPort
$ Endif
$!
$ If ( p4 .nes. "" )
$  Then
$   Assign/system/exec 'p4 Gopher_LogFile
$  Else
$   Assign/system/exec UMM$Manager:Gopher.log Gopher_LogFile
$ Endif
$!
$ If ( p5 .nes. "" )
$  Then
$   Assign/system/exec 'p5 Gopher_SecFile
$  Else
$   Assign/system/exec UMM$Manager:Secur. Gopher_SecFile
$ Endif
$!
$ run/detach/process_name=gopher1_server/working_set=500/extent=2500/-
    maximum_working_set=1500/-
    output=sys$manager:gopher.output/-
    error=sys$manager:gopher.error sys$system:vmsgopherd.exe
$!
$ exit
$!Last Modified:  20-MAR-1992 09:10:53.53, By: MARK 
