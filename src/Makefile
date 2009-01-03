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
	distalign.so \
	findrat.so \
	jostle.so \
	lockelements.so \
	renumberblock.so \
	sedrename.so \
	smartdisperse.so \
	teardrops.so

install:
	mv *.so ~/.pcb/plugins

clean:
	rm -f *.so *~

# EOF #

