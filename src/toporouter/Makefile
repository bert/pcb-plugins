CC = gcc
PCBPATH = ../../pcb

CFLAGS = -g -I$(PCBPATH)/src -I$(PCBPATH) -O2 `pkg-config gtk+-2.0 --cflags --libs` -std=c99 -Wall

INSTALLPATH = ~/.pcb/plugins
SOURCES = triangle.c toporouter.c
FLAGS = 

all: toporouter

toporouter: $(SOURCES) 
	$(CC) -fPIC -shared -o toporouter.so $(CFLAGS) $(SOURCES) -DDEBUG $(FLAGS)

test: $(SOURCES)
	$(CC) -o test $(CFLAGS) $(SOURCES) -DTEST -DTRACE -g -pg $(FLAGS)

trace.avi: 
	mencoder "mf://trace/*.png" -mf fps=25 -o trace.avi -ovc lavc -lavcopts vcodec=mpeg4

tarball:
	mkdir tar
	cp *.c *.h Makefile ./tar
	tar -cvf tar.tar ./tar
	gzip -v9 tar.tar
	rm -rf tar

install: toporouter
	cp toporouter.so $(INSTALLPATH)	

clean: 
	rm -rf *.o *.avi toporouter.so trace/* test tar.tar.gz 

