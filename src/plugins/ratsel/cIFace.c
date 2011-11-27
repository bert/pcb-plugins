/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

/*
 necessity of the .c interface:
 It would be better to only have a single C++ interface for connecting PCB and
 the Java plugin. But I had compile errors when I tried to inlcude PCB headers
 from C++ even with playing extern "C" includes.
*/


#include <stdio.h>
#include <stdlib.h>


#include "cIFace.h"
#include "cppIFace.hpp"

#include "config.h"
#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "error.h"
#include "draw.h"
#include "undo.h"

/*
	see macro.h const.h
	ONSOLDERFLAG - Pad should have it

	TEST_FLAG (ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER
	int layer = GetLayerNumber (PCB->Data, (LayerTypePtr) ptr1);
	const.h:#define SOLDER_LAYER            0
	const.h:#define COMPONENT_LAYER         1
 */

void msg(const char * fmt,...) {
	va_list ap;
	va_start(ap, fmt);
	Message(fmt, ap);
}

int getNetNameN() {
	LibraryType lib = PCB->NetlistLib;
	return lib.MenuN;
}

const char * getNetName(int idx) {
	LibraryType lib = PCB->NetlistLib;
	return lib.Menu[idx].Name;
}

int getPinNameN(int netNameIdx) {
	LibraryMenuType menu = PCB->NetlistLib.Menu[netNameIdx];
	return menu.EntryN;
}

const char * getPinName(int netNameIdx, int pinIdx) {
	LibraryMenuType menu = PCB->NetlistLib.Menu[netNameIdx];
	LibraryEntryType entry = menu.Entry[pinIdx];
	return entry.ListEntry;
}

#define VEC_INC	32

typedef struct Vec {
	int rawSize, size;
	void **data;
} Vec;

static void vec_init(Vec *vec) {
	vec->rawSize = vec->size = 0;
	vec->data = NULL;
}

static void vec_append(Vec *vec, void *ptr) {
	if (vec->size == vec->rawSize) {
		vec->rawSize += VEC_INC;
		vec->data = realloc(vec->data, vec->rawSize*sizeof(void*));
	}
	vec->data[vec->size++] = ptr;
}

CPin ** getPins() {
	Vec pinVec;
	vec_init(&pinVec);
	ELEMENT_LOOP(PCB->Data); {
		PIN_LOOP(element); {
			CPin * myPin = (CPin*)malloc(sizeof(CPin));
			myPin->eName = ELEMENT_NAME(PCB, element);
			myPin->num = atoi(pin->Number);
			myPin->seq = 0;
			myPin->x = pin->X;
			myPin->y = pin->Y;
			myPin->sideMask = SIDE_BOTH;
			vec_append(&pinVec, myPin);
		} END_LOOP;
		PAD_LOOP(element); {
			char * name = ELEMENT_NAME(PCB, element);
			int num = atoi(pad->Number);
			int sideMask = TEST_FLAG (ONSOLDERFLAG, pad) ? SIDE_SOLDER : SIDE_COMPONENT;
			CPin * myPin = (CPin*)malloc(sizeof(CPin));
			myPin->eName = name;
			myPin->num = num;
			myPin->seq = 0;
			myPin->x = pad->Point1.X;
			myPin->y = pad->Point1.Y;
			myPin->sideMask = sideMask;
			vec_append(&pinVec, myPin);
			myPin = (CPin*)malloc(sizeof(CPin));
			myPin->eName = name;
			myPin->num = num;
			myPin->seq = 1;
			myPin->x = pad->Point2.X;
			myPin->y = pad->Point2.Y;
			myPin->sideMask = sideMask;
			vec_append(&pinVec, myPin);
		} END_LOOP;
	} END_LOOP;	
	vec_append(&pinVec, NULL);
	return (CPin**)pinVec.data;
}

void freePins(CPin ** pinA) {
	int idx = 0;
	if (pinA == NULL) return;
	while (pinA[idx]) free(pinA[idx++]);
	free(pinA);
}

/* FIXME: this layer mapping may not be right */
#define SIDEMASK(group) \
	group == COMPONENT_LAYER ? SIDE_SOLDER : SIDE_COMPONENT;

CRat ** getRats() {
	Vec ratVec;
	vec_init(&ratVec);
	RAT_LOOP(PCB->Data); {
		CRat * myRat = (CRat*)malloc(sizeof(CRat));
		myRat->id = n;
		myRat->x0 = line->Point1.X;
		myRat->y0 = line->Point1.Y;
		myRat->sideMask0 = SIDEMASK(line->group1);
		myRat->x1 = line->Point2.X;
		myRat->y1 = line->Point2.Y;
		myRat->sideMask1 = SIDEMASK(line->group2);
		vec_append(&ratVec, myRat);
	} END_LOOP;
	vec_append(&ratVec, NULL);
	return (CRat**)ratVec.data;
}

void freeRats(CRat ** ratA) {
	int idx = 0;
	if (ratA == NULL) return;
	while (ratA[idx]) free(ratA[idx++]);
	free(ratA);
}

void selectRat(int idx) {
	/*
	type = SearchScreen (Crosshair.X, Crosshair.Y, SELECT_TYPES,
		       &ptr1, &ptr2, &ptr3);
	 */
	/* FIXME is ptr1 a layer? */
	RatTypePtr rat = &(PCB->Data)->Rat[idx];
	/* TODO: add undo FIXME: how? */
	/* AddObjectToFlagUndoList (RATLINE_TYPE, ptr1, ptr1, ptr1); */
	TOGGLE_FLAG (SELECTEDFLAG, rat);
	DrawRat (rat);
}

static int ratSel(int argc, char **argv, int x, int y) {
	Message("C:RatSel called\n");
	ratSelCpp(argc, (const char**)argv, x, y);
	return 0;
}

static HID_Action ratSelActionList[] = {
	{ "RatSel", NULL, ratSel, NULL, NULL }
};

REGISTER_ACTIONS (ratSelActionList)

void pcb_plugin_init() {
	register_ratSelActionList();
	Message("cIFace.c:RatSel plugin loaded!\n");
	initJava();
}
