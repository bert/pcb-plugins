/*
 * footprintupdate -- Replaces footprints in layout with updated
 *     footprints.
 *
 * Copyright 2008 Dean Ferreyra <dferreyra@igc.org>, All rights reserved
 *
 * This file is part of Footprint-Update.
 * 
 * Footprint-Update is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Footprint-Update is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Footprint-Update.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id: footprintupdate.c,v 1.31 2008-05-22 07:57:03 dean Exp $
 */

/*
 * This PCB plug-in adds the UpdateFootprintsFromBuffer action.  It
 * allows you to replace existing elements (i.e., footprints) in your
 * PCB layout with an updated element that you've loaded into the
 * buffer.
 *
 * Because PCB modifies the elements it places, the plug-in needs to
 * make an educated guess about which elements to replace, and how to
 * position and orient the replacement elements.  It might guess wrong
 * or not replace some elements.
 *
 * WARNING: Make a backup copy of your layout before using this
 * plug-in in case something goes wrong!
 *
 * Usage:
 *
 *   UpdateFootprintsFromBuffer() or UpdateFootprintsFromBuffer(auto) --
 *     Uses the description field of the buffer element and tries to
 *     replace any layout element with a matching description field.
 *     (In my workflow, the element description in the footprint files
 *     and in the layout elements is the original footprint file name,
 *     e.g., "PHOENIX-PT-4-R.fp".)
 *
 *   UpdateFootprintsFromBuffer(auto, selected) -- Like
 *     UpdateFootprintsFromBuffer(auto) but will only consider selected
 *     layout elements.
 *
 *   UpdateFootprintsFromBuffer(auto, named, <name1>[, <name2>...]) --
 *     Like UpdateFootprintsFromBuffer(auto) but will only consider
 *     elements that match the given element names.  For example,
 *     UpdateFootprintsFromBuffer(auto, named, U101, U103, U105).
 *
 *   UpdateFootprintsFromBuffer(manual) or
 *     UpdateFootprintsFromBuffer(manual, selected) -- Like the "auto"
 *     version, but tries to replace the selected layout elements
 *     regardless of their description field.
 *
 *   UpdateFootprintsFromBuffer(manual, named, <name1>[, <name2>...]) --
 *     Like the "auto" version, but tries to replace the named layout
 *     elements regardless of their description field.
 *
 * Current assumptions and limitations:
 *
 *   * The rotation of the new element is calculated using two
 *     non-coincident pads or pins from both the buffer element and
 *     the layout element.  For elements consisting entirely of
 *     coincident pads and pins, the plug-in will only translate the
 *     replacement element without regard to rotation.
 *
 *   * Element-specific attributes in the layout element, if it has
 *     any, are replaced with the buffer element's attributes; i.e.,
 *     attributes in the layout element, if it has any, will be lost.
 */

#include "utilities.h"
#include "matrix.h"
#include "pad-pin-data.h"

static int replace_footprints(ElementTypePtr new_element);
static Boolean replace_one_footprint(ElementTypePtr old_element,
                                     ElementTypePtr new_element);
static Boolean replace_one_footprint_quick(ElementTypePtr old_element,
                                           ElementTypePtr new_element);
static Boolean replace_one_footprint_expensive(ElementTypePtr old_element,
                                               ElementTypePtr new_element);
static Boolean replace_one_footprint_translate_only(ElementTypePtr old_element,
                                                    ElementTypePtr new_element);
static void replace_one_footprint_aux(ElementTypePtr old_element,
                                      PadOrPinType* old1_pp,
                                      PadOrPinType* old2_pp,
                                      ElementTypePtr new_element,
                                      PadOrPinType* new1_pp,
                                      PadOrPinType* new2_pp);
static BYTE calculate_rotation_steps(CheapPointType new1_pt,
                                     CheapPointType new2_pt,
                                     CheapPointType old1_pt,
                                     CheapPointType old2_pt);
static void transfer_text(ElementTypePtr old_element,
                          ElementTypePtr new_element);
