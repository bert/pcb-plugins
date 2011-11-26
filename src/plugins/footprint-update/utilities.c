/*
 * utilities
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
 * $Id: utilities.c,v 1.22 2008-05-22 04:52:13 dean Exp $
 * Dean Ferreyra
 */

#include "utilities.h"

static Boolean have_two_corresponding_aux(ElementTypePtr old_element,
                                          ElementTypePtr new_element,
                                          Boolean unique,
                                          Boolean non_coincident,
                                          PadOrPinType* old1_pp_ptr,
                                          PadOrPinType* old2_pp_ptr,
                                          PadOrPinType* new1_pp_ptr,
                                          PadOrPinType* new2_pp_ptr);
/*
 * Pad/pin type
 */

PadOrPinType
make_pad_or_pin(PadTypePtr pad, PinTypePtr pin)
{
  PadOrPinType pp = { pad, pin };
  return pp;
}

Boolean
is_pad_or_pin(const PadOrPinType* pp)
{
  return pp->pad || pp->pin;
}

const char*
pad_or_pin_number(const PadOrPinType* pp)
{
  if (pp->pad) {
    return pp->pad->Number;
  } else if (pp->pin) {
    return pp->pin->Number;
  } else {
    return NULL;
  }
}

const char*
pad_or_pin_name(const PadOrPinType* pp)
{
  if (pp->pad) {
    return pp->pad->Name;
  } else if (pp->pin) {
    return pp->pin->Name;
  } else {
    return NULL;
  }
}

int
number_cmp(const char* number_a, const char* number_b)
{
  if (! number_a && ! number_b) {
    return 0;
  } else if (! number_a) {
    return -1;
  } else if (! number_b) {
    return 1;
  } else {
    return strcmp(number_a, number_b);
  }
}

Boolean
pad_or_pin_test_flag(const PadOrPinType* pp, unsigned long flag)
{
  if (pp->pad) {
    return TEST_FLAG(flag, pp->pad);
  } else if (pp->pin) {
    return TEST_FLAG(flag, pp->pin);
  } else {
    return False;
  }
}
void
pad_or_pin_set_flag(PadOrPinType* pp, unsigned long flag)
{
  if (pp->pad) {
    SET_FLAG(flag, pp->pad);
  } else if (pp->pin) {
    SET_FLAG(flag, pp->pin);
  }
}

int
pad_or_pin_number_cmp(const PadOrPinType* ppa, const PadOrPinType* ppb)
{
  return number_cmp(pad_or_pin_number(ppa), pad_or_pin_number(ppb));
}

CheapPointType
pad_or_pin_center(PadOrPinType* pp)
{
  if (pp->pad) {
    return pad_center(pp->pad);
  } else if (pp->pin) {
    return pin_center(pp->pin);
  } else {
    base_log("Error: pad_or_pin_center() got an empty PadOrPinType.\n");
    return make_point(0, 0);
  }
}

CheapPointType
pad_center(PadTypePtr pad)
{
  return make_point((pad->Point1.X + pad->Point2.X) / 2,
                    (pad->Point1.Y + pad->Point2.Y) / 2);
}

CheapPointType
pin_center(PinTypePtr pin)
{
  return MAKE_PT(*pin);
}

Boolean
find_pad_or_pin(ElementTypePtr element, const char* number,
                PadOrPinType* pp_ptr)
{
  PadOrPinType pp = make_pad_or_pin(NULL, NULL);
  if (number &&
      ((pp.pad = find_pad(element, number))
       || (pp.pin = find_pin(element, number)))) {
    *pp_ptr = pp;
    return True;
  } else {
    return False;
  }
}

PadTypePtr
find_pad(ElementTypePtr element, const char* number)
{
  PAD_LOOP(element);
  {
    if (pad->Number && number_cmp(pad->Number, number) == 0) {
      return pad;
    }
  }
  END_LOOP;
  return NULL;
}

PinTypePtr
find_pin(ElementTypePtr element, const char* number)
{
  PIN_LOOP(element);
  {
    if (pin->Number && number_cmp(pin->Number, number) == 0) {
      return pin;
    }
  }
  END_LOOP;
  return NULL;
}

