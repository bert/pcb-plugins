/* Teardrops plug-in for PCB
   http://www.delorie.com/pcb/teardrops/

   Copyright (C) 2006 DJ Delorie <dj@delorie.com>

   Licensed under the terms of the GNU General Public License, version
   2 or later.
*/

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "create.h"
#include "rtree.h"
#include "undo.h"

static PinType *pin;
static int layer;
static int px, py;
static LayerType *silk;

static int new_arcs = 0;

static int
check_line_callback (const BoxType * box, void *cl)
{
  LayerType *lay = & PCB->Data->Layer[layer];
  LineType *l = (LineType *) box;
  int x1, x2, y1, y2;
  double a, b, c, x, r, t;
  double dx, dy, len;
  double ax, ay, lx, ly, theta;
  double ldist, adist, radius;
  double vx, vy, vr, vl;
  int delta, aoffset, count;
  ArcType *arc;

  if (l->Point1.X == px && l->Point1.Y == py)
    {
      x1 = l->Point1.X;
      y1 = l->Point1.Y;
      x2 = l->Point2.X;
      y2 = l->Point2.Y;
    }
  else if (l->Point2.X == px && l->Point2.Y == py)
    {
      x1 = l->Point2.X;
      y1 = l->Point2.Y;
      x2 = l->Point1.X;
      y2 = l->Point1.Y;
    }
  else
    return 1;

  r = pin->Thickness / 2.0;
  t = l->Thickness / 2.0;

  if (t > r)
    return 1;

  a = 1;
  b = 4 * t - 2 * r;
  c = 2 * t * t - r * r;

  x = (-b + sqrt (b * b - 4 * a * c)) / (2 * a);

  len = sqrt (((double)x2-x1)*(x2-x1) + ((double)y2-y1)*(y2-y1));

  if (len > (x+t))
    {
      adist = ldist = x + t;
      radius = x + t;
      delta = 45;

      if (radius < r || radius < t)
	return 1;
    }
  else if (len > r + t)
    {
      /* special "short teardrop" code */

      x = (len*len - r*r + t*t) / (2 * (r - t));
      ldist = len;
      adist = x + t;
      radius = x + t;
      delta = atan2 (len, x + t) * 180.0/M_PI;
    }
  else
    return 1;

  dx = ((double)x2 - x1) / len;
  dy = ((double)y2 - y1) / len;
  theta = atan2 (y2 - y1, x1 - x2) * 180.0/M_PI;

  lx = px + dx * ldist;
  ly = py + dy * ldist;

  /* We need one up front to determine how many segments it will take
     to fill.  */
  ax = lx - dy * adist;
  ay = ly + dx * adist;
  vl = sqrt (r*r - t*t);
  vx = px + dx * vl;
  vy = py + dy * vl;
  vx -= dy * t;
  vy += dx * t;
  vr = sqrt ((ax-vx) * (ax-vx) + (ay-vy) * (ay-vy));

  aoffset = 0;
  count = 0;
  do {
    if (++count > 5)
      {
	printf("a %d,%d v %d,%d adist %g radius %g vr %g\n",
	       (int)ax, (int)ay, (int)vx, (int)vy, adist, radius, vr);
	return 1;
      }

    ax = lx - dy * adist;
    ay = ly + dx * adist;

    arc = CreateNewArcOnLayer (lay, (int)ax, (int)ay, (int)radius, (int)radius,
			       (int)theta+90+aoffset, delta-aoffset,
			       l->Thickness, l->Clearance, l->Flags);
    if (arc)
      AddObjectToCreateUndoList (ARC_TYPE, lay, arc, arc);

    ax = lx + dy * (x+t);
    ay = ly - dx * (x+t);

    arc = CreateNewArcOnLayer (lay, (int)ax, (int)ay, (int)radius, (int)radius,
			       (int)theta-90-aoffset, -delta+aoffset,
			       l->Thickness, l->Clearance, l->Flags);
    if (arc)
      AddObjectToCreateUndoList (ARC_TYPE, lay, arc, arc);

    radius += t*1.9;
    aoffset = acos ((double)adist / radius) * 180.0 / M_PI;

    new_arcs ++;
  } while (vr > radius - t);

  return 1;
}

static void
check_pin (PinType *_pin)
{
  BoxType spot;

  pin = _pin;

  px = pin->X;
  py = pin->Y;

  spot.X1 = px - 1;
  spot.Y1 = py - 1;
  spot.X2 = px + 1;
  spot.Y2 = py + 1;

  for (layer = 0; layer < max_copper_layer; layer ++)
    {
      LayerType *l = &(PCB->Data->Layer[layer]);
      r_search (l->line_tree, &spot, NULL, check_line_callback, l);
    }
}

static int
teardrops (int argc, char **argv, Coord x, Coord y)
{
  silk = & PCB->Data->SILKLAYER;

  new_arcs = 0;

  VIA_LOOP (PCB->Data);
  {
    check_pin (via);
  }
  END_LOOP;

  ALLPIN_LOOP (PCB->Data);
  {
    check_pin (pin);
  }
  ENDALL_LOOP;

  gui->invalidate_all ();

  if (new_arcs)
    IncrementUndoSerialNumber ();

  return 0;
}

static HID_Action teardrops_action_list[] = {
  {"Teardrops", NULL, teardrops,
   NULL, NULL}
};

REGISTER_ACTIONS (teardrops_action_list)

void
hid_teardrops_init()
{
  register_teardrops_action_list();
}
