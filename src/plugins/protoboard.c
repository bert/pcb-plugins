/*!
 * \file protoboard.c
 *
 * \author Stephen Ecob <stephen.ecob@sioi.com.au>
 *
 * \copyright (C) 2010.
 *
 * Enable soldermask if you want shorting between each 0.1" by 0.1" node
 * and its four neighbours.
 * Even if you don't want this type of shorting you may still want to
 * turn it on to create nice openings in the solder mask.
 * Then you'll need to generate your solder mask Gerber with soldermask
 * enabled and your copper Gerbers with soldermask disabled.

 */
 
#include <stdio.h>

#define inch 1.0
#define mm (inch / 25.4)
#define mil (inch / 1000.0)
#define PCB_UNIT (inch / 100000.0)

#define PITCH (0.1 * inch)
#define HOLE_L (1.0 * mm)
#define HOLE_S (0.65 * mm)
#define GAP (10.0 * mil)
#define PLANE_GAP (5.0 * mil)
#define ANNULAR (10.0 * mil)
#define MASK_CLR (1.0 * mil)
#define POLY_CLR (10.0 * mil)
#define LINK (2.0 * mil)
#define MASK_CHAN (16.0 * mil)
#define	POWER_EDGE (25.0 * mil)
#define OCT_SOLDER_CLEAR (16.0 * mil)
#define LOTS 3000000
#define FOURLAYERS 0

static char Vias[LOTS];
static char Polys[LOTS];
static char Planes[LOTS];
static char Elements[LOTS];
static char Silks[LOTS];
static char *Via = Vias;
static char *Poly = Polys;
static char *Plane = Planes;
static char *Element = Elements;
static char *Silk = Silks;
static FILE *f;
static int ox, oy;
static double x1, x2, y1, y2;
double width;
double height;
int soldermask;
double x, y;
double xmax, ymax;
double dx, dy;
int xi, yi;
int CuPhi;
int MaskPhi;
int DrillPhi;
int PolyClr;
int toggle1;
int toggle2;
int OctClear;

void
StartFile ()
{
  f = fopen ("protoboard.pcb", "wb");
  fprintf (f, "FileVersion[20100606]\n\n");
  fprintf (f, "PCB[\"ProtoBoard\" %d0000 %d0000]\n\n",
    (int) (width * 10.0),
    (int) (height * 10.0));
  fprintf (f, "Grid[1.000000 0 0 1]\n");
  fprintf (f, "Cursor[0 0 0.000000]\n");
  fprintf (f, "PolyArea[200000000.000000]\n");
  fprintf (f, "Thermal[0.500000]\n");
  fprintf (f, "DRC[700 700 700 800 1500 700]\n");
  fprintf (f, "Flags(\"showdrc,nameonpcb,uniquename,clearnew\")\n");
  fprintf (f, "Groups(\"1,c:4,s:5\")\n");
  fprintf (f, "Styles\n");
  fprintf (f, "[\"Signal,800,3100,1500,800:Power,2500,6000,3500,1000:"
    "Fat,4000,6000,3500,1000:Skinny,700,2900,1500,700\"]\n\n");
}