Boolean
is_number_unique(ElementTypePtr element, const char* number)
{
  int count = 0;
  PAD_OR_PIN_LOOP(element);
  {
    const char* pp_number = pad_or_pin_number(&pp);
    if (pp_number && number_cmp(pp_number, number) == 0) {
      if (++count > 1) {
        return False;
      }
    }
  }
  END_LOOP;
  return True;
}

Boolean
have_two_corresponding_non_coincident(ElementTypePtr old_element,
                                      ElementTypePtr new_element,
                                      PadOrPinType* old1_pp_ptr,
                                      PadOrPinType* old2_pp_ptr,
                                      PadOrPinType* new1_pp_ptr,
                                      PadOrPinType* new2_pp_ptr)
{
  return have_two_corresponding_aux(old_element, new_element,
                                    False, True,
                                    old1_pp_ptr, old2_pp_ptr,
                                    new1_pp_ptr, new2_pp_ptr);
}

Boolean
have_two_corresponding_unique_non_coincident(ElementTypePtr old_element,
                                             ElementTypePtr new_element,
                                             PadOrPinType* old1_pp_ptr,
                                             PadOrPinType* old2_pp_ptr,
                                             PadOrPinType* new1_pp_ptr,
                                             PadOrPinType* new2_pp_ptr)
{
  return have_two_corresponding_aux(old_element, new_element,
                                    True, True,
                                    old1_pp_ptr, old2_pp_ptr,
                                    new1_pp_ptr, new2_pp_ptr);
}

static Boolean
have_two_corresponding_aux(ElementTypePtr old_element,
                           ElementTypePtr new_element,
                           Boolean unique,
                           Boolean non_coincident,
                           PadOrPinType* old1_pp_ptr,
                           PadOrPinType* old2_pp_ptr,
                           PadOrPinType* new1_pp_ptr,
                           PadOrPinType* new2_pp_ptr)
{
  PadOrPinType old1_pp = make_pad_or_pin(NULL, NULL);
  PadOrPinType new1_pp = make_pad_or_pin(NULL, NULL);
  Boolean first_found = False;

  PAD_OR_PIN_LOOP_HYG(old_element, _old);
  {
    if (! unique
        || is_number_unique(old_element,
                            pad_or_pin_number(&pp_old))) {
      CheapPointType old_pt = pad_or_pin_center(&pp_old);
      PAD_OR_PIN_LOOP_HYG(new_element, _new);
      {
        if (pad_or_pin_number_cmp(&pp_old, &pp_new) == 0
            && (! unique
                || is_number_unique(new_element,
                                    pad_or_pin_number(&pp_new)))) {
          CheapPointType new_pt = pad_or_pin_center(&pp_new);
          if (! non_coincident || point_distance2(old_pt, new_pt)) {
            if (! first_found) {
              old1_pp = pp_old;
              new1_pp = pp_new;
              first_found = True;
            } else {
              if (old1_pp_ptr) {
                *old1_pp_ptr = old1_pp;
              }
              if (new1_pp_ptr) {
                *new1_pp_ptr = new1_pp;
              }
              if (old2_pp_ptr) {
                *old2_pp_ptr = pp_old;
              }
              if (new2_pp_ptr) {
                *new2_pp_ptr = pp_new;
              }
              return True;
            }
          }
        }
      }
      END_LOOP;
    }
  }
  END_LOOP;
  return False;
}

Boolean
have_any_corresponding_pad_or_pin(ElementTypePtr old_element,
                                  ElementTypePtr new_element,
                                  PadOrPinType* old_pp,
                                  PadOrPinType* new_pp)
{
  PAD_OR_PIN_LOOP_HYG(old_element, _old);
  {
    PAD_OR_PIN_LOOP_HYG(new_element, _new);
    {
      if (pad_or_pin_number_cmp(&pp_old, &pp_new) == 0) {
        if (old_pp) {
          *old_pp = pp_old;
        }
        if (new_pp) {
          *new_pp = pp_new;
        }
        return True;
      }
    }
    END_LOOP;
  }
  END_LOOP;
  return False;
}

/*
 * CheapPointType helpers.
 */

CheapPointType
make_point(LocationType x, LocationType y)
{
  CheapPointType pt = { x, y };
  return pt;
}

