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
 * $Id: utilities.h,v 1.21 2008-05-22 04:52:13 dean Exp $
 * Dean Ferreyra
 */

#ifndef UTILITIES_H_INCLUDED
#define UTILITIES_H_INCLUDED

#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "box.h"
#include "copy.h"
#include "change.h"
#include "rotate.h"
#include "move.h"
#include "create.h"
#include "rtree.h"
#include "polygon.h"
#include "undo.h"

#define TITLE "footprintupdate"
#define ACTION_NAME "UpdateFootprintsFromBuffer"

/*
 * Pad/pin type
 */

typedef struct {
  PadType *pad;
  PinType *pin;
} PadOrPinType;

PadOrPinType make_pad_or_pin (GList *pad, GList *pin);
bool is_pad_or_pin (const PadOrPinType* pp);
int number_cmp(const char* number_a, const char* number_b);
const char* pad_or_pin_number(const PadOrPinType* pp);
const char* pad_or_pin_name(const PadOrPinType* pp);
bool pad_or_pin_test_flag (const PadOrPinType* pp, unsigned long flag);
void pad_or_pin_set_flag(PadOrPinType* pp, unsigned long flag);
int pad_or_pin_number_cmp(const PadOrPinType* ppa, const PadOrPinType* ppb);

CheapPointType pad_center (PadType *pad);
CheapPointType pin_center (PinType *pin);
CheapPointType pad_or_pin_center(PadOrPinType* pp);

PadType *find_pad (ElementType *element, const char* number);
PinType *find_pin (ElementType *element, const char* number);
bool find_pad_or_pin (ElementType *element, const char* number, PadOrPinType* pp_ptr);

bool is_number_unique (ElementType *element, const char* number);

bool have_two_corresponding_non_coincident (ElementType *old_element,
                                            ElementType *new_element,
                                            PadOrPinType* old1_pp_ptr,
                                            PadOrPinType* old2_pp_ptr,
                                            PadOrPinType* new1_pp_ptr,
                                            PadOrPinType* new2_pp_ptr);
bool have_two_corresponding_unique_non_coincident (ElementType *old_element,
                                                   ElementType *new_element,
                                                   PadOrPinType* old1_pp_ptr,
                                                   PadOrPinType* old2_pp_ptr,
                                                   PadOrPinType* new1_pp_ptr,
                                                   PadOrPinType* new2_pp_ptr);
bool have_any_corresponding_pad_or_pin (ElementType *old_element,
                                        ElementType *new_element,
                                        PadOrPinType* old_pp,
                                        PadOrPinType* new_pp);

/*
 * Points
 */

CheapPointType make_point (Coord x, Coord y);
CheapPointType subtract_point(CheapPointType pt1, CheapPointType pt2);
double point_distance2(CheapPointType pt1, CheapPointType pt2);

/*
 * Logging
 */

void log_point(CheapPointType pt);

void log_pad_or_pin(const PadOrPinType* pp);
void log_pad (PadType *p);
void log_pin (PinType *p);
void log_element (ElementType *e);

#if DEBUG
#  define debug_log_point(pt) log_point(pt)
#  define debug_log_pad_or_pin(pp) log_pad_or_pin(pp)
#  define debug_log_pad(p) log_pad(p)
#  define debug_log_pin(p) log_pin(p)
#  define debug_log_element(e) log_element(e)
#else
#  define debug_log_point(pt)
#  define debug_log_pad_or_pin(pp)
#  define debug_log_pad(p)
#  define debug_log_pin(p)
#  define debug_log_element(e)
#endif

void base_log(const char *fmt, ...);
void debug_log(const char *fmt, ...);

/*
 * Miscellaneous
 */

double round(double v);
double multiple_of_90(double rad);
BYTE angle_to_rotation_steps(double rad);

/*
 * Macros
 */

#define FLAG_VALUE(o) ((o).f)

#define MAKE_PT(obj)                            \
    make_point((obj).X, (obj).Y)

#define IS_REFLECTED(elem1, elem2)              \
    (TEST_FLAG(ONSOLDERFLAG, (elem1))           \
     != TEST_FLAG(ONSOLDERFLAG, (elem2)));

/* "Hygienic" loop macros for nesting */

#define	PAD_LOOP_HYG(element, suffix)                                   \
    do {                                                                \
        Cardinal	n##suffix, sn##suffix;                          \
        PadType*	pad##suffix;                                    \
        for (sn##suffix = (element)->PadN, n##suffix = 0;               \
             (element)->PadN > 0 && n##suffix < (element)->PadN ;       \
             sn##suffix == (element)->PadN ? n##suffix++ : 0)           \
            {                                                           \
                pad##suffix = &(element)->Pad[n##suffix]


#define	PIN_LOOP_HYG(element, suffix)                                   \
    do {                                                                \
        Cardinal	n##suffix, sn##suffix;                          \
        PinType*	pin##suffix;                                    \
        for (sn##suffix = (element)->PinN, n##suffix = 0;               \
             (element)->PinN > 0 && n##suffix < (element)->PinN ;       \
             sn##suffix == (element)->PinN ? n##suffix++ : 0)           \
            {                                                           \
                pin##suffix = &(element)->Pin[n##suffix]

#define PAD_OR_PIN_LOOP_HYG(element, suffix)                            \
    do {                                                                \
        Cardinal   n##suffix = 0;                                       \
        Cardinal   internal_n##suffix = 0;                              \
        bool       pad_done##suffix = false;                            \
        while (1) {                                                     \
            PadOrPinType pp##suffix = make_pad_or_pin(NULL, NULL);      \
            if (! pad_done##suffix) {                                   \
                if (internal_n##suffix < (element)->PadN) {             \
                    n##suffix = internal_n##suffix++;                   \
                    pp##suffix =                                        \
                        make_pad_or_pin(&(element)->Pad[n##suffix],     \
                                        NULL);                          \
                } else {                                                \
                    pad_done##suffix = true;                            \
                    internal_n##suffix = 0;                             \
                }                                                       \
            }                                                           \
            if (pad_done##suffix) {                                     \
                if (internal_n##suffix < (element)->PinN) {             \
                    n##suffix = internal_n##suffix++;                   \
                    pp##suffix =                                        \
                        make_pad_or_pin(NULL,                           \
                                        &(element)->Pin[n##suffix]);    \
                                                                        \
                } else {                                                \
                    break;                                              \
                }                                                       \
            }

#define PAD_OR_PIN_LOOP(element) PAD_OR_PIN_LOOP_HYG(element, )

#endif
