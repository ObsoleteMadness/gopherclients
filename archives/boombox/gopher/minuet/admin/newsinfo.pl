#!/usr/local/bin/perl
#
#
#       NewsInfo server
#
#       (C) 1993 University of Minnesota
#
#       Version 0.9
#
###############
#
#   "NewsInfo" provides compacted NNTP server information to
#   Minuet clients.
#
#    George R. Gonzalez
#
#    grg@boombox.micro.umn.edu
#
####
#
#     To install:
#     choose a directory for it  (/usr/local/bin/newsinfo is our choice)
#     change the directories below to your liking.
#     change the name of the news server to be your local news server.
#     copy this file to the dir
#     put "perl /thedir/newsinfo.pl &" in some system startup script
#
#     no need to add lines to "services" or "inetd.conf"
#
#     this script runs a server without the need of either one.
#
#
#     Questions, brickbats to minuet@boombox.micro.umn.edu
#
#
##########

###############
#
#   Site configurable parameters:
#
#
$OurDir      = "/usr/local/bin/newsinfo";     # our home base
$nntp_server = "news.cis.umn.edu";            # the suggested NNTP news server
$nntp_port   = 119;                           # the port it runs on
$maxage      = 1.0;                           # How often to refresh info (in days)

###############
#
#    optional configuration:
#
#    Most sites can live with these choices

$LogDir      = "$OurDir";                     # where the log files go
$LogFileName = "$LogDir/log";                 # our log file
$errors      = "$LogDir/errors";              # log of groups with errors
$zeroes      = "$LogDir/zeroes";              # log of groups that are empty


$version = "0.9";
$askserver = 1;
$our_port  = 7119;     # Temporary port number, will change to permanent someday
$using_inetd = 0;
$EINTR = 4;

$nntp_groups = "$OurDir/all.groups";   # our main data file
$nntp_temp   = "$OurDir/temp.groups";  # the one being built

#
$separator = "\001";

$eol = "\r\n";
@okcmds = ( "help", "quit", "where", "dir", "find" );



sub Gabort {
	&Log( "Server exiting, reason is $_[0]" );
	exit;
}

sub GopenServer {

	local($server,$port) = @_;
	$sockaddr = 'S n a4 x8';
	(($name, $aliases, $type, $len, $saddr) = gethostbyname($server))
	    || &Gabort("Can't get address of: $server");
	$sin = pack($sockaddr, 2, $port, $saddr);
	socket(GSERVER, 2, 1, 0) || &Gabort("Can't create socket: $!");
	connect(GSERVER, $sin)   || &Gabort("Can't connect to server: $!");
	select(GSERVER);
	$| = 1;
}



sub GcloseServer {
	close(GSERVER);
}



sub Gsend {
	print "send -> |$_[0]|\n" if (defined($Gdebug));
	print GSERVER "$_[0]$eol";
}

sub Grecv {
	local ($_);
	$_= <GSERVER>;
	s/\n$//;
	s/\r$//;
	print "recv -> |$_|\n" if (defined($Gdebug));
	return $_;
}

sub OutToNet {
   print $NETOUT @_[0];
}


sub fileok {
   local( $ok );
   $ok =  -f @_[0];
   if( ! $ok ) {
	&flush(1);
	&OutToNet( "-invalid command, file @_[0] not available.$eol" ); 
   }
   return( $ok );
}


sub parsenums {

	local($_);

	&Gsend( "GROUP " . $gname );
	$_ = &Grecv;
	$errline = $_;
	($stat,$lo,$hi,$num) = /(\d+)\s+(\d+)\s+(\d+)\s+(\d+)/;
}




sub massage {

	$gname =~ s/\./$separator/g;
	$desc = $_;
	$desc =~ s/^\S+\s+//;
	$desc =~ s/^\?+$//;
	if( $desc eq "" ) {
		$desc = "?";
	}
}



