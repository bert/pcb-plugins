# Compile single source files against the pcb source tree.

CC=gcc

CFLAGS=-g -Wall -I../../../pcb/src -I../../../pcb -O2 -shared

%.so: %.c
	${CC} ${CFLAGS} $< -o $@

.DEFAULT_GOAL := all

# Individual target definitions.
autocrop: autocrop.so
	
boardflip: boardflip.so
	
distalign: distalign.so
	
findelement: findelement.so
	
findrat: findrat.so
	
ghellopcb: ghellopcb.so
	${CC} ${CFLAGS} -lglib-2.0 -o ghellopcb.so ghellopcb.c `pkg-config --cflags gtk+-2.0 --libs gtk+-2.0`

jostle: jostle.so
	
lockelements: lockelements.so
	
renumberblock: renumberblock.so
	
sedrename: sedrename.so
	
smartdisperse: smartdisperse.so
	
teardrops: teardrops.so
	
upth2pth: upth2pth.so
	
#
all: \
	autocrop \
	boardflip \
	distalign \
	findelement \
	findrat \
	jostle \
	lockelements \
	renumberblock \
	sedrename \
	smartdisperse \
	teardrops \
	upth2pth

install:
	mkdir -p ~/.pcb/plugins
	mv *.so ~/.pcb/plugins

clean:
	rm -f *.so *~

# EOF #

