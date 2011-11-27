include Make.Settings

CFLAGS=-O2 -Wall -I$(PCBDIR) -I$(PCBDIR)/src -I. -shared `pkg-config --cflags glib-2.0`
CPPFLAGS=-O2 -Wall -I.
LFLAGS=-lstdc++ -lgcj

CP=$(LIBGCJPATH):src/:src/ratsel/:./
INC=-Isrc
COBJS=$(shell ./getFileNames.pl . o c cpp)
JOBJS=$(shell ./getFileNames.pl src o java)
OBJS=$(COBJS) $(JOBJS)
HDRS=$(shell ./getFileNames.pl . h c) $(shell ./getFileNames.pl . hpp cpp)

JAVAS=$(shell find src/ratsel/ -name '*.java')
JHDRS=src/ratsel/JIFace.h src/ratsel/Location.h src/ratsel/Pin.h src/ratsel/Rat.h

plugin: RatSel.so

RatSel.so: JIFace $(OBJS)
	g++ $(LFLAGS) $(OBJS) -shared -o RatSel.so

%.o:%.c
	gcc -c $(CFLAGS) $< -o $@

%.o:%.cpp 
	g++ -c $(CPPFLAGS) $(INC) $< -o $@

install: plugin
	cp RatSel.so $(HOME)/.pcb/plugins/.

inst: clean install

java:
	gcj -C $(JAVAS)

src/ratsel/%.class: src/ratsel/%.java
	gcj $(INC) -C $<

src/ratsel/%.h: src/ratsel/%.class
	gcjh -classpath $(CP) $< -o $@

JIFace: $(JHDRS)
	
test: src/ratsel/JIFace.java
	gcj --main=ratsel.JIFace src/ratsel/JIFace.java -o JIFace

src/ratsel/%.o: src/ratsel/%.java
	gcj $(INC) -c $< -o $@

cleanJIFace: cleanClasses
	rm -f src/ratsel/*.h

cleanClasses:
	rm -f *.o src/ratsel/*.class utst/ratsel/*.class

clean: cleanJIFace
	rm -f *.o *~ RatSel.so src/ratsel/*.o

tar: clean
	tar -cjf ../RatSel.tar.bz2 *