void
Render ()
{
  toggle1 = 0;
#if FOURLAYERS
  OctClear = 1;
#else
  OctClear = (int) (OCT_SOLDER_CLEAR / PCB_UNIT);
#endif
  xmax = width / PCB_UNIT;
  ymax = height / PCB_UNIT;
  DrillPhi = (HOLE_L / PCB_UNIT);
  CuPhi = DrillPhi + (2.0 * ANNULAR / PCB_UNIT);
  MaskPhi = CuPhi + (2.0 * MASK_CLR / PCB_UNIT);
  for (y = 0.5 * PITCH / PCB_UNIT; y < ymax; y += PITCH / PCB_UNIT)
  {
    for (x = 0.5 * PITCH / PCB_UNIT; x < xmax; x += PITCH / PCB_UNIT)
    {
      Via += sprintf (Via, "Via[%d ", ox + (int) x);
      Via += sprintf (Via, "%d ", oy + (int) y);
      Via += sprintf (Via, "%d ", CuPhi);
      Via += sprintf (Via, "%d ", OctClear);
      Via += sprintf (Via, "%d ", MaskPhi);
      Via += sprintf (Via, "%d ", DrillPhi);
#if !FOURLAYERS
      Via += sprintf (Via, "\"\" \"octagon,thermal(0S)\"]\n");
#else
      Via += sprintf (Via, "\"\" \"octagon,thermal(0S,3S)\"]\n");
#endif
    }
  }
  DrillPhi = (HOLE_S / PCB_UNIT);
  CuPhi = DrillPhi + (2.0 * ANNULAR / PCB_UNIT);
  MaskPhi = CuPhi - (8.0 * MASK_CLR / PCB_UNIT);
  PolyClr = (2.0 * POLY_CLR / PCB_UNIT);
  yi = 0;
  for (y = 1.0 * PITCH / PCB_UNIT; y < ymax - 0.5; y += PITCH / PCB_UNIT)
  {
    toggle1 ^= 1;
    toggle2 = toggle1;
    xi = 0;
    for (x = 1.0 * PITCH / PCB_UNIT; x < xmax - 0.5; x += PITCH / PCB_UNIT)
    {
      int yo;

      yo = ((xi & 3) == 3 && (yi & 1) == 1);
      yo |= ((xi & 1) == 1 && (yi & 3) == 3);
      Via += sprintf (Via, "Via[%d ", ox + (int) x);
      Via += sprintf (Via, "%d ", oy + (int) y);
      Via += sprintf (Via, "%d ", CuPhi);
      Via += sprintf (Via, "%d ", PolyClr);
      Via += sprintf (Via, "%d ", MaskPhi);
      Via += sprintf (Via, "%d ", DrillPhi);
      toggle2 ^= 1;
#if FOURLAYERS
      Via += sprintf(Via, "\"\" \"thermal(%dS)\"]\n", 1 + toggle2);
#else
      if (yo)
        Via += sprintf(Via, "\"\" \"square,thermal(%dS)\"]\n", 3);
      else
        Via += sprintf(Via, "\"\" \"thermal(%dS)\"]\n", 3);
#endif
      ++xi;
    }
    ++yi;
  }
  if (soldermask)
  {
    /*
     * ZZ -probably want to add these shorting pad/traces on the solder
     * side also
     */
    x = 0.5 * PITCH / PCB_UNIT;
    x2 = xmax - PITCH / PCB_UNIT;
    for (y = 0.5 * PITCH / PCB_UNIT; y < ymax; y += PITCH / PCB_UNIT)
    {
      Element += sprintf (Element, "Element[\"\" \"\" \"\" \"\" %d ", ox + (int) x);
      Element += sprintf (Element, "%d 0 0 0 100 \"\"]\n(\n", oy + (int) y);
      Element += sprintf (Element, "\tPad[0 0 %d ", (int) x2);
      Element += sprintf (Element, "0 %d ", (int) (LINK / PCB_UNIT));
      Element += sprintf (Element, "0 %d ", (int) (MASK_CHAN / PCB_UNIT));
      Element += sprintf (Element, "\"1\" \"1\" \"square\"]\n)\n");
    }
    Element += sprintf (Element, "\n");
    y = 0.5 * PITCH / PCB_UNIT;
    y2 = ymax - PITCH / PCB_UNIT;
    for (x = 0.5 * PITCH / PCB_UNIT; x < xmax; x += PITCH / PCB_UNIT)
    {
      Element += sprintf (Element, "Element[\"\" \"\" \"\" \"\" %d ",
        ox + (int) x);
      Element += sprintf (Element, "%d 0 0 0 100 \"\"]\n(\n", oy + (int) y);
      Element += sprintf (Element, "\tPad[0 0 0 %d ", (int) y2);
      Element += sprintf (Element, "%d ", (int) (LINK / PCB_UNIT));
      Element += sprintf (Element, "0 %d ", (int) (MASK_CHAN / PCB_UNIT));
      Element += sprintf (Element, "\"1\" \"1\" \"square\"]\n)\n");
    }
    Element += sprintf (Element, "\n");
    Element += sprintf (Element, "Element[\"\" \"\" \"\" \"\" %d ", ox);
    Element += sprintf (Element, "%d 0 0 0 100 \"\"]\n(\n", oy);
    Element += sprintf (Element, "\tPad[0 0 %d ", (int) xmax);
    Element += sprintf (Element, "0 %d ", (int) (LINK / PCB_UNIT));
    Element += sprintf (Element, "0 %d ", (int) (PITCH / PCB_UNIT));
    Element += sprintf (Element, "\"1\" \"1\" \"square\"]\n)\n\n");
    Element += sprintf (Element, "Element[\"\" \"\" \"\" \"\" %d ", ox);
    Element += sprintf (Element, "%d 0 0 0 100 \"\"]\n(\n", oy + (int) ymax);
    Element += sprintf (Element, "\tPad[0 0 %d ", (int) xmax);
    Element += sprintf (Element, "0 %d ", (int) (LINK / PCB_UNIT));
    Element += sprintf (Element, "0 %d ", (int) (PITCH / PCB_UNIT));
    Element += sprintf (Element, "\"1\" \"1\" \"square\"]\n)\n\n");
    Element += sprintf (Element, "Element[\"\" \"\" \"\" \"\" %d ", ox);
    Element += sprintf (Element, "%d 0 0 0 100 \"\"]\n(\n", oy);
    Element += sprintf (Element, "\tPad[0 0 0 %d ", (int) ymax);
    Element += sprintf (Element, "%d ", (int) (LINK / PCB_UNIT));
    Element += sprintf (Element, "0 %d ", (int) (PITCH / PCB_UNIT));
    Element += sprintf (Element, "\"1\" \"1\" \"square\"]\n)\n\n");
    Element += sprintf (Element, "Element[\"\" \"\" \"\" \"\" %d ",
      ox + (int) xmax);
    Element += sprintf (Element, "%d 0 0 0 100 \"\"]\n(\n", oy);
    Element += sprintf (Element, "\tPad[0 0 0 %d ", (int) ymax);
    Element += sprintf (Element, "%d ", (int) (LINK / PCB_UNIT));
    Element += sprintf (Element, "0 %d ", (int) (PITCH / PCB_UNIT));
    Element += sprintf (Element, "\"1\" \"1\" \"square\"]\n)\n\n");
    Element += sprintf (Element, "Element[\"onsolder\" \"\" \"\" \"\" %d ",
      ox);
    Element += sprintf (Element, "%d 0 0 0 100 \"\"]\n(\n", oy);
    Element += sprintf (Element, "\tPad[0 0 %d ", (int) xmax);
    Element += sprintf (Element, "0 %d ", (int) (LINK / PCB_UNIT));
    Element += sprintf (Element, "0 %d ", (int) (POWER_EDGE / PCB_UNIT));
    Element += sprintf (Element, "\"1\" \"1\" \"onsolder,square\"]\n)\n\n");
    Element += sprintf (Element, "Element[\"onsolder\" \"\" \"\" \"\" %d ",
      ox);
    Element += sprintf (Element, "%d 0 0 0 100 \"\"]\n(\n", oy + (int) ymax);
    Element += sprintf (Element, "\tPad[0 0 %d ", (int) xmax);
    Element += sprintf (Element, "0 %d ", (int) (LINK / PCB_UNIT));
    Element += sprintf (Element, "0 %d ", (int) (POWER_EDGE / PCB_UNIT));
    Element += sprintf (Element, "\"1\" \"1\" \"onsolder,square\"]\n)\n\n");
    Element += sprintf (Element, "Element[\"onsolder\" \"\" \"\" \"\" %d ",
      ox);
    Element += sprintf (Element, "%d 0 0 0 100 \"\"]\n(\n", oy);
    Element += sprintf (Element, "\tPad[0 0 0 %d ", (int) ymax);
    Element += sprintf (Element, "%d ", (int) (LINK / PCB_UNIT));
    Element += sprintf (Element, "0 %d ", (int) (POWER_EDGE / PCB_UNIT));
    Element += sprintf (Element, "\"1\" \"1\" \"onsolder,square\"]\n)\n\n");
    Element += sprintf (Element, "Element[\"onsolder\" \"\" \"\" \"\" %d ",
      ox + (int) xmax);
    Element += sprintf (Element, "%d 0 0 0 100 \"\"]\n(\n", oy);
    Element += sprintf (Element, "\tPad[0 0 0 %d ", (int) ymax);
    Element += sprintf (Element, "%d ", (int) (LINK / PCB_UNIT));
    Element += sprintf (Element, "0 %d ", (int) (POWER_EDGE / PCB_UNIT));
    Element += sprintf (Element, "\"1\" \"1\" \"onsolder,square\"]\n)\n\n");
  }
  dx = dy = 0.5 * (PITCH - GAP) / PCB_UNIT;
  for (y = 0.5 * PITCH / PCB_UNIT; y < ymax; y += PITCH / PCB_UNIT)
  {
    for (x = 0.5 * PITCH / PCB_UNIT; x < xmax; x += PITCH / PCB_UNIT)
    {
      Poly += sprintf (Poly, "\tPolygon(\"clearpoly\")\n\t(\n\t\t");
      Poly += sprintf (Poly, "[%d ", ox + (int) (x - dx));
      Poly += sprintf (Poly, "%d] ", oy + (int) (y - dy));
      Poly += sprintf (Poly, "[%d ", ox + (int) (x + dx));
      Poly += sprintf (Poly, "%d] ", oy + (int) (y - dy));
      Poly += sprintf (Poly, "[%d ", ox + (int) (x + dx));
      Poly += sprintf (Poly, "%d] ", oy + (int) (y + dy));
      Poly += sprintf (Poly, "[%d ", ox + (int) (x - dx));
      Poly += sprintf (Poly, "%d]\n\t)\n", oy + (int) (y + dy));
    }
  }
  x1 = PLANE_GAP / PCB_UNIT;
  x2 = xmax - PLANE_GAP / PCB_UNIT;
  y1 = PLANE_GAP / PCB_UNIT;
  y2 = ymax - PLANE_GAP / PCB_UNIT;
  Plane += sprintf (Plane, "\tPolygon(\"clearpoly\")\n\t(\n\t\t");
  Plane += sprintf (Plane, "[%d ", ox + (int) x1);
  Plane += sprintf (Plane, "%d] ", oy + (int) y1);
  Plane += sprintf (Plane, "[%d ", ox + (int) x2);
  Plane += sprintf (Plane, "%d] ", oy + (int) y1);
  Plane += sprintf (Plane, "[%d ", ox + (int) x2);
  Plane += sprintf (Plane, "%d] ", oy + (int) y2);
  Plane += sprintf (Plane, "[%d ", ox + (int) x1);
  Plane += sprintf (Plane, "%d]", oy + (int) y2);
  Plane += sprintf (Plane, "\n\t)\n");
  yi = 0;
  for (y = 1.0 * PITCH / PCB_UNIT; y < ymax - 0.5; y += PITCH / PCB_UNIT)
  {
    toggle1 ^= 1;
    toggle2 = toggle1;
    xi = 0;
    for (x = 1.0 * PITCH / PCB_UNIT; x < xmax - 0.5; x += PITCH / PCB_UNIT)
    {
      if ((xi & 3) == 3)
      {
        Silk += sprintf (Silk, "\tLine[%d %d %d %d 800 1600 \"\"]\n",
          ox + (int) x,
          oy + (int) (y - (40.0 * mil / PCB_UNIT)),
          ox + (int) x,
          oy + (int) (y - (25.0 * mil / PCB_UNIT)));
        Silk += sprintf (Silk, "\tLine[%d %d %d %d 800 1600 \"\"]\n",
          ox + (int) x,
          oy + (int) (y + (40.0 * mil / PCB_UNIT)),
          ox + (int) x,
          oy + (int) (y + (25.0 * mil / PCB_UNIT)));
      }
      if ((yi & 3) == 3)
      {
        Silk += sprintf (Silk, "\tLine[%d %d %d %d 800 1600 \"\"]\n",
          ox + (int) (x - (40.0 * mil / PCB_UNIT)),
          oy + (int) y,
          ox + (int) (x - (25.0 * mil / PCB_UNIT)),
          oy + (int) y);
        Silk += sprintf (Silk, "\tLine[%d %d %d %d 800 1600 \"\"]\n",
          ox + (int) (x + (40.0 * mil / PCB_UNIT)),
          oy + (int) y,
          ox + (int) (x + (25.0 * mil / PCB_UNIT)),
          oy + (int) y);
      }
      ++xi;
    }
    ++yi;
  }
}


