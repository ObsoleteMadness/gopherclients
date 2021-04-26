$! this procedure is to compile the VMS gopher server
$!
$ if ( p1 .eqs. "GCC" )
$  then
$    gcc special
$    gcc gopherstruct
$    gcc error
$    gcc util
$    gcc globals
$    gcc vmsgopherd
$ endif
$!
$ if ( p1 .eqs. "CC" )
$  then
$    cc special
$    cc gopherstruct
$    cc error
$    cc util
$    cc globals
$    cc vmsgopherd
$ endif
$!
$exit
$!Last Modified:   8-MAR-1993 10:39:52.64, By: MARK 