static void transfer_names(ElementTypePtr old_element,
                           ElementTypePtr new_element);
static void transfer_flags(ElementTypePtr old_element,
                           ElementTypePtr new_element);

enum {
  MATCH_MODE_AUTO,              /* Use element description field. */
  MATCH_MODE_MANUAL
} match_mode = MATCH_MODE_AUTO;

enum {
  STYLE_ALL,                    /* Only valid in auto mode. */
  STYLE_SELECTED,               /* Only consider selected elements. */
  STYLE_NAMED                   /* Only consider elements named in the
                                   arguments. */
} style = STYLE_ALL;

/*
 * Plug-in housekeeping.
 */

int footprint_update(int argc, char **argv, int x, int y);

HID_Action footprint_update_action_list[] = {
  { ACTION_NAME, NULL, footprint_update, NULL, NULL }
};

REGISTER_ACTIONS(footprint_update_action_list)

void
pcb_plugin_init()
{
  register_footprint_update_action_list();
}

int global_argc = 0;
char **global_argv = NULL;

int
usage()
{
  base_log("usage:\n");
  base_log("    UpdateFootprintsFromBuffer()\n");
  base_log("    UpdateFootprintsFromBuffer(auto|manual)\n");
  base_log("    UpdateFootprintsFromBuffer(auto|manual, selected)\n");
  base_log("    UpdateFootprintsFromBuffer(auto|manual, named, "
           "<name1>[, <name2>...])\n");
  base_log("version: %s\n", VERSION);
  return 1;
}

int
footprint_update(int argc, char **argv, int x, int y)
{
  global_argc = argc;
  global_argv = argv;

  debug_log("footprint_update\n");
  debug_log("  argc: %d\n", argc);
  if (argc) {
    int i;
    for (i = 0; i < argc; i++) {
      debug_log("  argv[%d]: %s\n", i, argv[i]);
    }
  }
  if (argc >= 1) {
    if (strcasecmp(argv[0], "auto") == 0) {
      match_mode = MATCH_MODE_AUTO;
      style = STYLE_ALL;
    } else if (strcasecmp(argv[0], "manual") == 0) {
      match_mode = MATCH_MODE_MANUAL;
      style = STYLE_SELECTED;
    } else {
      base_log("Error: If given, the first argument must be "
               "\"auto\" or \"manual\".\n");
      return usage();
    }
    if (argc >= 2) {
      if (strcasecmp(argv[1], "selected") == 0) {
        style = STYLE_SELECTED;
      } else if (strcasecmp(argv[1], "named") == 0) {
        style = STYLE_NAMED;
      } else {
        base_log("Error: If given, the second argument must be "
                 "\"selected\", or \"named\".\n");
        return usage();
      }
    }
    
  }
  debug_log("match_mode: %d\n", match_mode);
  debug_log("style: %d\n", style);

  if (PASTEBUFFER->Data->ElementN != 1) {
    base_log("Error: Paste buffer should contain one element.\n");
    return usage();
  }

  ELEMENT_LOOP(PASTEBUFFER->Data);
  {
    int replaced = replace_footprints(element);
    if (replaced) {
      base_log("Replaced %d elements.\n", replaced);
      IncrementUndoSerialNumber();
    }
  }
  END_LOOP;

  return 0;
}