sub serverstuff {
	$ok = 0;
	&parsenums;
	if    ( $stat != 211 )
	{
		$badstatus++;
		print ERR "$gname gave error $errline\n";
	}
	else {
		if ( $lo == 0 || $hi == 0 || $num == 0 )
		{
			$zeroval++;
			print ZER "Group $gname has zero messages\n";
		}
		else
		{
			&massage;
			$ok = 1;
		}
	}
}



sub getgroupinfo {

	&Log( "getting groups...");
        $enterwall = &JulianSeconds;
	&GopenServer( $nntp_server, $nntp_port );
	$a = &Grecv;
	&Gsend( "LIST NEWSGROUPS" );

	$_ = &Grecv;
	&Gabort($_) if !/^215/;

	@groups = ();
	while ( <GSERVER> ) {
		chop;
		chop;
		last if /^\.$/;
		push( @groups, $_ );
	}

	&Log( "now looking up each one..." );


	open( ERR, ">$errors" )||die "help me with errors!";
	open( ZER, ">$zeroes" )||die "help me with zeroes";

	@outlist = ();
	$inx = 0;
	$tot = 0;
	$badstatus = 0;
	$zeroval = 0;
	foreach( @groups ) {
		@res = split (' ', $_ );
		$gname = shift( @res );
		$inx++;
		$tot++;
		if( $askserver ) { 
			&serverstuff; 
		}
		else { 
			&massage; 
			$ok = 1; 
		}
		if( $ok ) { 
			push( @outlist, $gname . $separator . $desc ); 
		}
	}

	&Log( "all looked up, now sorting..." );
	close( ERR );
	close( ZER );
	@groups = ();
	@sorted = sort( @outlist );
	&Log( "Sorted, now writing file" );
	@outlist = ();
	&Gsend( "QUIT" );
	&GcloseServer;

	open( GROUPS, ">$nntp_temp" );
	foreach( @sorted ) {
		print GROUPS $_,"\n";
	}
	close GROUPS;
        &maketop;

	rename( $nntp_temp, $nntp_groups ) || die "cant rename $nntp_temp to $nntp_groups$eol";

	&Log( "Total groups in incoming list:   $tot" );
	&Log( "Groups that dont exist:          $badstatus" );
	&Log( "Zero size groups:                $zeroval" );
        $exitwall = &JulianSeconds;
        $elapsed = $exitwall - $enterwall;
	&Log( "Wall clock time elapsed:         $elapsed seconds." );
}




sub loadgroups{
	open( G, "$nntp_groups" );
	@sorted = ();
	while( <G> ) {
		chop;
		push( @sorted, $_ ) 				};
	close G;
}


sub maketop {
	open( TOP, ">$OurDir/top.cache" ) || die "cant create top cache file";
	open( INX, ">$OurDir/index" ) || die "cant create index file";
	$lastg = "?";
	$pos = 0;
	&flush(1);
	foreach( @sorted ) {
		@gps = split( $separator, $_ );
		$g = @gps[0];
		if( $g ne $lastg ) {
			$numflds = @gps;
			print TOP "$g";
			if ($numflds > 2 ) { 
				print TOP "."; 
			};
			print TOP "\n";
			$lastg = $g;
			print INX "$g $pos\n";
		}
		$pos += length($_) + 1;
	}
	close TOP;
	close INX;
}


sub loadorget {
	if( -e $nntp_groups ) {
		&loadgroups;
	}
	else
	{
		&getgroupinfo;
	}

}


#####################
#
#


sub CMD_find {
	local( $limit ) = @cmds[1];
        local( $pat ) = @cmds[2];

	if( &fileok( "$nntp_groups" )  )  {
	open( A, "$nntp_groups" )||die "help me with groups!";

	@list = eval "grep ( /$pat/io, <A> ); ";
	$sent = 0;
	foreach( @list ) {
		@f = split( $separator, $_ );
		$c = @f;
		$out = "";
		for( $i = 0; $i < $c-1; $i++ ) {
			$out = $out . @f[$i];
			if( $i < $c-2 ) {
				$out = $out .  ".";
			}
		}
		$d = @f[ $c - 1 ];
		chop( $d );
		$out = "$out|$d$eol";
		&OutToNet( $out );
		$sent += length($out);
		last if $sent >= $limit;
	}

	close( A );
	&SendEOM;
}
}




