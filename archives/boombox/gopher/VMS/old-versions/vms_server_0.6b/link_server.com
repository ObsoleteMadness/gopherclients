$ if ( p1 .eqs. "GCC" ) 
$  then if (p2 .eqs. "DEBUG")
$       THEN 
$          link/debug vmsgopherd,special,gopherstruct,error,globals,util,-
                      multinetopt/opt,gnu_cc:[000000]gcclib/lib
$       else
$           link vmsgopherd,special,gopherstruct,error,globals,util,-
            multinetopt/opt,gnu_cc:[000000]gcclib/lib
$       ENDIF
$ endif
$!
$ if ( p1 .eqs. "CC" )
$  then IF (P2 .EQS. "DEBUG")
$           THEN
$             link/debug vmsgopherd,special,gopherstruct,error,globals,-
		util,crtl/opt
$           else
$             link vmsgopherd,special,gopherstruct,error,globals,util,-
		multinetopt/opt
$        ENDIF
$ endif
$!
$exit
$!Last Modified:  31-JUL-1992 10:59:02.85, By: MARK 
