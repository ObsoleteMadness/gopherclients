 
README for MVS Gopher Client and Server, Version 3, Release 1
 
January 16, 1994
 
Instructions:
 
 FTP the appropriate distribution file to your MVS system first.
 
 If you have a C compiler, then use.... gopher.ggmvs.distrib.cntl.v3r1
 If you don't have a C compiler, use... gopher.ggmvs.distobj.cntl.v3r1
 
 Use something like the following:
 
 To FTP gopher.ggmvs.distrib.cntl.v3r1 (source distribution):
 
  ftp boombox.micro.umn.edu
  anonymous
  password@myhost.mydomain
  cd /pub/gopher/mvs
  get gopher.ggmvs.distrib.cntl.v3r1 GOPHER.DISTRIB.CNTL
  quit
 
 To FTP gopher.ggmvs.distobj.cntl.v3r1 (object distribution):
 
  ftp boombox.micro.umn.edu
  anonymous
  password@myhost.mydomain
  cd /pub/gopher/mvs
  binary 
  locsite recfm=fb lrecl=80 blksize=6160
  get gopher.ggmvs.distobj.cntl.v3r1 GOPHER.DISTOBJ.CNTL
  quit
 
 Please note that the *source* distribution is stored as a
 RECFM=VB LRECL=259 file, to save space and speed up file transfer.
 The *object* distribution is stored as RECFM=FB LRECL=80 because
 there's no other way to store combination EBCDIC and object text
 in a binary file.
 
 Either of these is one large JCL stream that executes the IEBUPDTE
 utility to build several PDS's.  These PDS's are:
 
 GOPHER.INSTALL.CNTL    JCL to complete the build, readmes, miscellany
 GOPHER.INSTALL.CLIST   CLISTs and REXX execs for client and server
 GOPHER.INSTALL.PANEL   ISPF panels for the client
 GOPHER.INSTALL.ABOUT   The "About This Gopher" text library
 GOPHER.INSTALL.OBJ     Object modules *** OBJECT DISTRIBUTION ONLY ***
 GOPHER.INSTALL.H       C headers      *** SOURCE DISTRIBUTION ONLY ***
 GOPHER.INSTALL.C       C source       *** SOURCE DISTRIBUTION ONLY ***
 
 You can modify the job to generate a different prefix for the names
 if you don't want to call them GOPHER.INSTALL.*.  You'll need to
 provide your own JOB card, too.  Other than that, the JCL is ready
 to run (from ISPF EDIT, anyway).  If you run into problems submitting
 a RECFM=VB file, copy the file over into a RECFM=FB LRECL=80 data set
 and then try it.
 
 Once you've done this, proceed to the $$README file in the .CNTL PDS,
 which you can read once the first stage of the unload is done.  This
 will tell you the rest of the steps to finish building Gopher.
 
 You may install just the client, just the server, or both the
 client and the server.  It is purely up to what your needs are.
 
Legal stuff:
 
------------------------------------------------------------------------
 
 Copyright (c) The Charles Stark Draper Laboratory, Inc.,1992,1993,1994
 
 This software is provided on an "AS IS" basis.  All warranties,
 including the implied warranties of merchantability and fitness,
 are expressly denied.
 
 Provided this copyright notice is included, this software may
 be freely distributed and not offered for sale.
 
 Changes or modifications may be made and used only by the maker
 of same, and not further distributed.  Such modifications should
 be mailed to the author for consideration for addition to the
 software and incorporation in subsequent releases.
 
------------------------------------------------------------------------
 
 Questions?  Comments?  Suggestions?  Gripes?  Please email to...
 
 Steve Bacher      <seb@draper.com>
 