sub CMD_dir {
	local($_);

	$numargs = @cmds;
	if( $numargs == 1 ) { 
		&SendTop; 
	} else
	{
		if( &fileok( "$nntp_groups" ) ) {
		open( F, $nntp_groups );
		$s = @cmds[1];
		&Log( "Searching for /$s/" );
		$s =~ s/\./$separator/;
		$oldf = "?";
		@grp = split( $separator, $s );
		$firstgrp = @grp[0];
		$place = $index{ $firstgrp };
		seek( F, $place, 0 );
		while( <F> ) {
			chop;
			$line = $_;
			last if ! /^$firstgrp/;
			if( /^$s/ ) {
				$line =~ s/^$s//;
				$line =~ s/^$separator//;
				@flds = split( $separator, $line );
				$num = @flds;
				if($num == 1 ) {
					# ignore duplicate upper group
				}
else
				{
					$f = @flds[0];
					if( $f ne $oldf ) {
						&OutToNet(  "$f" );
						if( @flds[2] ne "" ) {
							&OutToNet(  "." );
							$oldf = $f;
						}
						&OutToNet(  "|" );
						$desc = @flds[$num-1];
						if( $desc ne "?" ) {
							&OutToNet(  $desc );
						};
						&OutToNet(  "$eol" );
					}
				}
			}
		}
		close( F );
		&SendEOM;
	}
	}
}



sub CMD_where {

	&flush(1);
	&OutToNet(  "+ $nntp_server $nntp_port$eol" );

}


sub CMD_help {
	local( $_ );
	&OutToNet(  "HELP:$eol Commands are:$eol" );
	foreach( @okcmds ) {
		&OutToNet(  "$_$eol" );
	}
	&SendEOM;
}

sub CMD_quit {
	&OutToNet(  "+ Bye for now...$eol" );
	$done = 1;
}

sub SendTop {
	local( $_);
	if( &fileok( "$OurDir/top.cache" ) ) {
        open( TOP, "$OurDir/top.cache" ) || die "cant open top cache";
	while( <TOP> ) {
		chop;
		&OutToNet(  "$_$eol" ); 
	}
	&SendEOM;
	close( TOP );
        }
}

sub flush {
	local( $mode ) = @_[0];
	select( $NETOUT ); 
	$| = $mode; 
        select( STDOUT );
}

sub SendEOM{
	&flush(1);
	&OutToNet(  ".$eol" );
}

sub GetInput {
	$_ = <$NETIN>;
	&flush(0);
	return $_;
}


sub server {
	$NETOUT = @_[1];
	$NETIN = @_[0];
	&flush(1);
	&OutToNet(  "+ Welcome to the NewsInfo server - version $version$eol" );
	$done = 0;
	while ( &GetInput  ) {
		&readindex;
		tr/A-Z/a-z/;
		@cmds = split( ' ', $_ );
		$cmd = @cmds[0];
		&Log( "Command: $cmd" );
		$ok = 0;
		foreach( @okcmds ) {
			if( $_ eq $cmd ) {
				eval( "&CMD_$cmd" );
				$ok = 1;
			};
		}
		last if $done;
		if( $ok != 1 ) {
			&OutToNet(  "- Invalid command$eol" );
			&Log( "Invalid command: /$cmd/" );
		}
	}
}




sub refresh {
        if( -e $nntp_temp )  # refresher is busy.. don't run again
		{ $fileok = 1; }
	else	{
		if( -e $nntp_groups )
			 { $fileok = 0; $age = -C $nntp_groups; }
		else
			 { $fileok = 0; $age = 9999999; } 
		}
	if( ! $fileok )
		{
			 if( $age > $maxage )
				{
					if( fork() == 0 ) 
						{ &Log( "refreshing grp file" );
						  &getgroupinfo;
						  exit(0);
     						}
				}
		}
}


