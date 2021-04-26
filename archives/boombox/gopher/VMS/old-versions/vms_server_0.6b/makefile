# makefile for vms gopher server

server : vmsgopherd.exe

vmsgopherd.exe : vmsgopherd.obj special.obj gopherstruct.obj error.obj \
                 util.obj globals.obj 
   link /exe=vmsgopherd.exe vmsgopherd.obj,special.obj,gopherstruct.obj,-
        error.obj,globals.obj,util.obj,crtl.opt/opt

special.obj : special.c
   cc /obj=special.obj special.c

gopherstruct.obj : gopherstruct.c
   cc /obj=gopherstruct.obj gopherstruct.c

error.obj : error.c
   cc /obj=error.obj error.c

util.obj : util.c
   cc /obj=util.obj util.c /def=(WOLLONGONG)

globals.obj : globals.c
   cc /obj=globals.obj globals.c

vmsgopherd.obj : vmsgopherd.c 
   cc /obj=vmsgopherd.obj vmsgopherd.c /def=(WOLLONGONG)
