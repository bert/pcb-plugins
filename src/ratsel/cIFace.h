#ifndef CIFACE_H
#define CIFACE_H

/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

#include <stdarg.h>

#define SIDE_SOLDER		1
#define SIDE_COMPONENT	2
#define SIDE_BOTH		(SIDE_SOLDER|SIDE_COMPONENT)

typedef struct CPin {
	const char *eName;
	int num, seq, x, y;
	int sideMask;
} CPin;

typedef struct CRat {
	int id, x0, y0, sideMask0, x1, y1, sideMask1;
} CRat;

extern void msg(const char * fmt,...);

extern int getNetNameN();
extern const char * getNetName(int idx);
extern int getPinNameN(int netNameIdx);
extern const char * getPinName(int netNameIdx, int pinIdx);

extern CPin ** getPins();
extern void freePins(CPin ** pinA);

extern CRat ** getRats();
extern void freeRats(CRat ** ratA);

extern void selectRat(int idx);

#endif