static int
replace_footprints(ElementTypePtr new_element)
{
  int replaced = 0;
  int i = 0;

  ELEMENT_LOOP (PCB->Data);
  {
    if (match_mode != MATCH_MODE_AUTO
        || strcmp(DESCRIPTION_NAME(new_element),
                  DESCRIPTION_NAME(element)) == 0) {
      Boolean matched = False;

      switch (style) {
      case STYLE_ALL:
        matched = True;
        break;
      case STYLE_SELECTED:
        matched = TEST_FLAG(SELECTEDFLAG, element);
        break;
      case STYLE_NAMED:
        for (i = 0; i < global_argc; i++) {
          if (NAMEONPCB_NAME(element)
              && strcasecmp(NAMEONPCB_NAME(element), global_argv[i]) == 0) {
            matched = True;
            break;
          }
        }
        break;
      }
      if (matched) {
        if (TEST_FLAG (LOCKFLAG, element)) {
          base_log("Skipping \"%s\".  Element locked.\n",
                   NAMEONPCB_NAME(element));
        } else {
          debug_log("Considering \"%s\".\n", NAMEONPCB_NAME(element));
          if (replace_one_footprint(element, new_element)) {
            base_log("Replaced \"%s\".\n", NAMEONPCB_NAME(element));
            replaced++;
          }
        }
      }
    }
  }
  END_LOOP;
  return replaced;
}

static Boolean
replace_one_footprint(ElementTypePtr old_element,
                      ElementTypePtr new_element)
{
  /* Called in order from from most desirable to least desirable. */
  return (replace_one_footprint_quick(old_element, new_element)
          || replace_one_footprint_expensive(old_element, new_element)
          || replace_one_footprint_translate_only(old_element, new_element));
}

static Boolean
replace_one_footprint_quick(ElementTypePtr old_element,
                            ElementTypePtr new_element)
{
  /* Requires that there be two non-coincident, uniquely numbered
     pads/pins that correspond between the old and new element. */
  if (! have_two_corresponding_unique_non_coincident(old_element,
                                                     new_element,
                                                     NULL, NULL,
                                                     NULL, NULL)) {
    return False;
  }

  /* Create copy of new element. */
  ElementTypePtr copy_element =
    CopyElementLowLevel(PCB->Data, NULL, new_element, False, 0, 0);

  PadOrPinType old1_pp;
  PadOrPinType old2_pp;
  PadOrPinType copy1_pp;
  PadOrPinType copy2_pp;
  if (! have_two_corresponding_unique_non_coincident(old_element,
                                                     copy_element,
                                                     &old1_pp, &old2_pp,
                                                     &copy1_pp, &copy2_pp)) {
      base_log("Error: Couldn't find two corresponding, unique, "
               "non-coincident element pads/pins.");
      return False;
  }
  replace_one_footprint_aux(old_element, &old1_pp, &old2_pp,
                            copy_element, &copy1_pp, &copy2_pp);
  debug_log("Used quick replacement.\n");
  return True;
}

static Boolean
replace_one_footprint_expensive(ElementTypePtr old_element,
                                ElementTypePtr new_element)
{
  /* Requires that there be two corresponding, non-coincident
     pads/pins; non-unique pad/pin numbers okay.

     It's expensive because it does an exhaustive comparison of the
     resulting transformations for the four possible 90 degree
     rotations plus combinations of same-numbered pads/pins.  Should
     only be necessary if the element *only* has pads and pins that
     share the same pad/pin number. */
  if (! have_two_corresponding_non_coincident(old_element, new_element,
                                              NULL, NULL, NULL, NULL)) {
    return False;
  }

  /* Create copy of new element. */
  ElementTypePtr copy_element =
    CopyElementLowLevel(PCB->Data, NULL, new_element, False, 0, 0);

  int old_ppd_len = 0;
  ElementPadPinData* old_ppd =
    alloc_pad_pin_data_array(old_element, &old_ppd_len);

  int copy_ppd_len = 0;
  ElementPadPinData* copy_ppd =
    alloc_pad_pin_data_array(copy_element, &copy_ppd_len);

  int copy_index1 = 0;
  int copy_index2 = 0;
  if (! find_non_coincident(copy_ppd, copy_ppd_len,
                            &copy_index1, &copy_index2)) {
    base_log("Error: Couldn't find non-coincident element pads/pins.");
    MYFREE(old_ppd);
    MYFREE(copy_ppd);
    return False;
  }

  Boolean reflect = IS_REFLECTED(new_element, old_element);
  int old_index1 = 0;
  int old_index2 = 0;
  if (! find_best_corresponding_pads_or_pins(copy_ppd, copy_ppd_len,
                                             copy_index1, copy_index2,
                                             reflect,
                                             old_ppd, old_ppd_len,
                                             &old_index1, &old_index2)) {
    MYFREE(old_ppd);
    MYFREE(copy_ppd);
    return False;
  }

  replace_one_footprint_aux(old_element,
                            &old_ppd[old_index1].pp,
                            &old_ppd[old_index2].pp,
                            copy_element,
                            &copy_ppd[copy_index1].pp,
                            &copy_ppd[copy_index2].pp);
  MYFREE(old_ppd);
  MYFREE(copy_ppd);
  debug_log("Used expensive replacement.\n");
  return True;
}

