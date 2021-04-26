From:	IN%"sherman@trln.lib.unc.edu"  "Dennis R. Sherman" 12-MAR-1993 11:48:21.17
To:	IN%"Mark@CAA.MRS.UMN.EDU"
CC:	
Subj:	compiling gopher for VMS

Return-path: <sherman@trln.lib.unc.edu>
Received: from trln.lib.unc.edu by CAA.MRS.UMN.EDU (PMDF #2574 ) id
 <01GVPX4U6SSG95NDHE@CAA.MRS.UMN.EDU>; Fri, 12 Mar 1993 11:47:46 CST
Received: by trln.lib.unc.edu (MX V3.2-beta) id 4689; Fri,
 12 Mar 1993 12:47:30 EST
Date: 12 Mar 1993 12:47:28 -0500 (EST)
From: "Dennis R. Sherman" <sherman@trln.lib.unc.edu>
Subject: compiling gopher for VMS
To: Mark@CAA.MRS.UMN.EDU
Message-id: <0096965C.F4125C40.4689@trln.lib.unc.edu>
X-Envelope-to: Mark
Content-transfer-encoding: 7BIT

I'm including a short .com file below you might want to add to the VMS 
client distribution.  It'll make it a bit clearer that there are 
switches needed for different TCP/IP implementations, and make it 
easier to use them.  

Looks like it works fine on our system, in case you're keeping track. 
VMS 5.4-3, UCX 1.3

-----------------cut here---------------------
$! compile.com
$!  19930312	D.Sherman	dennis_sherman@unc.edu
$!	added parameter handling to specify TCP/IP flavor
$!
$ if (P1 .eqs. "" ) 
$ then
$	write sys$output "Usage: $ @compile <tcpip>"
$	write sys$output "  where <tcpip> is one of: "
$	write sys$output "    MULTINET"
$	write sys$output "    UCX"
$	write sys$output "    WOOLONGONG"
$	exit
$ else
$ 	P1 = f$edit( P1, "UPCASE" )
$ endif
$ cc/define=('P1'=1) calendar
$ cc/define=('P1'=1) cso
$ cc/define=('P1'=1) error
$ cc/define=('P1'=1) globals
$ cc/define=('P1'=1) gopher
$ cc/define=('P1'=1) gopherstruct
$ cc/define=('P1'=1) manager
$ cc/define=('P1'=1) ourutils
$ cc/define=('P1'=1) subprocs
$ cc/define=('P1'=1) util
$ cc/define=('P1'=1) v2stuff
$!
$exit
---------------------cut here -------------

*--------------------------------------------------------------------*
* Dennis R. Sherman  	         Triangle Research Libraries Network *
* dennis_sherman@unc.edu       Univ. of North Carolina - Chapel Hill *
*--------------------------------------------------------------------*
