/*
 * matrix
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
 * $Id: matrix.c,v 1.3 2008-05-22 05:29:29 dean Exp $
 */

#include "matrix.h"

/*
 * 3x1 Vectors
 */

void
make_vec(Vector3x1 vec, double x, double y)
{
  vec[0][0] = x;
  vec[1][0] = y;
  vec[2][0] = 1;
}

CheapPointType
vec_to_point(Vector3x1 vec)
{
  return make_point(round(vec[0][0]), round(vec[1][0]));
}

void
point_to_vec(CheapPointType pt, Vector3x1 vec)
{
  make_vec(vec, pt.X, pt.Y);
}

/*
 * 3x3 Matrices
 */

void
copy_matrix(Matrix3x3 src, Matrix3x3 dst)
{
  memcpy(dst, src, sizeof(Matrix3x3));
}

void
make_identity_matrix(Matrix3x3 mat)
{
  memset(mat, 0, sizeof(Matrix3x3));
  int i;
  for (i = 0; i < 3; i++) {
    mat[i][i] = 1;
  }
}

void
make_translation_matrix(Matrix3x3 mat, double dx, double dy)
{
  make_identity_matrix(mat);
  mat[0][2] = dx;
  mat[1][2] = dy;
}

void
make_rotation_matrix(Matrix3x3 mat, double theta)
{
  double c = cos(theta);
  double s = sin(theta);
  make_identity_matrix(mat);
  mat[0][0] = c; mat[0][1] = -s;
  mat[1][0] = s; mat[1][1] = c;
}

void
make_reflection_matrix_x_axis(Matrix3x3 mat)
{
  make_identity_matrix(mat);
  mat[1][1] = -1;
}

void
make_reflection_matrix_y_axis(Matrix3x3 mat)
{
  make_identity_matrix(mat);
  mat[0][0] = -1;
}

void
multiply_matrix_vector(Matrix3x3 mat, Vector3x1 vec,
                       Vector3x1 result)
{
  int i;
  for (i = 0; i < 3; i++) {
    double s = 0;
    int j;
    for (j = 0; j < 3; j++) {
      s += mat[i][j] * vec[j][0];
    }
    result[i][0] = s;
  }
}

void
multiply_matrix_matrix(Matrix3x3 mat1, Matrix3x3 mat2,
                       Matrix3x3 result)
{
  int k;
  for (k = 0; k < 3; k++) {
    int i;
    for (i = 0; i < 3; i++) {
      double s = 0;
      int j;
      for (j = 0; j < 3; j++) {
        s += mat1[i][j] * mat2[j][k];
      }
      result[i][k] = s;
    }
  }
}

void
multiply_matrix_matrix_inplace(Matrix3x3 mat1, Matrix3x3 mat2)
{
  Matrix3x3 result;
  multiply_matrix_matrix(mat1, mat2, result);
  memcpy(mat2, result, sizeof(Matrix3x3));
}

CheapPointType
transform_point(Matrix3x3 mat, CheapPointType pt)
{
  Vector3x1 vec;
  Vector3x1 vec_result;

  point_to_vec(pt, vec);
  multiply_matrix_vector(mat, vec, vec_result);
  return vec_to_point(vec_result);
}

/*
 * Logging
 */

void
log_vector(Vector3x1 vec)
{
  base_log("Vector\n");
  int i;
  for (i = 0; i < 3; i++) {
    base_log("%7.4f\n", vec[i][0]);
  }
}

void
debug_log_vector(Vector3x1 vec)
{
#if DEBUG
  log_vector(vec);
#endif
}

void
log_matrix(Matrix3x3 mat)
{
  base_log("Matrix\n");
  int i;
  for (i = 0; i < 3; i++) {
    base_log("%7.4f %7.4f %7.4f\n",
             mat[i][0], mat[i][1], mat[i][2]);
  }
}

void
debug_log_matrix(Matrix3x3 mat)
{
#if DEBUG
  debug_log_matrix(mat);
#endif
}