static Boolean
replace_one_footprint_translate_only(ElementTypePtr old_element,
                                     ElementTypePtr new_element)
{
  /* Just requires one corresponding pad/pin.  Does no rotations. */
  if (! have_any_corresponding_pad_or_pin(old_element, new_element,
                                          NULL, NULL)) {
    return False;
  }

  /* Create copy of new element. */
  ElementTypePtr copy_element =
    CopyElementLowLevel(PCB->Data, NULL, new_element, False, 0, 0);

  PadOrPinType old_pp = make_pad_or_pin(NULL, NULL);
  PadOrPinType copy_pp = make_pad_or_pin(NULL, NULL);
  if (! have_any_corresponding_pad_or_pin(old_element, copy_element,
                                          &old_pp, &copy_pp)) {
    base_log("Error: Couldn't find any corresponding pads or pins.");
    return False;
  }
  replace_one_footprint_aux(old_element, &old_pp, NULL,
                            copy_element, &copy_pp, NULL);
  debug_log("Used translation-only replacement.\n");
  return True;
}

static void
replace_one_footprint_aux(ElementTypePtr old_element,
                          PadOrPinType* old1_pp, PadOrPinType* old2_pp,
                          ElementTypePtr copy_element,
                          PadOrPinType* copy1_pp, PadOrPinType* copy2_pp)
{
  Boolean two_points = (old2_pp && copy2_pp);
  Boolean reflect = IS_REFLECTED(copy_element, old_element);

  debug_log("Reflect?: %s\n", (reflect ? "yes" : "no"));
  if (reflect) {
    /* Change side of board */
    ChangeElementSide(copy_element, 0);
  }

  CheapPointType copy1_pt = pad_or_pin_center(copy1_pp);
  CheapPointType old1_pt = pad_or_pin_center(old1_pp);

  BYTE rot_steps = 0;
  if (two_points) {
    /* Calculate nearest rotation steps */
    CheapPointType copy2_pt = pad_or_pin_center(copy2_pp);
    CheapPointType old2_pt = pad_or_pin_center(old2_pp);
    rot_steps =
      calculate_rotation_steps(copy1_pt, copy2_pt, old1_pt, old2_pt);
  }
  if (rot_steps) {
    /* Rotate copy */
    RotateElementLowLevel(PCB->Data, copy_element, 0, 0, rot_steps);
    /* Recalculate since copy_element has changed. */
    copy1_pt = pad_or_pin_center(copy1_pp);
  }

  /* Calculate translation */
  LocationType dx = old1_pt.X - copy1_pt.X;
  LocationType dy = old1_pt.Y - copy1_pt.Y;
  /* Move element */
  MoveElementLowLevel(PCB->Data, copy_element, dx, dy);

  /* Transfer pad/pin text and names. */
  transfer_text(old_element, copy_element);
  transfer_names(old_element, copy_element);
  transfer_flags(old_element, copy_element);
  SetElementBoundingBox(PCB->Data, copy_element, &PCB->Font);

  AddObjectToCreateUndoList(ELEMENT_TYPE,
                            copy_element, copy_element, copy_element);
  /* Remove old element. */
  MoveObjectToRemoveUndoList(ELEMENT_TYPE,
                             old_element, old_element, old_element);
}

