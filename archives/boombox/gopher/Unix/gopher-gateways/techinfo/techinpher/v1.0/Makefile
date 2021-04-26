CFLAGS = -DSERV -DTEST -DDECMIPS -g

TECHINPHER_SOURCES=\
 transact.c server.c datastruct.c glob.c util.c inet.c netio.c gopherchild.c
techinpher_objs=\
 transact.o server.o datastruct.o glob.o util.o inet.o netio.o gopherchild.o

HELPER_SOURCES = techinpher-helper.c inet.c
helper_objs=     techinpher-helper.o inet.o

SOURCES=$(HELPER_SOURCES) $(TECHINPHER_SOURCES) 

all: techinpher techinpher-helper

techinpher: $(techinpher_objs)
	cc -o techinpher $(techinpher_objs)

techinpher-helper: $(helper_objs)
	cc -o techinpher-helper $(helper_objs)

clean:
	rm -f *.o  *~  techinpher techinpher-helper core

depend:
		makedepend $(CFLAGS) $(SOURCES)
		@echo "" >> Makefile
		@echo -n "# Last dependency update: " >> Makefile
		@date >> Makefile

# DO NOT DELETE THIS LINE -- make depend depends on it.
