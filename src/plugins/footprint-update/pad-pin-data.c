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
 * $Id: pad-pin-data.c,v 1.2 2008-05-22 04:52:13 dean Exp $
 * Dean Ferreyra
 */

#include "matrix.h"
#include "pad-pin-data.h"

static bool calculate_transformation (CheapPointType new1_pt, CheapPointType new2_pt, CheapPointType old1_pt, CheapPointType old2_pt, bool reflect, double angle, Matrix3x3 new_to_old_mat);
static void transform_pad_pin_data (ElementPadPinData* ppd, int len, Matrix3x3 trans);
static double calculate_sum_of_distances (ElementPadPinData* ppd_a, int len_a, ElementPadPinData* ppd_b, int len_b);
static bool find_number_block (const ElementPadPinData* ppd, int len, const PadOrPinType* pp, int* start, int* end);
static int pad_pin_data_cmp_by_number (const void* va, const void* vb);

ElementPadPinData*
alloc_pad_pin_data_array (ElementType *element, int* len_ptr)
{
  int len;
  int i;
  ElementPadPinData* ppd = NULL;

  len = element->PadN + element->PinN;
  realloc ((ElementPadPinData*) ppd, len * sizeof (ElementPadPinData));

  /* Set the pad/pin pointers and centers */
  i = 0;
  PAD_OR_PIN_LOOP(element);
  {
    ppd[i].pp = pp;
    ppd[i].center
      = ppd[i].transformed_center
      = pad_or_pin_center(&pp);
    i++;
  }
  END_LOOP;

  /* Sort by pad/pin number */
  qsort(ppd, len, sizeof(ElementPadPinData), pad_pin_data_cmp_by_number);

  /* Set the shared field */
  int i_start = 0;
  const PadOrPinType* pp_start = &ppd[0].pp;
  /* Index i goes all the way to len; watch out. */
  for (i = 1; i < len + 1; i++) {
    if (i == len || pad_or_pin_number_cmp(pp_start, &ppd[i].pp) != 0) {
      int shares = i - i_start;
      int j;
      for (j = i_start; j < i; j++) {
        ppd[j].shares = shares;
      }
      /* Prepare for next block */
      if (i != len) {
        i_start = i;
        pp_start = &ppd[i].pp;
      }
    }
  }
  *len_ptr = len;
  return ppd;
}

double
find_non_coincident(const ElementPadPinData* ppd, int len,
                    int* index1_ptr, int* index2_ptr)
{
  int index1 = 0;
  const ElementPadPinData* ppd1 = &ppd[index1];
  int i;
  for (i = 1; i < len; i++) {
    double d2 = point_distance2(ppd1->center, ppd[i].center);
    if (d2) {
      *index1_ptr = index1;
      *index2_ptr = i;
      return d2;
    }
  }
  return 0;
}

bool
find_best_corresponding_pads_or_pins(ElementPadPinData* ppd_a, int len_a,
                                     int index1_a, int index2_a,
                                     bool reflect,
                                     ElementPadPinData* ppd_b, int len_b,
                                     int* index1_b_ptr, int* index2_b_ptr)
{
  int block1_start = 0;
  int block1_end = 0;
  int block2_start = 0;
  int block2_end = 0;
  if (! find_number_block(ppd_b, len_b, &ppd_a[index1_a].pp,
                          &block1_start, &block1_end)) {
    return false;
  }
  if (! find_number_block(ppd_b, len_b, &ppd_a[index2_a].pp,
                          &block2_start, &block2_end)) {
    return false;
  }

  bool min_set = false;
  double min_sum_of_distances = 0;
  int min_index1_b = 0;
  int min_index2_b = 0;

  int i;
  for (i = block1_start; i < block1_end; i++) {
    int j;
    for (j = block2_start; j < block2_end; j++) {
      /* Block1 and block2 ranges may coincide. */
      if (i != j) {
        int k;
        for (k = 0; k < 4; k++) {
          /* Find transformation */
          double angle = k * M_PI / 2;
          Matrix3x3 trans;
          if (calculate_transformation(ppd_a[index1_a].center,
                                       ppd_a[index2_a].center,
                                       ppd_b[i].center,
                                       ppd_b[j].center,
                                       reflect, angle,
                                       trans)) {
            /* Transform ppd_a */
            transform_pad_pin_data(ppd_a, len_a, trans);
            /* Sum of distances */
            double sd = calculate_sum_of_distances(ppd_a, len_a,
                                                   ppd_b, len_b);
            if (! min_set || sd < min_sum_of_distances) {
              min_set = true;
              min_sum_of_distances = sd;
              min_index1_b = i;
              min_index2_b = j;
            }
          }
        }
      }
    }
  }
  if (min_set) {
    debug_log("Best corresponding pads/pins found.\n");
    *index1_b_ptr = min_index1_b;
    *index2_b_ptr = min_index2_b;
  } else {
    debug_log("No best corresponding pads/pins found.\n");
  }
  return min_set;
}

