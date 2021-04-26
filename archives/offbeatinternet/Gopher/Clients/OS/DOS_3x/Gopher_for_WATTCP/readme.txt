+++ COPIED FROM FDISK.COM +++


I needed a break from DOSLYNX that would give me a warm fuzzy feeling 
about my programming abilities and a perspective on what some of us 
want for a Lynx in the DOS world. So, I dug through my old source code
and found UGOPHER. This was a DOS port of Curses Gopher for people who 
own a commercial tcp/ip stack (PC/TCP, Lan Workplace for DOS and PCNFS).

Located here are a bunch of ZIP and LZH files used for the project. 
GOPHER-S.ZIP is the hacked source for building the WATTCP version. This
is ALL you should need to get it compiled with BC++ 3.1. If you want to
see the result, you only need GOPHER-B.ZIP. Everything else is SRC used
to get from point a to point b.

This port is unsupported and unwarrenteed. Use at your own risk. You can
still ask me questions, just don't expect the kind of responce you would
get if I was being paid.


Porting notes:

I butchered the source a bit getting it to work with BC++3.1 and WATTCP.
Not bad though...a diff of the old/new source and a couple of #defines 
could easily fix it. I actually had to make a couple changes after 
getting it working so that it wouldn't crash. I suspect the original 
product was buggy.


Final note:

This port required changes to make it compatable with BC++ 3.1, a 
different curses library and WATTCP. I completed the port in ONE DAY. 
Expect bugs, maybe even killer ones. Still, I am pretty proud of myself
getting it going in that short of time.


Wayne.