void
EndFile ()
{
  double step = 0.9 * mm / PCB_UNIT;

  xmax = width / PCB_UNIT - step;
  ymax = height / PCB_UNIT - 0.5 * step;
/*
fprintf(f, "\tLine[%d %d %d %d 700 1400 \"clearline\"]\n", 
        (int) (3.0 * inch / PCB_UNIT),
        (int) (0.0 * inch / PCB_UNIT),
        (int) (3.0 * inch / PCB_UNIT),
        (int) (4.0 * inch / PCB_UNIT));
fprintf(f, "\tLine[%d %d %d %d 700 1400 \"clearline\"]\n", 
        (int) (3.0 * inch / PCB_UNIT),
        (int) (2.0 * inch / PCB_UNIT),
        (int) (6.0 * inch / PCB_UNIT),
        (int) (2.0 * inch / PCB_UNIT));
fprintf(f, "\tLine[%d %d %d %d 700 1400 \"clearline\"]\n", 
        (int) (4.5 * inch / PCB_UNIT),
        (int) (2.0 * inch / PCB_UNIT),
        (int) (4.5 * inch / PCB_UNIT),
        (int) (4.0 * inch / PCB_UNIT));
*/
  x = 3.0 * inch / PCB_UNIT;
  for (y = step; y < ymax; y += step)
  {
    Via += sprintf (Via, "Via[%d ", (int) x);
    Via += sprintf (Via, "%d ", (int) y);
    Via += sprintf (Via, "%d ", 0); // Cu
    Via += sprintf (Via, "%d ", 0); // Clear
    Via += sprintf (Via, "%d ", 0); // Mask
    Via += sprintf (Via, "%d ", (int) (0.5 * mm / PCB_UNIT)); // Drill
    Via += sprintf (Via, "\"\" \"hole\"]\n");
  }
  x = 4.5 * inch / PCB_UNIT;
  for (y = step + 2.0 * inch / PCB_UNIT; y < ymax; y += step)
  {
    Via += sprintf (Via, "Via[%d ", (int) x);
    Via += sprintf (Via, "%d ", (int) y);
    Via += sprintf (Via, "%d ", 0); // Cu
    Via += sprintf (Via, "%d ", 0); // Clear
    Via += sprintf (Via, "%d ", 0); // Mask
    Via += sprintf (Via, "%d ", (int) (0.5 * mm / PCB_UNIT)); // Drill
    Via += sprintf (Via, "\"\" \"hole\"]\n");
  }
  y = 2.0 * inch / PCB_UNIT;
  for (x = step + 3.0 * inch / PCB_UNIT; x < xmax; x += step)
  {
    Via += sprintf (Via, "Via[%d ", (int) x);
    Via += sprintf (Via, "%d ", (int) y);
    Via += sprintf (Via, "%d ", 0); // Cu
    Via += sprintf (Via, "%d ", 0); // Clear
    Via += sprintf (Via, "%d ", 0); // Mask
    Via += sprintf (Via, "%d ", (int) (0.5 * mm / PCB_UNIT)); // Drill
    Via += sprintf (Via, "\"\" \"hole\"]\n");
  }
  xmax = width / PCB_UNIT;
  ymax = height / PCB_UNIT;
  fprintf (f, "%s\n", Vias);
  fprintf (f, "%s\n", Elements);
  fprintf (f, "Layer(1 \"component\")\n(\n");
  fprintf (f, "%s\n", Polys);
  fprintf (f, ")\n");
  fprintf (f, "Layer(2 \"power\")\n(\n");
#if FOURLAYERS
  fprintf (f, "%s\n", Planes);
#endif
  fprintf (f, ")\n");
  fprintf (f, "Layer(3 \"GND\")\n(\n");
#if FOURLAYERS
  fprintf (f, "%s\n", Planes);
#endif
  fprintf (f, ")\n");
  fprintf (f, "Layer(4 \"solder\")\n(\n");
#if FOURLAYERS
  fprintf (f, "%s\n", Polys);
#else
  fprintf (f, "%s\n", Planes);
#endif
  fprintf (f, ")\n");
  fprintf (f, "Layer(5 \"outline\")\n(\n");
  fprintf (f, "\tLine[0 0 %d 0 800 1600 \"clearline\"]\n", (int) xmax);
  fprintf (f, "\tLine[%d 0 %d %d 800 1600 \"clearline\"]\n",
    (int) xmax,
    (int) xmax,
    (int) ymax);
  fprintf (f, "\tLine[%d %d 0 %d 800 1600 \"clearline\"]\n",
    (int) xmax,
    (int) ymax,
    (int) ymax);
  fprintf (f, "\tLine[0 %d 0 0 800 1600 \"clearline\"]\n", (int) ymax);
  fprintf (f, "\tLine[%d %d %d %d 700 1400 \"clearline\"]\n",
    (int) (3.0 * inch / PCB_UNIT),
    (int) (0.0 * inch / PCB_UNIT),
    (int) (3.0 * inch / PCB_UNIT),
    (int) (4.0 * inch / PCB_UNIT));
  fprintf (f, "\tLine[%d %d %d %d 700 1400 \"clearline\"]\n",
    (int) (3.0 * inch / PCB_UNIT),
    (int) (2.0 * inch / PCB_UNIT),
    (int) (6.0 * inch / PCB_UNIT),
    (int) (2.0 * inch / PCB_UNIT));
  fprintf (f, "\tLine[%d %d %d %d 700 1400 \"clearline\"]\n",
    (int) (4.5 * inch / PCB_UNIT),
    (int) (2.0 * inch / PCB_UNIT),
    (int) (4.5 * inch / PCB_UNIT),
    (int) (4.0 * inch / PCB_UNIT));
  fprintf (f, ")\n");
  fprintf (f, "Layer(6 \"silk\")\n(\n)\n");
  fprintf (f, "Layer(7 \"silk\")\n(\n");
  fprintf (f, "%s)\n", Silks);
  fclose (f);
}


int
main ()
{
  soldermask = 1;
  width = 6.0 * inch;
  height = 4.0 * inch;
  StartFile ();
  width = 3.0 * inch;
  height = 4.0 * inch;
  ox = 0;
  oy = 0;
  Render ();
  width = 3.0 * inch;
  height = 2.0 * inch;
  ox = (int) (3.0 * inch / PCB_UNIT);
  oy = 0;
  Render ();
  width = 1.5 * inch;
  height = 2.0 * inch;
  ox = (int) (3.0 * inch / PCB_UNIT);
  oy = (int) (2.0 * inch / PCB_UNIT);
  Render ();
  ox = (int) (4.5 * inch / PCB_UNIT);
  oy = (int) (2.0 * inch / PCB_UNIT);
  Render ();
  width = 6.0 * inch;
  height = 4.0 * inch;
  EndFile ();
  return 0;
}