sub JulianSeconds {
  return( time );
}
 

sub Log{
	local( $line ) = @_[0];
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdist) = localtime;
	$now = "$hour:$min:$sec";
	$who = "PID $$";
	print LOG "$now $who: $line\n";
}


sub openlog {

	$fn = "$LogFileName";
	if( -e $fn ) { 
		$size = -s $fn; 
	} else { 
		$size = 999999999; 	};
	if( $size > 1000000 ) { 
		$mode = ">"; 
	} else { 
		$mode = ">>"; 
	}
	$fname = "$mode$fn";
	open( LOG, $fname ) || die "Died:  Opening log file $fname " ;
	select(LOG);
	$| = 1; 
        select( STDOUT );
	&Log( "-------------------------------" );
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdist) = localtime;
	$time = "$mday/$mon/$year $hour:$min:$sec ";
	&Log( "NewsInfo server started at $time" );

}

sub f4{
	local( $param ) = @_;

	$sub = substr( $param, 1, 5 );
	return $sub;
}

sub closelog {

	($user, $sys ) = times;
	$utime = &f4($user);
	$stime = &f4($sys);
	&Log( "Used $utime CPU seconds and $stime system seconds" );
	close( LOG );

}


sub ReadTheIndex {
        &Log( "Reread index, old was $indexage, latest is $nowindexage.\n" );
        $indexage = $nowindexage;
        &flush(1);
        open( INX, "$OurDir/index" );

        while( <INX> ) {
                ( $g, $place ) = /(\S+)\s+(\d+)/;
                $index{ $g } = $place;
        }
        close INX;
}



sub readindex{
	$nowindexage = -C "$OurDir/index";
	if( $nowindexage != $indexage )
		{ &ReadTheIndex };
}



sub termhandler {

&Log( "Server killed, exiting" );
&Log( "---------------------------------" );
exit(1);
}


sub childhandler {
   &Log("Child @_[0] died" );
   wait;
}



sub TrueServer {


$SIG{ 'TERM' } = 'termhandler';
$SIG{ 'CHLD' } = 'childhandler';
$AF_INET = 2;
$SOCK_STREAM = 1;

$sockaddr = 'S n a4 x8';

($name, $aliases, $proto) = getprotobyname('tcp');
$this = pack($sockaddr, $AF_INET, $our_port, "\0\0\0\0" );

select(NS); $| = 1; select( STDOUT );

socket( S, $AF_INET, $SOCK_STREAM, $proto) || die "bad socket: $!";
bind(S, $this) || die "Bad bind: $!";

listen(S,5) || die "listen failed: $!";

select(S); $| = 1; select( STDOUT );

$con = 0;
&Log( "NewsInfo server starting" );
&Log( "Listening on port $our_port" );



for(;;) {
   $noconn = 1;
   while( $noconn ) {
      $addr = accept( NS, S );
      if( $addr )
           { $noconn = 0; }
      else {
	     $noconn = 1;
	     $accerr = $! + 0;
	     if( $accerr != $EINTR ) { die "accept failed: $!";}
           }
   }
   $con++;
   &readindex;
   $pid = fork();
   if( $pid != 0 )
	{
		close( NS );
		$child[ $con ] = $pid;
		&refresh;
     	}
   else
	{
     		($af, $our_port, $inetaddr)  = unpack( $sockaddr, $addr );
     		@inetaddr = unpack( "C4", $inetaddr );
     		&Log( "Connection from: @inetaddr" );
     		&server( NS, NS );
		close( NS );
     		exit;
	}
}
}




sub main {
	&openlog;
	if( $using_inetd )
           	{ &server( STDIN, STDOUT ); } 
	else 
		{ &TrueServer };
	&closelog;
}


&main;


