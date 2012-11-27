#
# Makefile
#
# Copyright 2008 Dean Ferreyra <dferreyra@igc.org>, All rights reserved
#
# This file is part of Footprint-Update.
#
# Footprint-Update is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# Footprint-Update is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Footprint-Update.  If not, see <http://www.gnu.org/licenses/>.
#
# $Id: Makefile,v 1.12 2008-05-25 00:16:06 dean Exp $

# Location of PCB source tree
PCB_CVS_DIR=../../../../pcb

# Installation location of PCB plug-ins
INSTALL_DIR=~/.pcb/plugins

#MY_VERSION=1.01

# DEBUG_FLAGS=-DDEBUG=1
CFLAGS=-Wall -O2 -shared `pkg-config --cflags glib-2.0`
#        -Werror -fPIC $(DEBUG_FLAGS) -DVERSION='"$(VERSION)"'

all: footprintupdate.so

%.o: %.c
	$(CC) $(CFLAGS) -I$(PCB_CVS_DIR)/src -I$(PCB_CVS_DIR) -c $< -o $@

footprintupdate.so: footprintupdate.o utilities.o matrix.o pad-pin-data.o
	$(CC) $(CFLAGS) -I$(PCB_CVS_DIR)/src -I$(PCB_CVS_DIR) $+ -o $@

footprintupdate.o: footprintupdate.c pad-pin-data.h utilities.h
pad-pin-data.o: pad-pin-data.c pad-pin-data.h matrix.h utilities.h
matrix.o: matrix.c matrix.h utilities.h
utilities.o: utilities.c utilities.h


.PHONY : install
install: footprintupdate.so
	install -m755 -d $(INSTALL_DIR)
	install -m755 $+ $(INSTALL_DIR)/

.PHONY : clean
clean:
	rm -f *.o *.so

DIST_DIR=footprint-update-$(VERSION)
DIST_FILE=$(DIST_DIR).tar.gz

.PHONY : dist
dist:
	rm -rf $(DIST_FILE) $(DIST_DIR)
	mkdir $(DIST_DIR)
	cp README \
           COPYING \
           Makefile \
           footprintupdate.c \
           pad-pin-data.[ch] \
           matrix.[ch] \
           utilities.[ch] \
           $(DIST_DIR)
	tar -czf $(DIST_FILE) $(DIST_DIR)

.PHONY : uninstall
uninstall:
	rm ~/.pcb/plugins/footprintupdate.so
