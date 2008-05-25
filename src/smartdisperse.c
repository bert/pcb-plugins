/* smartdisperse plug-in for PCB

   Improve the initial dispersion of elements by choosing an order based
   on the netlist, rather than the arbitrary element order.  This isn't
   the same as a global autoplace, it's more of a linear autoplace.  It
   might make some useful local groupings.  For example, you should not
   have to chase all over the board to find the resistor that goes with
   a given LED.

   Copyright (C) 2007 Ben Jackson <ben@ben.com> based on teardrops.c by
   Copyright (C) 2006 DJ Delorie <dj@delorie.com>
   as well as the original action.c, and autoplace.c

   Licensed under the terms of the GNU General Public License, version
   2 or later.
*/

#include <stdio.h>
#include <math.h>

#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "create.h"
#include "rtree.h"
#include "undo.h"
#include "rats.h"

#define GAP 10000
static LocationType minx;
static LocationType miny;
static LocationType maxx;
static LocationType maxy;

/*
 * Place one element.  Must initialize statics above before calling for
 * the first time.
 *
 * This is taken almost entirely from ActionDisperseElements, with cleanup
 */
static void
place(ElementTypePtr element)
{
	LocationType dx, dy;

	/* figure out how much to move the element */
	dx = minx - element->BoundingBox.X1;
	dy = miny - element->BoundingBox.Y1;

	/* snap to the grid */
	dx -= (element->MarkX + dx) % (long) (PCB->Grid);
	dx += (long) (PCB->Grid);
	dy -= (element->MarkY + dy) % (long) (PCB->Grid);
	dy += (long) (PCB->Grid);

	/*
	 * and add one grid size so we make sure we always space by GAP or
	 * more
	 */
	dx += (long) (PCB->Grid);

	/* Figure out if this row has room.  If not, start a new row */
	if (minx != GAP && GAP + element->BoundingBox.X2 + dx > PCB->MaxWidth) {
		miny = maxy + GAP;
		minx = GAP;
		place(element);		/* recurse can't loop, now minx==GAP */
		return;
	}

	/* move the element */
	MoveElementLowLevel (PCB->Data, element, dx, dy);

	/* and add to the undo list so we can undo this operation */
	AddObjectToMoveUndoList (ELEMENT_TYPE, NULL, NULL, element, dx, dy);

	/* keep track of how tall this row is */
	minx += element->BoundingBox.X2 - element->BoundingBox.X1 + GAP;
	if (maxy < element->BoundingBox.Y2) {
		maxy = element->BoundingBox.Y2;
	}
}

/*
 * Return the X location of a connection's pad or pin within its element
 */
static LocationType
padDX(ConnectionTypePtr conn)
{
	ElementTypePtr element = (ElementTypePtr) conn->ptr1;
	AnyLineObjectTypePtr line = (AnyLineObjectTypePtr) conn->ptr2;

	return line->BoundingBox.X1 -
			(element->BoundingBox.X1 + element->BoundingBox.X2) / 2;
}

/* return true if ea,eb would be the best order, else eb,ea, based on pad loc */
static int
padorder(ConnectionTypePtr conna, ConnectionTypePtr connb)
{
	LocationType dxa, dxb;

	dxa = padDX(conna);
	dxb = padDX(connb);
	/* there are other cases that merit rotation, ignore them for now */
	if (dxa > 0 && dxb < 0)
		return 1;
	return 0;
}

/* ewww, these are actually arrays */
#define ELEMENT_N(DATA,ELT)	((ELT) - (DATA)->Element)
#define VISITED(ELT)		(visited[ELEMENT_N(PCB->Data, (ELT))])
#define IS_ELEMENT(CONN)	((CONN)->type == PAD_TYPE || (CONN)->type == PIN_TYPE)

#define ARG(n) (argc > (n) ? argv[n] : 0)

static const char smartdisperse_syntax[] = "SmartDisperse([All|Selected])";

static int
smartdisperse (int argc, char **argv, int x, int y)
{
	char *function = ARG(0);
	NetListTypePtr Nets;
	char *visited;
	PointerListType stack = { 0, 0, NULL };
	int all;
	int changed = 0;
	int i;

	if (! function) {
		all = 1;
	} else if (strcmp(function, "All") == 0) {
		all = 1;
	} else if (strcmp(function, "Selected") == 0) {
		all = 0;
	} else {
		AFAIL(smartdisperse);
	}

	Nets = ProcNetlist (&PCB->NetlistLib);
	if (! Nets) {
		Message (_("Can't use SmartDisperse because no netlist is loaded.\n"));
		return 0;
	}

	/* remember which elements we finish with */
	visited = MyCalloc(PCB->Data->ElementN, sizeof(*visited),
				"smartdisperse()");

	/* if we're not doing all, mark the unselected elements as "visited" */
	ELEMENT_LOOP(PCB->Data);
	{
		if (! (all || TEST_FLAG (SELECTEDFLAG, element))) {
			visited[n] = 1;
		}
	}
	END_LOOP;

	/* initialize variables for place() */
	minx = GAP;
	miny = GAP;
	maxx = GAP;
	maxy = GAP;

	/*
	 * Pick nets with two connections.  This is the start of a more
	 * elaborate algorithm to walk serial nets, but the datastructures
	 * are too gross so I'm going with the 80% solution.
	 */
	NET_LOOP(Nets);
	{
		ConnectionTypePtr	conna, connb;
		ElementTypePtr		ea, eb;
		ElementTypePtr		*epp;

		if (net->ConnectionN != 2)
			continue;

		conna = &net->Connection[0];
		connb = &net->Connection[1];
		if (!IS_ELEMENT(conna) || !IS_ELEMENT(conna))
			continue;

		ea = (ElementTypePtr) conna->ptr1;
		eb = (ElementTypePtr) connb->ptr1;

		/* place this pair if possible */
		if (VISITED(ea) || VISITED(eb))
			continue;
		VISITED(ea) = 1;
		VISITED(eb) = 1;

		/* a weak attempt to get the linked pads side-by-side */
		if (padorder(conna, connb)) {
			place(ea);
			place(eb);
		} else {
			place(eb);
			place(ea);
		}
	}
	END_LOOP;

	/* Place larger nets, still grouping by net */
	NET_LOOP(Nets);
	{
		CONNECTION_LOOP(net);
		{
			ElementTypePtr element;

			if (! IS_ELEMENT(connection))
				continue;

			element = (ElementTypePtr) connection->ptr1;

			/* place this one if needed */
			if (VISITED(element))
				continue;
			VISITED(element) = 1;
			place(element);
		}
		END_LOOP;
	}
	END_LOOP;

	/* Place up anything else */
	ELEMENT_LOOP(PCB->Data);
	{
		if (! visited[n]) {
			place(element);
		}
	}
	END_LOOP;

	free(visited);

	IncrementUndoSerialNumber();
	ClearAndRedrawOutput();
	SetChangedFlag(1);

	return 0;
}

static HID_Action smartdisperse_action_list[] = {
	{"smartdisperse", NULL, smartdisperse, NULL, NULL}
};

REGISTER_ACTIONS (smartdisperse_action_list)

void
hid_smartdisperse_init()
{
	register_smartdisperse_action_list();
}
