#
# FILE rosetex.awk
#
# translate ROSE product file into LaTeX format for printing
#
# written:       1992-10-23 <Gerhard.Gonter@wu-wien.ac.at>
# latest update: 1993-05-29
#
# ----------------------------------------------------------------------------
BEGIN {
  verb=0;
  xverb=-1;
  emptyline=0;          # number of empty lines suppressed in verbatim env.
  args["#erase"]="\\subsection";
  argt["#erase"]="Module";
  args["#help"]="\\subsection";
  argt["#help"]="{\\sl{help}}";
}
# ----------------------------------------------------------------------------
$1=="#section"||$1=="#subsection"||$1=="#subsubsection"||$1=="#paragraph" {
  if (verb==1) {
    print "\\end{verbatim}\n";
    verb=0;
  }
  nm=substr($0,length($1)+2);
  print "\\"substr($1,2)"{"nm"}\n";
  if ($1=="#section") {
    print "\\def\\LPtopC{"nm"}\n";
    print "\\def\\LPtopD{~}\n";
    print "\\def\\LPtopF{~}\n";
  }
  if ($1=="#subsection") {
    print "\\def\\LPtopD{"nm"}\n";
    print "\\def\\LPtopF{~}\n";
  }
  if ($1=="#subsubsection") {
    print "\\def\\LPtopF{"nm"}\n";
  }
  next;
}

# ----------------------------------------------------------------------------
$1=="#erase"||$1=="#help" {
  if (verb==1) {
    print "\\end{verbatim}\n";
    verb=0;
  }
  nm=substr($0,length($1)+2);
  gsub("_","\\_",nm);
  print args[$1]"{"argt[$1]" {\\tt "nm"}}\n"
  print "\\def\\LPtopD{"argt[$1]" {\\tt "nm"}}\n";
  print "\\def\\LPtopF{~}\n";
#  /* this line needs to be printed too! */
}

# ----------------------------------------------------------------------------
$1=="#end" {
  if (verb==1) {
    print "\\end{verbatim}\n";
    verb=0;
  }
  next;
}

# ----------------------------------------------------------------------------
$1=="#organize" {
  args[$2]=$3;
  argt[$2]=$4;
}

# ----------------------------------------------------------------------------
$1=="#verbatim" {
  xverb=verb;
  verb=2;
  next;
}

# ----------------------------------------------------------------------------
$1=="#v" {
  print substr($0,4);
  next;
}

# ----------------------------------------------------------------------------
$1=="#endverbatim" {
  verb=xverb;
  next;
}

# ----------------------------------------------------------------------------
verb==0&&$0!=""&&$0!=" " {
  print "\\begin{verbatim}"
  verb=1;
  emptyline=0;
}

# ----------------------------------------------------------------------------
verb==1 {
  if ($0==""||$0==" ") {
    emptyline++;
  } else {
    for (; emptyline>0; emptyline--) print "";  # make empty line
    print
  }
}

# ----------------------------------------------------------------------------
verb==2 { print }       # LaTeX section in ROSE product file

# ----------------------------------------------------------------------------
END {
  if (verb==1) {
    print "\\end{verbatim}\n";
    verb=0;
  }
}
