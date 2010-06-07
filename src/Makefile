# Compile single source files against the pcb source tree.

CC=gcc

# Test for osx install,  pass in cflags for shared libraries under OS X.
OSX_INSTALL=$(shell bash -c "if [ -n \"`which sw_vers`\" ]; then sw_vers -productName; else echo 'Not OSX'; fi" )

ifeq ($(OSX_INSTALL),Mac OS X)
	CFLAGS= -Wall -I../pcb/src -I../pcb -O2 -flat_namespace -bundle -undefined suppress
else
	CFLAGS=-g -Wall -I../../pcb/src -I../../pcb -O2 -shared
endif

%.so: %.c
	${CC} ${CFLAGS} $< -o $@

.DEFAULT_GOAL := all

# Individual target definitions.
autocrop: autocrop.so
	
boardflip: boardflip.so
	
distalign: distalign.so
	
findelement: findelement.so
	
findrat: findrat.so
	
jostle: jostle.so
	
lockelements: lockelements.so

polycombine: polycombine.so
	
polystitch: polystitch.so
	
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
	polycombine \
	polystitch \
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

uninstall:
	rm ~/.pcb/plugins/autocrop.so
	rm ~/.pcb/plugins/boardflip.so
	rm ~/.pcb/plugins/distalign.so
	rm ~/.pcb/plugins/findelement.so
	rm ~/.pcb/plugins/findrat.so
	rm ~/.pcb/plugins/jostle.so
	rm ~/.pcb/plugins/lockelements.so
	rm ~/.pcb/plugins/polycombine.so
	rm ~/.pcb/plugins/polystitch.so
	rm ~/.pcb/plugins/renumberblock.so
	rm ~/.pcb/plugins/sedrename.so
	rm ~/.pcb/plugins/smartdisperse.so
	rm ~/.pcb/plugins/teardrops.so
	rm ~/.pcb/plugins/upth2pth.so
	
# EOF #
