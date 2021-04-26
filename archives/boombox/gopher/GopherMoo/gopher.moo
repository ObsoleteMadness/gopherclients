gopher.moo	Thu July 15, 1993, 9:31PM, Version 1.0

Copyright (c) 1992, 1993, Larry Masinter, Erik Ostrom
All Rights Reserved

Permission granted to use this software for non-commercial purposes;
we'd like to be notified of any enhancements, applications, or
bug-fixes in the software.

This is a general MOO interface to Gopher. To use it, you need a MOO
server. The MOO software is available from
<ftp://parcftp.xerox.com/pub/MOO/>. In addition, you need some minor
changes to the MOO server, so that does not change tabs into spaces on
input, and to have open_network_connection enabled.


================================================================
Be sure you're running the server patch (describe dbelow) first!
This is a dump of $gopher, the gopher slate, and $network

Create three objects
	@create #1 named Gopher utilities
	@create $thing named Generic Gopher Slate
	@create #1 named Network Utilities

edit the following script to replace these numbers numbers with the
numbers of the three new ones, and then execute it.

  #11111    == number of Gopher utilities
  #22222    == number of Generic Gopher Slate
  #33333    == number of Network Utilities

(Note the '@prop #0.gopher #11111' and '@prop #0.network #33333' commands,
which set $gopher and $network respectively. You may need to
@rmprop #0.network to remove the bogus LambdaCore version.)

Fix $network.postmaster, .site, .port, .MOO_name, .large_domains.
$network.large_domains = list of network domains such that FOO.name.edu
			should be considered a separate host than
			BAR.name.edu.

Using the Generic slate, use 'goto host port on generic slate' and
'remember on slate' to set up the default 'top level' menu of new
gopher slates.


WARNING: The script contains tab characters. Be sure they don't get turned into spaces.

================================================================
Change log:

Version 0.1  -- initial release
Version 0.2  -- use $network:open instead of raw o_n_c
	        validity check on host names
		limit on retrievals
		add (some) documentation to $gopher.room verbs
		gopher rooms have a remembered set
		add CSO phone book entries
		use .desclines property instead of :description
		(exam won't spam).
		add gopher lists
Version 0.3:	change gopher room to portable slate
		subsumes notes
		differential cache timeout (shorter for failures)
		
Version 0.4:    Include $network in release (Thanks to unattributed JHM programmers)
		Add 'controlled' state on slate.
                Slates show headers when they update if watcher isn't controller.

Version 0.5:    clean up the $network dump some

Version 0.6:
Version 0.7:    very minor patches: more general mailing,
                hopefully better installation instructions

Version 1.0:	after port to LambdaMOO, simplify $network, $gopher


================================================================
The patch to allow tabs is in: net_multi.c
(not sure if a similar change is necessary in net_single.c)
***************
*** 157,161 ****
  		stream_add_char(s, c);
  	    else if (c == '\t')
! 		stream_add_char(s, '\t');
  	    else if (c == proto.eol_in_char)
  		server_receive_line(h->shandle, reset_stream(s));
--- 157,161 ----
  		stream_add_char(s, c);
  	    else if (c == '\t')
! 		stream_add_char(s, ' ');
  	    else if (c == proto.eol_in_char)
  		server_receive_line(h->shandle, reset_stream(s));
================================================================
@prop #0.gopher #11111 "r"
@prop #11111."cache_requests" {} r
@prop #11111."cache_times" {} r
@prop #11111."cache_values" {} r
@prop #11111."limit" 2000 rc
@prop #11111."cache_timeout" 900 r
;#11111.("description") = {"An interface to Gopher internet services.", "Copyright (c) 1992,1993 Grump,JoeFeedback@LambdaMOO.", "", "This object contains just the raw verbs for getting data from gopher servers and parsing the results. Look at #50122 (Generic Gopher Slate) for one example of a user interface. ", "", ":get(site, port, selection)", "  Get data from gopher server: returns a list of strings, or an error if it couldn't connect. Results are cached.", "", ":get_now(site, port, selection)", "  Like $gopher:get, but bypass the cache (used by $gopher:get).", "", ":show_text(who, start, end, site, port, selection)", "  Requires wiz-perms to call.", "  like who:notify_lines($gopher:get(..node..)[start..end])", "", ":clear_cache()", "  Erase the gopher cache.", "", ":parse(string)", "  Takes a directory line as returned by $gopher:get, and return a list", "  {host, port, selector, label}", "   host, port, and selector are what you send to :get.", "  label is a string, where the first character is the type code.", "", ":type(char)", "   returns the name of the gopher type indicated by the character, e.g.", "   $gopher:type(\"I\") => \"image\"", ""}
@verb #11111:"get_now" this none this rx
@program #11111:get_now
"Usage:  get_now(site, port, message)";
"Returns a list of strings, or an error if we couldn't connect.";
host = args[1];
port = args[2];
if (!this:trusted(caller_perms()))
  return E_PERM;
elseif ((!match(host, $network.valid_host_regexp)) && (!match(host, "[0-9]+%.[0-9]+%.[0-9]+%.[0-9]+")))
  "allow either welformed internet hosts or explicit IP addresses.";
  return E_INVARG;
elseif (((port != 70) && (port != 80)) && (port < 100))
  "disallow connections to low number ports; necessary?";
  return E_INVARG;
endif
opentime = time();
con = $network:open(args[1], args[2]);
opentime = (time() - opentime);
if (typeof(con) == ERR)
  return con;
endif
read(con);
"eliminate blank line";
notify(con, args[3]);
results = {};
count = this.limit;
"perhaps this isn't necessary, but if a gopher source is slowly spewing things, perhaps we don't want to hang forever -- perhaps this should just fork a process to close the connection instead?";
now = time();
timeout = 30;
end = "^%.$";
if ((length(args) == 4) && (args[4][1] == "2"))
  end = "^[2-9]";
endif
while ((((typeof(string = read(con)) == STR) && (!match(string, end))) && ((count = (count - 1)) > 0)) && ((now + timeout) > (now = time())))
  if (string && (string[1] == "."))
    string = string[2..length(string)];
  endif
  results = {@results, string};
endwhile
$network:close(con);
if (opentime > 0)
  "This is to keep repeated calls to $network:open to 'slow responding hosts' from totally spamming.";
  suspend(0);
endif
return results;
.

@verb #11111:"parse" this none this
@program #11111:parse
"parse gopher result line:";
"return {host, port, tag, label}";
"host/port/tag are what you send to the gopher server to get that line";
"label is <type>/human readable entry";
string = args[1];
tab = index(string, "	");
label = string[1..tab - 1];
string = string[tab + 1..length(string)];
tab = index(string, "	");
tag = string[1..tab - 1];
string = string[tab + 1..length(string)];
tab = index(string, "	");
host = string[1..tab - 1];
string = string[tab + 1..length(string)];
tab = index(string, "	");
port = tonum(tab ? string[1..tab - 1] | string);
return {host, port, tag, label};
"ignore extra material after port, if any";
.

@verb #11111:"show_text" this none this rx
@program #11111:show_text
"$gopher:show_text(who, start, end, ..node..)";
"like who:notify_lines($gopher:get(..node..)[start..end]), but pipelined";
if (!caller_perms().wizard)
  return E_PERM;
endif
who = args[1];
start = args[2];
end = args[3];
args = args[4..length(args)];
con = $network:open(args[1], args[2]);
if (typeof(con) == ERR)
  player:tell("Sorry, can't get this information now.");
  return;
endif
notify(con, args[3]);
read(con);
"initial blank line";
line = 0;
sent = 0;
end = (end || this.limit);
while (((string = read(con)) != ".") && (typeof(string) == STR))
  line = (line + 1);
  if ((line >= start) && ((!end) || (line <= end)))
    sent = (sent + 1);
    if (valid(who))
      if (string && (string[1] == "."))
        string = string[2..length(string)];
      endif
      who:notify(string);
    else
      notify(who, string);
    endif
  endif
endwhile
$network:close(con);
return sent;
.

@verb #11111:"type" this none this
@program #11111:type
type = args[1];
if (type == "1")
  return "menu";
elseif (type == "?")
  return "menu?";
elseif (type == "0")
  return "text";
elseif (type == "7")
  return "search";
elseif (type == "9")
  return "binary";
elseif (type == "2")
  return "phone directory";
elseif (type == "4")
  return "binhex";
elseif (type == "8")
  return "telnet";
elseif (type == "I")
  return "image";
elseif (type == " ")
  "not actually gopher protocol: used by 'goto'";
  return "";
else
  return "unknown";
endif
"not done, need to fill out";
.

@verb #11111:"summary" this none this
@program #11111:summary
"return a 'nice' string showing the information in a gopher node";
if (typeof(parse = args[1]) == STR)
  parse = this:parse(parse);
endif
if (parse[1] == "!")
  return {"[remembered set]", "", ""};
endif
if (length(parse) > 3)
  label = parse[4];
  if (label)
    type = $gopher:type(label[1]);
    label = label[2..length(label)];
    if (type == "menu")
    elseif (type == "search")
      label = ((("<" + parse[3][rindex(parse[3], "	") + 1..length(parse[3])]) + "> ") + label);
    else
      label = ((type + ": ") + label);
    endif
  else
    label = "(top)";
  endif
else
  label = (parse[3] + " (top)");
endif
port = "";
if (parse[2] != 70)
  port = tostr(" ", parse[2]);
endif
return {tostr("[", parse[1], port, "]"), label, parse[3]};
.

@verb #11111:"get" this none this
@program #11111:get
"Usage: get(site, port, selection)";
"returns a list of strings, or an error if it couldn't connect. Results are cached.";
request = args[1..3];
while ((index = (request in this.cache_requests)) && (this.cache_times[index] > time()))
  if (typeof(result = this.cache_values[index]) != NUM)
    return result;
  endif
  if ($code_utils:task_valid(result))
    "spin, let other process getting same data win, or timeout";
    suspend(1);
  else
    "well, other process crashed, or terminated, or whatever.";
    this.cache_times[index] = 0;
  endif
endwhile
if (!this:trusted(caller_perms()))
  return E_PERM;
endif
while (this.cache_times && (this.cache_times[1] < time()))
  $command_utils:suspend_if_needed(0);
  this.cache_times = listdelete(this.cache_times, 1);
  this.cache_values = listdelete(this.cache_values, 1);
  this.cache_requests = listdelete(this.cache_requests, 1);
  "caution: don't want to suspend between test and removal";
endwhile
$command_utils:suspend_if_needed(0);
this:cache_entry(@request);
value = this:get_now(@args);
$command_utils:suspend_if_needed(0);
index = this:cache_entry(@request);
this.cache_times[index] = (time() + ((typeof(value) == ERR) ? 120 | 1800));
this.cache_values[index] = value;
return value;
.

@verb #11111:"clear_cache" this none this
@program #11111:clear_cache
if (!this:trusted(caller_perms()))
  return E_PERM;
endif
if (!args)
  this.cache_values = (this.cache_times = (this.cache_requests = {}));
elseif (index = (args[1..3] in this.cache_requests))
  this.cache_requests = listdelete(this.cache_requests, index);
  this.cache_times = listdelete(this.cache_times, index);
  this.cache_values = listdelete(this.cache_values, index);
endif
.

@verb #11111:"unparse" this none this
@program #11111:unparse
"unparse(host, port, tag, label) => string";
host = args[1];
port = args[2];
tag = args[3];
label = args[4];
if (tab = index(tag, "	"))
  "remove search terms from search nodes";
  tag = tag[1..tab - 1];
endif
return tostr(label, "	", tag, "	", host, "	", port);
.

@verb #11111:"interpret_error" this none this
@program #11111:interpret_error
"return an explanation for a 'false' $gopher:get result";
value = args[1];
if (value == E_INVARG)
  return "That gopher server is not reachable or is not responding.";
elseif (value == E_QUOTA)
  return "Gopher connections cannot be made at this time because of system resource limitations!";
elseif (typeof(value) == ERR)
  return tostr("The gopher request results in an error: ", value);
else
  return "The gopher request has no results.";
endif
.

@verb #11111:"trusted" this none this
@program #11111:trusted
"default -- gopher trusts everybody";
return 1;
.

@verb #11111:"_textp" this none this
@program #11111:_textp
"_textp(parsed node)";
"Return true iff the parsed info points to a text node.";
return index("02", args[1][4][1]);
.

@verb #11111:"_mail_text" this none this
@program #11111:_mail_text
"_mail_text(parsed node)";
"Return the text to be mailed out for the given node.";
where = args[1];
if (this:_textp(where))
  return $gopher:get(@where);
else
  text = {};
  for x in ($gopher:get(@where))
    parse = $gopher:parse(x);
    sel = parse[4];
    text = {@text, "Type=" + sel[1], "Name=" + sel[2..length(sel)], "Path=" + parse[3], "Host=" + parse[1], "Port=" + tostr(parse[2]), "#"};
  endfor
  return text;
endif
.

@verb #11111:"init_for_core" this none this
@program $gopher:init_for_core
if (caller_perms().wizard)
   this:clear_cache();
   pass(@args);
endif
.
@verb #11111:"display_cache" this none none rxd
@program #11111:display_cache
"Just for debugging -- shows what's in the gopher cache";
req = this.cache_requests;
tim = this.cache_times;
val = this.cache_values;
"save values in case cache changes while printing";
player:tell("size -- expires -- host (port) ------ selector ------------");
for i in [1..length(req)]
  re = req[i];
  host = $string_utils:left(re[1] + ((re[2] == 70) ? "" | tostr(" (", re[2], ")")), 24);
  expires = $string_utils:right($time_utils:dhms(tim[i] - time()), 8);
  va = val[i];
  if (typeof(va) == LIST)
    va = length(va);
  elseif (typeof(va) == ERR)
    va = $error:name(va);
  else
    va = tostr(va);
  endif
  selector = re[3];
  if (length(selector) > 40)
    selector = ("..." + selector[length(selector) - 37..length(selector)]);
  endif
  player:tell($string_utils:right(va, 8), expires, " ", host, selector);
endfor
player:tell("--- end cache display -------------------------------------");
.

@verb #11111:"get_cache" this none this
@program #11111:get_cache
"Usage: get_cache(site, port, selection)";
"return current cache";
request = args[1..3];
if (index = (request in this.cache_requests))
  if (this.cache_times[index] > now)
    return this.cache_values[index];
  endif
endif
return 0;
.

@verb #11111:"cache_entry" this none this
@program #11111:cache_entry
if (index = (args in this.cache_requests))
  return index;
else
  this.cache_times = {@this.cache_times, time() + 240};
  this.cache_values = {@this.cache_values, task_id()};
  this.cache_requests = {@this.cache_requests, args};
  return length(this.cache_requests);
endif
.

"***finished loading $gopher***
@prop #22222."value" {} r
@prop #22222."stack" {} r
@prop #22222."busy" 0 r
@prop #22222."remembered" {} r
@prop #22222."desclines" {} r
@prop #22222."seen" {} r
@prop #22222."length" 20 rc
@prop #22222."help_msg" {} rc
;#22222.("help_msg") = {"Moving around:", " pick <item> on slate", "    select the given menu item (either a number or partial name).", "    If it is a text item, it will show it to you.", " <number> on slate", "    e.g., 12 on slate. You can omit `pick' when chosing items", "    by their number.", " back slate [for n]", "    go back up a level; with n supplied, goes back n levels", " reset slate", "    reset slate to the default list of `remember'-ed nodes", " goto host [port [path]] on slate", "    make a direct jump to a specified host. Please be careful --", "    at the moment this slows everyone down if the host isn't valid.", "", "Controlling noise:", " ignore slate", "    stop listening when other people fiddle with the slate", " watch slate", "    start watching while other people fiddle with the slate", " show slate to person", "    show the contents of the slate to someone even if they're not watching", "", "Bookmarks:", " remember [<item>] on slate", "    adds item to the default of nodes", "    will prompt you for title", " remember on slate", "    remembers the current menu choice rather than any ", "    particular item", " forget <item> on slate", "    (only when looking at the default list)", "    deletes the given item", "", "In long menus and text:", " next [<n>] on slate", " prev [<n>] on slate", "    move you forward/backward in the set of visible menu items.", "    You can give a `number of pages' to move forward.", " read slate", "    show you the entire contents of the slate", "", "Miscellaneous:", " stack slate", "    show stack, where `back' will go", " details <item> on slate", "    show host, port number, and selection string for a given item.    ", " mailme slate", "    if you have a valid registration address: send mail with the", "    slate contents to your email address.", "", "When you first make a gopher slate, you will need to use `goto'", "and then `remember' to set up the default list of nodes."}
@prop #22222."locked" 0 r
@prop #22222."ignoring" {} r
@prop #22222."watching" {} r
@prop #22222."controlled" #-1 r
@prop #22222."work_with_msg" "%N %<starts> to work with %d." rc
;#22222.("description") = "A laptop size computer, with various controls on it."

@verb #22222:"p*ick" any on this rxd
@program #22222:pick
"pick <entry> on slate";
"  entry is either a line number or an initial substring of a line description";
"  select that entry: if it is a menu, go to that node. If it is a search,";
"  asks you for the search term & does the search.";
"  Some kinds of nodes are not implemented.";
if (this:_textp() || (!(this.stack || this.remembered)))
  return player:tell("There's nothing to pick.");
endif
if (this:busy("picking"))
  return;
endif
if (!(which = this:match_choice(dobjstr)))
  "match_choice took care of it.";
  this:busy(0);
  return;
endif
if ((tostr(tonum(dobjstr)) == dobjstr) && (!({player, @this:_place()} in this.seen)))
  player:tell($string_utils:pronoun_sub("Oooops, perhaps you should look at the %t first."));
  this:busy(0);
  return;
endif
parse = $gopher:parse(this.value[which]);
desc = this.desclines[which];
this:announce_op("%N %<picks> '", desc, "' on the %t.");
this:do_pick(@parse);
return;
.

@verb #22222:"reset" this none none rxd
@program #22222:reset
"reset slate";
"  reset the slate to its set of 'remembered' selections";
if (why = this:is_locked(player))
  return player:tell($string_utils:pronoun_sub("Sorry, %t seems to be "), why, ".");
elseif (this:busy("resetting"))
  return;
endif
this:announce_op("%N %<resets> the %t.");
this.seen = {};
this:set_pointer();
this:busy(0);
.

@verb #22222:"pop back" any any any rxd
@program #22222:pop
"back this [by <n>]";
"  move back up the gopher stack to the previous menu";
"  or previous N menus.";
n = 1;
if (iobjstr && (!(iobjstr == tostr(n = tonum(iobjstr)))))
  return player:tell("Sorry, '", iobjstr, "' doesn't look like a number.");
endif
if (length(this.stack) < n)
  player:tell("Sorry, there aren't ", n, " levels to go back.");
  return;
endif
if (this:busy("going back"))
  return;
endif
this:announce_op("%N %<goes> back up ", (n == 1) ? "a level" | tostr(n, " levels"), " on the %t.");
this:set_pointer(@this.stack[n + 1..length(this.stack)]);
this:busy(0);
.

@verb #22222:"location_string" this none this rx
@program #22222:location_string
"location_string([location])";
"A nice-looking version of the location provided, or current location.";
loc = ((args && args[1]) || this.stack[1]);
where = loc[1];
if (st = loc[4])
  "human readable string";
  return ((st[2..length(st)] + " (from ") + where) + ")";
  return (where + ": ") + st[2..length(st)];
endif
if (loc[3])
  return ((loc[3] + " (from ") + where) + ")";
  return (where + ": ") + loc[3];
endif
return where;
.

@verb #22222:"stack" this none none rxd
@program #22222:stack
"stack slate";
"  show a summary of the gopher stack";
max = 0;
if (!this.stack)
  return player:tell($string_utils:pronoun_sub("%T is at the top level."));
endif
for x in (this.stack)
  max = max(max, length(x[1]));
endfor
max = (max + 6);
for x in ($list_utils:reverse(this.stack))
  summary = $gopher:summary(x);
  player:tell($string_utils:left(summary[1], max), " ", summary[2]);
endfor
.

@verb #22222:"busy" this none this
@program #22222:busy
"interlock for caching -- mark cache busy or clear; return true of interlock failed";
if (args[1])
  if ((args[1] != "reading") && (why = this:is_locked(player)))
    player:tell($string_utils:pronoun_sub("Sorry, %t seems to be "), why, ".");
    return 1;
  endif
  "make player running this watch it.";
  this.watching = setadd(this.watching, player);
  "set busy";
  if (this.busy && (this.busy[1] > time()))
    player:tell("***Sorry, ", this.name, " is busy ", this.busy[2], " for ", this.busy[3], " -- wait a bit.");
    return 1;
  else
    this.busy = {time() + (60 * 5), args[1], player.name, task_id()};
    return 0;
  endif
else
  this.busy = 0;
  return 0;
endif
.

@verb #22222:"match_choice" this none this
@program #22222:match_choice
"match_choice(input string)";
"returns the index of the choice, or 0.";
"is noisy.";
if (this:_textp())
  player:tell($string_utils:pronoun_sub("%T is looking at a text node and has no choices."));
  return 0;
endif
input = args[1];
which = $code_utils:tonum(input);
len = length(value = this.value);
if (typeof(which) == NUM)
  if ((which < 1) || (which > len))
    player:tell("Sorry, ", input, " isn't a number between 1 and ", len, ".");
    return 0;
  endif
  return which;
else
  exact = (partial = {});
  for choice in [1..len]
    valchoice = value[choice][2..index(value[choice], "	") - 1];
    if (input == valchoice)
      exact = {@exact, choice};
    elseif (index(valchoice, input) == 1)
      partial = {@partial, choice};
    endif
  endfor
  if (length(exact) > 1)
    player:tell("I'm not sure whether you meant ", $string_utils:english_list(exact, "", " or "), ".");
    return 0;
  elseif (exact)
    return exact[1];
  elseif (length(partial) > 1)
    player:tell("I'm not sure whether you meant ", $string_utils:english_list(partial, "", " or "), ".");
    return 0;
  elseif (partial)
    return partial[1];
  else
    player:tell("Sorry, there is no choice named ", $string_utils:print(input), ".");
    return 0;
  endif
endif
.

@verb #22222:"jump goto" any on this rxd
@program #22222:jump
"goto <host> [socket] on slate";
"  given an explicit host name and optional socket, attempt to open a";
"  gopher connection to that socket";
words = $string_utils:words(dobjstr);
if (!words)
  player:tell("Usage:  ", verb, " <host> [socket]", prepstr ? tostr(" on ", iobjstr) | "");
  return;
endif
host = words[1];
socket = 70;
if (length(words) > 1)
  socket = tonum(words[2]);
  if (socket < 3)
    player:tell("The value '", words[2], "' is not a valid socket.");
    return;
  endif
endif
path = "";
if (length(words) > 2)
  path = dobjstr[(index(dobjstr, words[2]) + length(words[2])) + 1..length(dobjstr)];
endif
if (this:busy(tostr("jumping to ", host, " socket ", socket)))
  return;
endif
this:announce_op(tostr("%N %<jumps> to ", host, " socket ", socket, path ? " " | "", path, " on the %t."));
parse = {host, socket, path, "1<jump>"};
this:set_pointer(parse, @this:_textp() ? listdelete(this.stack, 1) | this.stack);
this:busy(0);
.

@verb #22222:"details" any on this rxd
@program #22222:details
if (!(which = this:match_choice(dobjstr)))
  "match_choice took care of it.";
  return;
endif
parse = $gopher:parse(this.value[which]);
sel = parse[4];
if (sel)
  for x in ({"Type=" + sel[1], "Name=" + sel[2..length(sel)], "Path=" + parse[3], "Host=" + parse[1], "Port=" + tostr(parse[2]), "#"})
    player:tell(x);
  endfor
else
  player:tell("**** ERROR, ", which, " is not a valid entry.");
endif
.

@verb #22222:"set_pointer" this none this rx
@program #22222:set_pointer
if (!args)
  value = this.remembered;
else
  value = $gopher:get(@args[1]);
endif
if (!value)
  this:busy(0);
  this:announce_op($gopher:interpret_error(value));
  return 0;
endif
if (value[1][1] == "3")
  this:busy(0);
  this:announce_op("The gopher request results in an error:");
  for x in (value)
    this:announce_op(": ", x ? x[2..length(x)] | x);
  endfor
  return 0;
endif
if (args && (args[1][4][1] == "0"))
  "text node";
  desc = value;
else
  desc = {};
  cnt = 1;
  for x in (value)
    $command_utils:suspend_if_needed(0);
    type = $gopher:type(x[1]);
    if (type == "text")
      type = "";
    else
      type = ((" (" + type) + ")");
    endif
    tab = index(x, "	");
    label = x[2..tab - 1];
    desc = {@desc, tostr(cnt, ". ", label, type)};
    cnt = (cnt + 1);
  endfor
endif
$command_utils:suspend_if_needed(0);
this.desclines = desc;
this.stack = args;
this.value = value;
this:busy(0);
this:show_results();
return 1;
.

@verb #22222:"do_pick" this none this
@program #22222:do_pick
"do_pick(host, port, path, string) -- take parsed output & interact with user as appropriate.";
string = args[4];
if ((!string) || index("1?", type = string[1]))
  "menu";
  this:set_pointer(args, @this.stack);
elseif (type == "7")
  player:tell("Search for what? Enter search line or @abort:");
  search = read();
  if (search != "@abort")
    this:announce_op("%N %<searches> for ", search, " on %t.");
    this:set_pointer({args[1], args[2], (args[3] + "	") + search, args[4]}, @this.stack);
  else
    this:busy(0);
    this:announce_op("%N %<decides> not to search.");
  endif
elseif (type == "3")
  this:busy(0);
  this:announce_op("%N chose an error line.");
elseif (type == "0")
  "slates can point at text nodes";
  this:set_pointer(args, @this.stack);
elseif (type == "2")
  search = $command_utils:read("one of 'name=<name>' 'phone=<phone>' 'email=<email>'");
  if (!match(search, "[a-z]+=[a-z0-9@-]+"))
    this:busy(0);
    player:tell((search == "@abort") ? "No search." | ("Invalid query: " + search));
    return;
  endif
  this:announce_op("%N %<searches> for ", search, " on %t.");
  this:set_pointer({args[1], args[2], (args[3] + "	query ") + search, args[4]}, @this.stack);
elseif ($object_utils:has_property(player, "gopher_local") && player.gopher_local)
  this:busy(0);
  notify(player, tostr("#$# gopher	", args[1], "	", args[2], "	", args[4], "	", args[3]));
else
  this:busy(0);
  this:announce_op("Type ", type, " (", $gopher:type(type), ") gopher requests not implemented.");
  if (type == "8")
    player:tell("**** telnet ", args[1], (args[2] in {23, 0}) ? "" | (" " + tostr(args[2])));
    if (args[3])
      player:tell("     log in as: ", args[3]);
    endif
  endif
endif
.

@verb #22222:"remember" any on this rxd
@program #22222:remember
"remember <entry/here> on <this>";
"  add the entry (or this menu) to the 'remembered set' for this room.";
"  use 'remembered' to retrieve the set.";
if (!this.stack)
  return player:tell("Sorry, remembering remembered nodes doesn't work.");
endif
if (dobjstr == "")
  parse = this.stack[1];
  desc = "the current menu";
elseif (choice = this:match_choice(dobjstr))
  parse = $gopher:parse(this.value[choice]);
  desc = this.desclines[choice];
else
  "Match_choice took care of it.";
  return;
endif
parse[4] = (parse[4][1] + $command_utils:read("description for " + desc));
this.remembered = {@this.remembered, $gopher:unparse(@parse)};
this:announce_op("%N %<remembers> ", desc, " on the %t as ", parse[4][2..length(parse[4])], ".");
.

@verb #22222:"forget delete" any on this rxd
@program #22222:forget
"forget <entry> on slate";
"  erase an entry from the 'remembered set'";
"  only works if you're looking at the 'remembered set'";
if (this.stack)
  player:tell("You're not looking at the top.");
  return;
endif
if (!(choice = this:match_choice(dobjstr)))
  return;
endif
this:announce_op("%N %<forgets> '", this.desclines[choice], "' on the %t.");
this.remembered = listdelete(this.remembered, choice);
this:set_pointer();
.

@verb #22222:"look_self" this none this
@program #22222:look_self
if (this.stack)
  sum = $gopher:summary(this.stack[1]);
  player:tell(this:titlec(), ": ", sum[1], " ", sum[2]);
else
  player:tell(this:titlec());
endif
player:tell_lines(this:description());
this:_tell_desc();
state = "";
if (valid(this.controlled))
  state = (($string_utils:pronoun_sub("The %t is being controlled by ") + this.controlled:title()) + ".");
endif
if ((busy = this:_is_busy()) || state)
  player:tell(state ? state + " " | "", busy ? $string_utils:pronoun_sub(tostr("The %t is busy ", this.busy[2], " for ", this.busy[3], ".")) | "");
endif
.

@verb #22222:"_tell_desc" this none this
@program #22222:_tell_desc
who = (args ? args[1] | player);
plen = ((length(args) > 1) ? args[2] | this.length);
header = ((length(args) > 2) && args[3]);
if (this:_textp())
  text = this:text();
  len = length(text);
  if ((!plen) || (len <= plen))
    $command_utils:suspend_if_needed(0);
    "6/24/93 change tell_lines to notify_lines to reduce lag.";
    if (header)
      who:tell("--------------- ", this.name, "-----");
      who:notify_lines(text);
      who:tell("--------------- ", this.name, "-----");
    else
      who:notify_lines(text);
    endif
    return;
  endif
  offset = this:offset();
  npages = ((len / plen) + 1);
  thispage = ((offset / plen) + 1);
  if ((offset != 1) || header)
    who:tell("--", thispage, " of ", npages, "----- 'prev on ", this.name, "' for previous----");
  endif
  end = ((offset + plen) - 1);
  who:tell_lines(text[offset..min(len, end)]);
  if ((len > end) || header)
    who:tell("--", thispage, " of ", npages, "----- 'next on ", this.name, "' for more --------");
  endif
  return;
endif
this.seen = setadd(this.seen, {who, @this:_place()});
len = length(this.desclines);
if (header)
  who:tell("--------------- ", this.name, "-----");
endif
if (plen && (len > plen))
  offset = this:offset();
  who:tell_lines(this.desclines[offset..min((offset + this.length) - 1, len)]);
  nxt = ("next on " + this.name);
  prv = ("previous on " + this.name);
  who:tell("---- '", (offset == 1) ? nxt | (((offset + plen) > len) ? prv | (((("'" + nxt) + "' or '") + prv) + "'")), "' to see additional choices (", len, " total) ---");
else
  who:tell_lines(this.desclines || {$string_utils:pronoun_sub("%T is empty right now.")});
  if (header)
    who:tell("--------------- ", this.name, "-----");
  endif
endif
.

@verb #22222:"next prev*ious" any on this rxd
@program #22222:next
if (this:busy("reading"))
  "can't 'next' if it is busy";
  return;
endif
this:busy(0);
n = (tonum(dobjstr) || 1);
if (verb != "next")
  n = (-n);
  verb = "previous";
endif
offset = this:offset();
new = (offset + (n * this.length));
if (new < 1)
  if (offset == 1)
    return player:tell("You're already at the beginning.");
  else
    new = 1;
  endif
elseif (new > length(this.desclines))
  return player:tell("You're already at the end.");
endif
this:announce_op("%N %<looks> at the ", verb, " ", this:_textp() ? "page" | "results", " on the %t.");
this:offset(new);
this:show_results();
.

@verb #22222:"initialize" this none this
@program #22222:initialize
if ((caller == this) || $perm_utils:controls(caller_perms(), this))
  "don't call this unless you mean it.";
  this.seen = {};
  this.desclines = {};
  "The default is that slate's inherit the 'remembered' from their parent. This means, though, that they're initially blank but have to be 'reset' to fire up. See :do_reset";
  "this.remembered = {}";
  this.busy = 0;
  this.stack = {};
  this.watching = {};
  this.controlled = #-1;
  pass(@args);
endif
.

@verb #22222:"announce_op" this none this
@program #22222:announce_op
msg = tostr(@args);
player:tell($string_utils:pronoun_sub(msg, $you));
if (this.location != player)
  this.location:announce($string_utils:pronoun_sub(msg));
endif
return;
"announcing only to watching";
if (watching = setremove($set_utils:intersection(this.watching, this.location:contents()), player))
  msg = $string_utils:pronoun_sub(msg);
  for x in (watching)
    x:tell(msg);
  endfor
endif
.

@verb #22222:"_place" this none this
@program #22222:_place
return this.stack && this.stack[1][1..3];
.

@verb #22222:"_textp" this none this
@program #22222:_textp
return this.stack && index("02", this.stack[1][4][1]);
.

@verb #22222:"r*ead" any any any rxd
@program #22222:read
if ((!argstr) || ((dobj == this) && (!prepstr)))
  this:_tell_desc(player, 0);
elseif (which = this:match_choice((($code_utils:short_prep(prepstr) == "on") && (iobj == this)) ? dobjstr | argstr))
  where = $gopher:parse(this.value[which]);
  if (index("02", where[4][1]))
    this:announce_op("%N %<reads> '", this.desclines[which], "' on the %t.");
    $gopher:show_text(player, 0, 0, @where);
    player:tell("-------");
  else
    player:tell("Item '", this.desclines[which], "' isn't text and can't be read.");
  endif
else
  player:tell("Read what?");
endif
.

@verb #22222:"lock unlock" this none none rxd
@program #22222:lock
this.locked = (verb == "lock");
this:announce_op("%N %<", $string_utils:lowercase(verb), "s> %t.");
.

@verb #22222:"text" this none this
@program #22222:text
return this.value;
"don't update slates";
.

@verb #22222:"update" this none none rxd
@program #22222:update
if (this:busy("updating", 1))
  return;
endif
this:announce_op("%N %<updates> %t.");
if (this.stack)
  $gopher:clear_cache(@this.stack[1]);
endif
this:set_pointer(@this.stack);
.

@verb #22222:"_mail_text" this none this
@program #22222:_mail_text
if (this:_textp())
  return this.value;
else
  text = {};
  for x in (this.value)
    parse = $gopher:parse(x);
    sel = parse[4];
    text = {@text, "Type=" + sel[1], "Name=" + sel[2..length(sel)], "Path=" + parse[3], "Host=" + parse[1], "Port=" + tostr(parse[2]), "#"};
  endfor
  return text;
endif
.

@verb #22222:"show_results" this none this
@program #22222:show_results
"after a selection is made, this verb is used to show the results; usually to 'player'";
inhere = ($object_utils:isa(this.location, $room) ? this.location:contents() | {player});
for x in (this.watching = setadd(this.watching, player))
  $command_utils:suspend_if_needed(0);
  if (x in inhere)
    this:_tell_desc(x, this.length, player != x);
  else
    this.watching = setremove(this.watching, x);
  endif
endfor
.

@verb #22222:"ignore watch" this none none rxd
@program #22222:ignore
was = (player in this.watching);
this.watching = ((verb == "watch") ? setadd(this.watching, player) | setremove(this.watching, player));
is = (player in this.watching);
if (was == is)
  player:tell("You already were ", (verb == "watch") ? "watching" | "ignoring", " ", this:title(), ".");
elseif (this.location == player)
  player:tell("You start to ", verb, " ", this:title(), ".");
else
  $you:say_action(("%N %<starts> to " + verb) + " %t.");
endif
.

@verb #22222:"show" this to any rxd
@program #22222:show
if (!valid(iobj))
  return player:tell("I don't see '", iobjstr, "' here.");
endif
$you:say_action("%N %<shows> %t to %i.");
this:_tell_desc(iobj, this.length, 1);
.

@verb #22222:"_is_busy" this none this
@program #22222:_is_busy
if (this.busy)
  if (this.busy[1] > time())
    return 1;
  else
    this.busy = 0;
  endif
endif
return 0;
.

@verb #22222:"control" this none none rxd
@program #22222:control
if (this.controlled == player)
  player:tell("You are already controlling ", this:title(), ".");
  return;
endif
from = (valid(this.controlled) ? (" from " + this.controlled:title()) + "." | ".");
if (this.location != player)
  this.location:announce_all_but({player}, $string_utils:pronoun_sub("%N takes the controls of %t"), from);
endif
player:tell("You take the controls of ", this:title(), from);
this.controlled = player;
.

@verb #22222:"release" this none none rxd
@program #22222:release
if (this.controlled == player)
  $you:say_action("%N %<releases> the controls of %t.");
  this.controlled = #-1;
else
  player:tell("You weren't holding the controls of ", this.name, ".");
endif
.

@verb #22222:"is_locked" this none this
@program #22222:is_locked
"is this locked?";
if (this.locked)
  return "locked";
elseif (valid(this.controlled) && (this.controlled != args[1]))
  if (this.location in {this.controlled, this.controlled.location})
    return "controlled by " + this.controlled.name;
  else
    this.controlled = #-1;
  endif
endif
return 0;
.

@verb #22222:"match_command" this none this rx
@program #22222:match_command
"match_command(vrb, dlist, plist, ilist)";
"return true if this object can handle the command, false otherwise";
"vrb - name of the verb the player typed";
"dlist - list of objspecs that this command matches";
"plist and ilist - likewise for prepspecs, iobjspecs";
if ((player.focus_object == this) && (this.location in {player, player.location}))
  vrb = args[1];
  dlist = args[2];
  plist = args[3];
  ilist = args[4];
  if (((vrb in {"pick", "jump", "goto", "details", "remember", "forget", "delete", "next", "prev", "previ", "previo", "previou", "previous"}) && ("none" in plist)) && ("none" in ilist))
    return 1;
  elseif (((vrb in {"read", "ignore", "watch"}) && ("none" in dlist)) && ("none" in plist))
    return 1;
  elseif (((vrb in {"show"}) && ("none" in dlist)) && ("at/to" in plist))
    return 1;
  elseif ((vrb in {"reset", "stack", "mailme", "lock", "unlock", "update", "control", "release"}) && (!("on top of/on/onto/upon" in plist)))
    return 1;
  elseif (((vrb in {"pop", "back"}) && ("none" in dlist)) && (("none" in plist) || ("for/about" in plist)))
    return 1;
  endif
endif
return pass(@args);
.

@verb #22222:"work" none with this r
@program #22222:work
"This is a JaysHouseMOO verb -- probably doesn't work on other MOOs without a 'focus' object.";
if (valid(player:set_focus_object(this)))
  $you:say_action(this.work_with_msg);
else
  player:tell("You just can't seem to focus on that.");
endif
.

@verb #22222:"mailme" any any any rxd
@program #22222:mailme
"mailme note";
if ((caller_perms() != player) && (caller != player))
  return player:tell("Someone tried to mail you some text, but it didn't work.");
endif
if (!player.email_address)
  return player:tell("Sorry, you don't have a registered email address.");
endif
if ((!argstr) || ((dobj == this) && (!prepstr)))
  where = this.stack[1];
elseif (which = this:match_choice((($code_utils:short_prep(prepstr) == "on") && (iobj == this)) ? dobjstr | argstr))
  where = $gopher:parse(this.value[which]);
endif
if (where)
  player:tell("Mailing ", this:location_string(where), " to ", player.email_address, ".");
  text = $gopher:_mail_text(where);
  player:tell("... ", length(text), " lines ...");
  text = {tostr("(Mail initiated by ", player.name, " (", player, ") connected from ", $string_utils:connection_hostname(connection_name(player)), " using ", this.name, ")"), @text};
  suspend(0);
  result = $network:sendmail(player.email_address, this:location_string(where), @text);
  if (result == 0)
    player:tell("Mail sent successfully.");
  else
    player:tell("Mail sending error: ", result, ".");
  endif
else
  player:tell("Sorry, can't mail this.");
endif
.

@verb #22222:"header" this none this
@program #22222:header
"used by _tell_desc for prefix & suffix lines";
args[1]:tell("------- ", $string_utils:left($string_utils:pronoun_sub(tostr(@listdelete(args, 1), " ")), args[1]:linelen(), "-"));
.

@verb #22222:"offset" this none this
@program #22222:offset
if (!this.stack)
  return 1;
endif
menu = this.stack[1];
if (args)
  if (length(menu) > 4)
    this.stack[1][5] = args[1];
  else
    this.stack[1] = {@{@menu, "", "", "", ""}[1..4], args[1]};
  endif
elseif (length(menu) > 4)
  return menu[5];
else
  return 1;
endif
.

"***finished loading gopher slate ***
@prop #0.network #33333 "r"
@prop #33333."site" "lambda.parc.xerox.com" r
"Change $network.site to your site
@prop #33333."large_domains" {} r
@prop #33333."open_connections" {} r
@prop #33333."connect_connections_to" {} r
@prop #33333."postmaster" "lambdamoo-registration@parc.xerox.com" rc
"Set $network.postmaster to your email address
@prop #33333."port" 8888 rc
"Set $network.port to the MOO port number
@prop #33333."MOO_name" "LambdaMOO" rc
"Set $network.MOO_Name to the name of the MOO
@prop #33333."valid_host_regexp" "^%([-a-z0-9]+%.%)+%(gov%|edu%|com%|org%|int%|mil%|net%|%nato%|arpa%|[a-z][a-z]%)$" rc
@prop #33333."maildrop" "sandbox.xerox.com" rc
@prop #33333."trusts" {} r
@prop #33333."reply_address" "moomail@sandbox.xerox.com" rc
"set $network.reply_address to return address for mail back to the MOO
@prop #33333."active" 1 r
@prop #33333."valid_email_regexp" "^[-a-z0-9_!.]+$" rc
@prop #33333."invalid_userids" {} r
;#33333.("invalid_userids") = {"", "sysadmin", "root", "postmaster", "system", "operator", "bin"}
@prop #33333."debugging" 0 rc
;#33333.("description") = {"Utilities for dealing with network connections", "---------------", "Creating & tracking hosts:", "", ":open(host, port [, connect-connection-to]) => {connection, object}", "    open a network connection (using open_network_connection), optionally", "    allows for it to be connected to another object.", "    (see #0:do_login_command for details).", "", ":close(connection)", "     closes the connection & cleans up data", "", "------------------", "Parsing network things:", "", ":invalid_email_address(email)", "     return \"\" or string saying why 'email' is invalid.", "     uses .valid_email_regexp", "", ":invalid_hostname(host)", "     return \"\" or string saying why 'host' doesn't look", "     like a valid internet host name", "", ":local_domain(host)", "     returns the 'important' part of a host name, e.g.", "     golden.parc.xerox.com => parc.xerox.com", "", "-------------------", "Sending mail", "", ":sendmail(to, subject, @lines)", "     send mail to the email address 'to' with indicated subject.", "     header fields like 'from', 'date', etc. are filled in.", "     lines can start with additional header lines.", "", ":raw_sendmail(to, @lines)", "     used by :sendmail. Send mail to given user at host, just", "     as specified, no error checking.", "", "================================================================", "Parameters:", "", ".active If 0, disabled sending of mail.", "", ".site   Where does this MOO run?", "        (Maybe MOOnet will use it later).", "", ".port   The network port this MOO listens on.", "", ".large_domains ", "        A list of sites where more than 2 levels of host name are", "        significant, e.g., if you want 'parc.xerox.com' to be", "        different than 'cinops.xerox.com', put \"xerox.com\" as an", "        element in .large_domains.", "", ".postmaster", "        Email address to which problems with MOO mail should", "        go. This should be a real email address that someone reads.", "", ".maildrop", "        Hostname to connect to for dropping off mail. Usually can", "        just be \"localhost\".", "", ".reply_address", "        If a MOO character sends email, where does a reply go?", "        Inserted in 'From:' for mail from characters without", "        registration addresses.        ", "", ".trusts", "        List of (non-wizard) programmers who can call", "        :open, :sendmail, :close", "", "                "}
;#33333.("object_size") = {11843, 741006149}

@verb #33333:"parse_address" this none this
@program #33333:parse_address
"Given an email address, return {userid, site}.";
"Valid addresses are of the form `userid[@site]'.";
"At least for now, if [@site] is left out, site will be returned as blank.";
"Should be a default address site, or something, somewhere.";
address = args[1];
return (at = index(address, "@")) ? {address[1..at - 1], address[at + 1..length(address)]} | {address, ""};
.

@verb #33333:"local_domain" this none this
@program #33333:local_domain
"given a site, try to figure out what the `local' domain is.";
"if site has a @ or a % in it, give up and return E_INVARG.";
"blank site is returned as is; try this:local_domain(this.localhost) for the answer you probably want.";
site = args[1];
if (index(site, "@") || index(site, "%"))
  return E_INVARG;
elseif (match(site, "^[0-9.]+$"))
  return E_INVARG;
elseif (!site)
  return "";
elseif (!(dot = rindex(site, ".")))
  dot = rindex(site = this.site, ".");
endif
if ((!dot) || (!(dot = rindex(site[1..dot - 1], "."))))
  return site;
else
  domain = site[dot + 1..length(site)];
  site = site[1..dot - 1];
  while (site && (domain in this.large_domains))
    if (dot = rindex(site, "."))
      domain = tostr(site[dot + 1..length(site)], ".", domain);
      site = site[1..dot - 1];
    else
      return tostr(site, ".", domain);
    endif
  endwhile
  return domain;
endif
.

@verb #33333:"open" this none this rx
@program #33333:open
":open(address, port, [connect-connection-to])";
"Open a network connection to address/port.  If the connect-connection-to is passed, then the connection will be connected to that object when $login gets ahold of it.  If not, then the connection is just ignored by $login, i.e. not bothered by it with $welcome_message etc.";
"The object specified by connect-connection-to has to be a player (though it need not be a $player).";
"Returns the (initial) connection or an error, as in open_network_connection";
if (!this:trust(forwhom = caller_perms()))
  return E_PERM;
endif
address = args[1];
port = args[2];
if (length(args) < 3)
  connect_to = $nothing;
elseif ((typeof(connect_to = args[3]) == OBJ) && (valid(connect_to) && is_player(connect_to)))
else
  return E_INVARG;
endif
if (typeof(connection = open_network_connection(address, port)) != ERR)
  this.open_connections = {@this.open_connections, connection};
  if (valid(connect_to))
    this.connect_connections_to = {@this.connect_connections_to, {connection, connect_to}};
  endif
endif
return connection;
.

@verb #33333:"close" this none this rx
@program #33333:close
if (!this:trust(caller_perms()))
  return E_PERM;
endif
boot_player(args[1]);
$login.ignored = setremove($login.ignored, args[1]);
$network.open_connections = setremove($network.open_connections, args[1]);
if (i = $list_utils:iassoc(args[1], $network.connect_connections_to))
  $network.connect_connections_to = listdelete($network.connect_connections_to, i);
endif
return 1;
.

@verb #33333:"sendmail" any none none rxd
@program #33333:sendmail
"sendmail(to, subject, @lines)";
"  sends mail to internet address 'to', with given subject.";
"  It fills in various fields, such as date, from (from player), etc.";
"  lines are remaining lines of the message, and may begin with additional header fields.";
"  (must match RFC822 specification).";
"Requires $network.trust to call (no anonymous mail from MOO).";
"Returns 0 if successful, or else error condition or string saying why not.";
if (!this:trust(caller_perms()))
  return E_PERM;
endif
mooname = this.MOO_name;
mooinfo = tostr(mooname, " (", this.site, " ", this.port, ")");
if (reason = this:invalid_email_address(to = args[1]))
  return reason;
endif
return this:raw_sendmail(to, "Date: " + ctime(), ((((("From: \"" + player.name) + "@") + mooname) + "\" <") + this.reply_address) + ">", "To: " + to, "Subject: " + args[2], "X-Mail-Agent: " + mooinfo, @args[3..length(args)]);
.

@verb #33333:"trust" this none this
@program #33333:trust
return (who = args[1]).wizard || (who in this.trusts);
.

@verb #33333:"init_for_core" this none this
@program #33333:init_for_core
if (caller_perms().wizard)
  pass(@args);
  this.active = 0;
  this.reply_address = "moomailreplyto@yourhost";
  this.site = "yoursite";
  this.postmaster = "postmastername@yourhost";
  this.MOO_name = "YourMOO";
  this.maildrop = "localhost";
  this.port = 7777;
  this.large_domains = {};
  this.trusts = {};
  this.open_connections = (this.connect_connections_to = {});
endif
.

@verb #33333:"raw_sendmail" any none none rx
@program #33333:raw_sendmail
"rawsendmail(to, @lines)";
"sends mail without processing. Returns 0 if successful, or else reason why not.";
if (!caller_perms().wizard)
  return E_PERM;
endif
if (!this.active)
  return "Networking is disabled.";
endif
debugging = this.debugging;
address = args[1];
body = listdelete(args, 1);
data = {"HELO " + this.site, ("MAIL FROM:<" + this.postmaster) + ">", ("RCPT TO:<" + address) + ">", "DATA"};
blank = 0;
for x in (body)
  $command_utils:suspend_if_needed(0);
  if (!(blank || match(x, "[a-z0-9-]*: ")))
    if (x)
      data = {@data, ""};
    endif
    blank = 1;
  endif
  data = {@data, (x == ".") ? "." + x | x};
endfor
data = {@data, ".", "QUIT", ""};
suspend(0);
target = $network:open(this.maildrop, 25);
if (typeof(target) == ERR)
  return tostr("Cannot open connection to maildrop ", this.maildrop, ": ", target);
endif
fork (0)
  for line in (data)
    $command_utils:suspend_if_needed(0);
    if (debugging)
      notify(this.owner, "SEND:" + line);
    endif
    notify(target, line);
  endfor
endfork
expect = {"2", "2", "2", "2", "3", "2"};
while (expect && (typeof(line = read(target)) != ERR))
  if (line)
    if (debugging)
      notify(this.owner, "GET: " + line);
    endif
    if (!index("23", line[1]))
      $network:close(target);
      return line;
      "error return";
    else
      if (line[1] != expect[1])
        expect = {@expect, "2", "2", "2", "2", "2"};
      else
        expect = listdelete(expect, 1);
      endif
    endif
  endif
endwhile
$network:close(target);
return 0;
.

@verb #33333:"invalid_email_address" this none this
@program #33333:invalid_email_address
"invalid_email_address(email) -- check to see if email looks like a valid email address. Return reason why not.";
address = args[1];
if (!(at = rindex(address, "@")))
  return ("'" + address) + "' contains no @";
endif
name = address[1..at - 1];
host = address[at + 1..length(address)];
if (!match(host, $network.valid_host_regexp))
  return tostr("'", host, "' doesn't look like a valid internet host");
endif
if (!match(name, $network.valid_email_regexp))
  return tostr("'", name, "' doesn't look like a valid user name for internet mail");
endif
return "";
.

@verb #33333:"invalid_hostname" this none this
@program #33333:invalid_hostname
return match(args[1], this.valid_host_regexp) ? "" | tostr("'", args[1], "' doesn't look like a valid internet host name");
.

"***finished***
