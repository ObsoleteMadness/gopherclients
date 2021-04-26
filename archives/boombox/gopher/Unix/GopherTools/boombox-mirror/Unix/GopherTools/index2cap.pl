Article 4376 of comp.infosystems.gopher:
Xref: feenix.metronet.com comp.infosystems.gopher:4376 comp.archives.admin:237
Path: feenix.metronet.com!news.utdallas.edu!tamsun.tamu.edu!cs.utexas.edu!math.ohio-state.edu!darwin.sura.net!news-feed-2.peachnet.edu!concert!samba.oit.unc.edu!sunSITE!jem
From: jem@sunSITE.unc.edu (Jonathan Magid)
Newsgroups: comp.infosystems.gopher,comp.archives.admin
Subject: new tool for FTP and gopher archives
Followup-To: comp.archives.admin
Date: 11 Aug 1993 16:26:59 GMT
Organization: University of North Carolina, Chapel Hill
Lines: 36
Distribution: world
Message-ID: <24b6kj$sgc@samba.oit.unc.edu>
NNTP-Posting-Host: calypso.oit.unc.edu

I've just put the first publically available version of Index2cap up for FTP.
Index2cap is a Perl script which will rewrite FTP area index files into
.cap files suitable for gopher.  From the README:

-------

index2cap v 0.2.1  July 6, 1993 
(the most current release of this program is available on sunsite.unc.edu in
/pub/packages/gopher/index2cap)

index2cap is a way of making an archive that is accessed by both FTP and
gopher easier to maintain. It starts at the beginning of an archive (or part of
an archive) searching for index files in the form:

foo.tar.Z  a useful utility that does bar

and then rewriting them into .cap entries with a "Name" field which gopherd 
uses in replace of the file name.  so if the index file was in /pub/shoop, 
index2cap would create a file /pub/shoop/.cap/foo.tar.Z, which would contain

Name= foo.tar.Z a useful utility that does bar

index2cap will create the .cap directory if it doesn't exist.


index2cap was written to be run from a nightly crontab; probably the easiest
way of using it is to write a special configuration file that describes your
index file (see the man page) and then let it run automatically.

I hope this is useful, but make no guarrantees.  Please send any patches
or bug-fixes to me to be included in a future release.

enjoy,
jem.
--
jem@sunsite.unc.edu\/sunSITE admin


