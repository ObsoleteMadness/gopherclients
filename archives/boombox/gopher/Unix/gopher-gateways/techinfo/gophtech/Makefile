PUBDIR=/usr/ftp/pub/gophtech
CFLAGS=-g

TARGETS = gophtech

GOTIGW_OBJS=gophtech.o

all: $(TARGETS)

pub:
	cp [A-Z]* *.c *.h stats cleanuplog $(PUBDIR)
	chmod ugo+r $(PUBDIR)/*

clean:
	rm -f *.o
	rm -f $(TARGETS)
	rm -f *~

