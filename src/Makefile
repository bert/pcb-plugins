# Compile single source files against the pcb source tree.

CC=gcc

CFLAGS=-g -Wall -I../../../pcb/src -I../../../pcb -O2 -shared

%.so: %.c
	${CC} ${CFLAGS} $< -o $@

# Individual target definitions.
autocrop: autocrop.so
	
boardflip: boardflip.so
	
distalign: distalign.so
	
findrat: findrat.so
	
jostle: jostle.so
	
lockelements: lockelements.so
	
renumberblock: renumberblock.so
	
sedrename: sedrename.so
	
smartdisperse: smartdisperse.so
	
teardrops: teardrops.so
	
#
all: \
	autocrop \
	boardflip \
	distalign \
	findrat \
	jostle \
	lockelements \
	renumberblock \
	sedrename \
	smartdisperse \
	teardrops

install:
	mv *.so ~/.pcb/plugins

clean:
	rm -f *.so *~

# EOF #

