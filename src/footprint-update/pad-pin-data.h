/*
 * pad-pin-data
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
 * $Id: pad-pin-data.h,v 1.2 2008-05-22 04:52:13 dean Exp $
 * Dean Ferreyra
 */

#ifndef PAD_PIN_DATA_H_INCLUDED
#define PAD_PIN_DATA_H_INCLUDED 1

#include "utilities.h"

typedef struct {
  PadOrPinType pp;
  CheapPointType center;
  int shares;
  Boolean taken;
  CheapPointType transformed_center;
} ElementPadPinData;

ElementPadPinData* alloc_pad_pin_data_array(ElementTypePtr element,
                                            int* len_ptr);
double find_non_coincident(const ElementPadPinData* ppd, int len,
                           int* index1_ptr, int* index2_ptr);
Boolean find_best_corresponding_pads_or_pins(ElementPadPinData* ppd_a,
                                             int len_a,
                                             int index1_a, int index2_a,
                                             Boolean reflect,
                                             ElementPadPinData* ppd_b,
                                             int len_b,
                                             int* index1_ptr,
                                             int* index2_ptr);

#endif