static void
transform_pad_pin_data(ElementPadPinData* ppd, int len,
                       Matrix3x3 trans)
{
  int i;
  for (i = 0; i < len; i++) {
    Vector3x1 v;
    point_to_vec(ppd[i].center, v);
    Vector3x1 t;
    multiply_matrix_vector(trans, v, t);
    ppd[i].transformed_center = vec_to_point(t);
  }
}

/* Isn't guaranteed to find the minimum distances between all
   corresponding pads/pins when pad/pin numbers are shared, but it
   takes a pretty good stab at it. */
static double
calculate_sum_of_distances(ElementPadPinData* ppd_a, int len_a,
                           ElementPadPinData* ppd_b, int len_b)
{
  /* Clear the taken flags */
  int i;
  for (i = 0; i < len_b; i++) {
    ppd_b[i].taken = false;
  }

  double sum_d2 = 0;
  for (i = 0; i < len_a; i++) {
    int block_b_start = 0;
    int block_b_end = 0;
    ElementPadPinData* a = &ppd_a[i];
    if (find_number_block(ppd_b, len_b, &a->pp,
                          &block_b_start, &block_b_end)) {
      bool min_set = false;
      double min_d2 = 0;
      int min_index = 0;
      int j;
      for (j = block_b_start; j < block_b_end; j++) {
        ElementPadPinData* b = &ppd_b[j];
        if (! b->taken) {
          double d2 = point_distance2(a->transformed_center,
                                      b->transformed_center);
          if (! min_set || d2 < min_d2) {
            min_set = true;
            min_d2 = d2;
            min_index = j;
          }
        }
      }
      if (min_set) {
        ppd_b[min_index].taken = true;
        sum_d2 += min_d2;
      }
    }
  }
  return sum_d2;
}

bool
calculate_transformation(CheapPointType new1_pt, CheapPointType new2_pt,
                         CheapPointType old1_pt, CheapPointType old2_pt,
                         bool reflect,
                         double angle,
                         Matrix3x3 new_to_old_mat)
{
  Matrix3x3 t;

  /* Translation of new1_pt to origin. */
  Matrix3x3 new_to_origin_mat;
  make_translation_matrix(new_to_origin_mat, -new1_pt.X, -new1_pt.Y);
  if (reflect) {
    /* If components are not on same side of board, reflect through
       the y-axis. */
    make_reflection_matrix_y_axis(t);
    multiply_matrix_matrix_inplace(t, new_to_origin_mat);
  }

  Matrix3x3 result;
  /* Start with translation of new1_pt to origin. */
  copy_matrix(new_to_origin_mat, result);
  /* Rotate from new element angle to old element angle. */
  make_rotation_matrix(t, angle);
  multiply_matrix_matrix_inplace(t, result);
  /* Translate from origin to old1_pt. */
  make_translation_matrix(t, old1_pt.X, old1_pt.Y);
  multiply_matrix_matrix_inplace(t, result);

  copy_matrix(result, new_to_old_mat);
  return true;
}

static bool
find_number_block(const ElementPadPinData* ppd, int len,
                  const PadOrPinType* pp,
                  int* start, int* end)
{
  int i;
  for (i = 0; i < len; i++) {
    if (pad_or_pin_number_cmp(&ppd[i].pp, pp) == 0) {
      int j;
      for (j = i + 1; j < len; j++) {
        if (pad_or_pin_number_cmp(&ppd[j].pp, pp) != 0) {
          break;
        }
      }
      *start = i;
      *end = j;
      return true;
    }
  }
  return false;
}

static int
pad_pin_data_cmp_by_number(const void* va, const void* vb)
{
  const ElementPadPinData* a = (const ElementPadPinData*)va;
  const ElementPadPinData* b = (const ElementPadPinData*)vb;
  return pad_or_pin_number_cmp(&a->pp, &b->pp);
}