CheapPointType
point_subtract(CheapPointType pt1, CheapPointType pt2)
{
  return make_point(pt1.X - pt2.X, pt1.Y - pt2.Y);
}

/* Distance squared */
double
point_distance2(CheapPointType pt1, CheapPointType pt2)
{
  CheapPointType diff = point_subtract(pt1, pt2);
  return (double)diff.X * (double)diff.X + (double)diff.Y * (double)diff.Y;
}

/*
 * Miscellaneous
 */

double
round(double v)
{
  if (v < 0) {
    return ceil(v - 0.5);
  } else {
    return floor(v + 0.5);
  }
}

double
multiple_of_90(double rad)
{
  return (M_PI / 2) * round(rad / (M_PI / 2));
}

#define ANGLE_EPSILON 0.01
#define ANGLE_NEAR(a1, a2) (fabs((a1) - (a2)) < ANGLE_EPSILON)

BYTE
angle_to_rotation_steps(double rad)
{
  double rad90 = multiple_of_90(rad);

  if (ANGLE_NEAR(rad90, M_PI / 2)) {
    return 3;
  } else if (ANGLE_NEAR(fabs(rad90), M_PI)) {
    return 2;
  } else if (ANGLE_NEAR(rad90, -M_PI / 2)) {
    return 1;
  } else if (ANGLE_NEAR(rad90, 0)) {
    return 0;
  } else {
    base_log("Error: angle_to_rotation_steps() got "
             "an unhandled angle: %lf\n",
             rad * RAD_TO_DEG);
    return 0;
  }
}

/*
 * Logging
 */

void
log_point(CheapPointType pt)
{
  base_log("Point\n");
  base_log("%8d %8d\n", pt.X, pt.Y);
}

void
log_pad_or_pin(const PadOrPinType* pp)
{
  if (pp->pad) {
    log_pad(pp->pad);
  } else {
    log_pin(pp->pin);
  }
}

void
log_pad(PadTypePtr p)
{
  base_log("Pad\n");
  base_log("Name: \"%s\"\n", p->Name);
  base_log("Number: \"%s\"\n", p->Number);
  base_log("Point 1: (%d, %d)\n", p->Point1.X, p->Point1.Y);
  base_log("Point 2: (%d, %d)\n", p->Point2.X, p->Point2.Y);
}

void
log_pin(PinTypePtr p)
{
  base_log("Pin\n");
  base_log("Name: \"%s\"\n", p->Name);
  base_log("Number: \"%s\"\n", p->Number);
  base_log("Point: (%d, %d)\n", p->X, p->Y);
}

void
log_element(ElementTypePtr e)
{
  base_log("Element\n");
  base_log("Description: %s\n", DESCRIPTION_NAME(e));
  base_log("Name on PCB: %s\n", NAMEONPCB_NAME(e));
  base_log("Value: %s\n", VALUE_NAME(e));
  base_log("Flags: %04x\n", FLAG_VALUE(e->Flags));
  base_log("MarkX, MarkY: %d, %d\n", e->MarkX, e->MarkY);
  base_log("PinN: %d\n", e->PinN);
  base_log("PadN: %d\n", e->PadN);
  base_log("LineN: %d\n", e->LineN);
  base_log("ArcN: %d\n", e->ArcN);
  base_log("Attributes number: %d\n", e->Attributes.Number);

  int i = 0;
  PAD_LOOP(e);
  {
    base_log("Pad %d\n", i++);
    log_pad(pad);
  }
  END_LOOP;

  i = 0;
  PIN_LOOP(e);
  {
    base_log("Pin %d\n", i++);
    log_pin(pin);
  }
  END_LOOP;
}

void base_log(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  gui->log(TITLE);
  gui->log(": ");
  gui->logv(fmt, args);
#if DEBUG
  /* In debug mode, output logs to stderr. */
  vfprintf(stderr, fmt, args);
#endif
  va_end(args);
}

void debug_log(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
#if DEBUG
  gui->log(TITLE);
  gui->log(": ");
  gui->logv(fmt, args);
  /* In debug mode, output logs to stderr. */
  vfprintf(stderr, fmt, args);
#endif
  va_end(args);
}