static BYTE
calculate_rotation_steps(CheapPointType new1_pt, CheapPointType new2_pt,
                         CheapPointType old1_pt, CheapPointType old2_pt)
{
  /* Translation of new1_pt to origin. */
  LocationType new1_to_origin_dx = -new1_pt.X;
  LocationType new1_to_origin_dy = -new1_pt.Y;
  /* Use translation for new2_pt. */
  CheapPointType new2_translated_pt =
    make_point(new2_pt.X + new1_to_origin_dx,
               new2_pt.Y + new1_to_origin_dy);
  double new2_angle =
    (new2_translated_pt.Y || new2_translated_pt.X
     ? atan2(new2_translated_pt.Y, new2_translated_pt.X) : 0);

  /* Translation of old1_pt to origin. */
  LocationType old1_to_origin_dx = -old1_pt.X;
  LocationType old1_to_origin_dy = -old1_pt.Y;
  /* Use translation for old2_pt. */
  CheapPointType old2_translated_pt =
    make_point(old2_pt.X + old1_to_origin_dx,
               old2_pt.Y + old1_to_origin_dy);
  double old2_angle =
    (old2_translated_pt.X || old2_translated_pt.Y
     ? atan2(old2_translated_pt.Y, old2_translated_pt.X) : 0);

  /* Compute rotation, adjust to match atan2 range. */
  double angle = old2_angle - new2_angle;
  if (angle > M_PI) {
    angle -= 2 * M_PI;
  } else if (angle < -M_PI) {
    angle += 2 * M_PI;
  }
  debug_log("Rotation: %lf\n", RAD_TO_DEG * angle);
  /* Return a PCB rotation steps count. */
  return angle_to_rotation_steps(angle);
}

static void
transfer_text(ElementTypePtr old_element, ElementTypePtr new_element)
{
  int i;
  for (i = 0; i < MAX_ELEMENTNAMES; i++) {
    TextTypePtr old_text = &old_element->Name[i];
    TextTypePtr new_text = &new_element->Name[i];
    MYFREE(new_text->TextString);
    new_text->X = old_text->X;
    new_text->Y = old_text->Y;
    new_text->Direction = old_text->Direction;
    new_text->Flags = old_text->Flags;
    new_text->Scale = old_text->Scale;
    new_text->TextString =
      ((old_text->TextString && *old_text->TextString) ?
       MyStrdup(old_text->TextString, "transfer_text()") : NULL);
  }
}

static void
transfer_names(ElementTypePtr old_element, ElementTypePtr new_element)
{
  PAD_OR_PIN_LOOP_HYG(old_element, _old);
  {
    const char* old_name = pad_or_pin_name(&pp_old);
    PAD_OR_PIN_LOOP_HYG(new_element, _new);
    {
      if (pad_or_pin_number_cmp(&pp_old, &pp_new) == 0) {
        if (pp_new.pad) {
          MYFREE(pp_new.pad->Name);
          pp_new.pad->Name = MyStrdup((char*)old_name, "transfer_names()");
        } else if (pp_new.pin) {
          MYFREE(pp_new.pin->Name);
          pp_new.pin->Name = MyStrdup((char*)old_name, "transfer_names()");
        }
      }
    }
    END_LOOP;
  }
  END_LOOP;
}

static void
transfer_flags(ElementTypePtr old_element, ElementTypePtr new_element)
{
  PAD_OR_PIN_LOOP_HYG(old_element, _old);
  {
    if (pad_or_pin_test_flag(&pp_old, SELECTEDFLAG)) {
      PAD_OR_PIN_LOOP_HYG(new_element, _new);
      {
        if (pad_or_pin_number_cmp(&pp_old, &pp_new) == 0) {
          pad_or_pin_set_flag(&pp_new, SELECTEDFLAG);
        }
      }
      END_LOOP;
    }
  }
  END_LOOP;
  if (TEST_FLAG(SELECTEDFLAG, old_element)) {
    SET_FLAG(SELECTEDFLAG, new_element);
  }
}
