#include <stdio.h>
#include <math.h>

#include "global.h"
#include "data.h"
#include "hid.h"
#include "error.h"

static int
findrat (int argc, char **argv, int x, int y)
{
  RatType *r;

  if (PCB->Data->RatN == 0)
    {
      Message("No Rats");
      return 0;
    }

  r = & (PCB->Data->Rat[0]);
  gui->set_crosshair (r->Point1.X, r->Point1.Y, HID_SC_PAN_VIEWPORT);
  return 0;
}

static HID_Action findrat_action_list[] = {
  {"FindRat", NULL, findrat,
   NULL, NULL}
};

REGISTER_ACTIONS (findrat_action_list)

void
pcb_plugin_init()
{
  register_findrat_action_list();
}
