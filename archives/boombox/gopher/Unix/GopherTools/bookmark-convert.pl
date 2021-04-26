From: me30400@maroon.tc.umn.edu
Subject: perl bookmark conversion

------------------------------<cut here>------------------------------------
#!/usr/local/bin/perl
#
# bookmark-convert.perl
# v.1
#

print "1) UNIX .gopherrc  -> Mac Turbogopher\n";
print "2) Mac Turbogopher -> UNIX .gopherrc\n";
print "Operation: ";
chop($op = <STDIN>);

($op == 1) && &tomac;
($op == 2) && &tounix;

sub openfiles {
    print "Input Filename [$in]: ";
    chop($reply = <STDIN>);
    if ($reply ne "") {
        $in = $reply;
    }
    open(IN, "$in") || die "$!\n";
    
    print "Output Filename [$out]: ";
    chop($reply = <STDIN>);
    if ($reply ne "") {
        $out = $reply;
    }
    open(OUT, ">$out") || die "$!\n";
}

sub tomac {
    $in = ".gopherrc";
    $out = "mac.bookmarks";
    &openfiles;
    
    while(<IN>) {
        if (1 .. /^#$/) {
            next;
        }
        if (s/^Type.*=//i) {
            if (s/\+//) {
                $plus = "+";
            }
            chop($type = $_);
            next;
        }
        if (s/^Name.*=//i) { chop($name = $_); next; }
        if (s/^Path.*=//i) { chop($path = $_); next; }
        if (s/^Host.*=//i) { chop($host = $_); next; }
        if (s/^Port.*=//i) { chop($port = $_); next; }
        if (/^#/) {
            print OUT "${type}${name}\t${path}\t${host}\t${port}\t${plus}\n";
            $type = $name = $path = $port = $host = $plus = "";
            $lastwrite = $.
        }
    }
    if (($. - $lastwrite) >= 5) {
        # last bookmark, no ending '#'
        print OUT "${type}${name}\t${path}\t${host}\t${port}\t${plus}\n";
    }
}

sub tounix {
    $in = "mac.bookmarks";
    $out = ".gopherrc";
    &openfiles;
    
    while(<IN>) {
        $type = $name = $path = $port = $host = $plus = "";
        ($name, $path, $host, $port, $plus) = split('\t');
        $type = ($name =~ /^\d/); # 1 digit in length?
        ($plus =~ /\+/) && $type .= '+';
        $name =~ s/^\d//;
        print OUT "#\n";
        print OUT "Type=${type}\n";
        print OUT "Name=${name}\n";
        print OUT "Path=${path}\n";
        print OUT "Host=${host}\n";
        print OUT "Port=${port}\n";
    }
}
