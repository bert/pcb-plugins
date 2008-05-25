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
 * $Id: matrix.h,v 1.3 2008-05-22 04:52:13 dean Exp $
 */

#ifndef MATRIX_H_INCLUDED
#define MATRIX_H_INCLUDED 1

#include "utilities.h"

/*
 * 3x1 Vectors
 */

typedef double Vector3x1[3][1]; /* [row][column] */

void make_vec(Vector3x1 vec, double x, double y);
CheapPointType vec_to_point(Vector3x1 vec);
void point_to_vec(CheapPointType pt, Vector3x1 vec);

/*
 * 3x3 Matrices
 */

typedef double Matrix3x3[3][3]; /* [row][column] */

void make_identity_matrix(Matrix3x3 mat);
void make_translation_matrix(Matrix3x3 mat, double dx, double dy);
void make_rotation_matrix(Matrix3x3 mat, double theta);
void make_reflection_matrix_x_axis(Matrix3x3 mat);
void make_reflection_matrix_y_axis(Matrix3x3 mat);
void copy_matrix(Matrix3x3 src, Matrix3x3 dst);

void multiply_matrix_vector(Matrix3x3 mat, Vector3x1 vec,
                            Vector3x1 result);
void multiply_matrix_matrix(Matrix3x3 mat1, Matrix3x3 mat2,
                            Matrix3x3 result);
void multiply_matrix_matrix_inplace(Matrix3x3 mat1, Matrix3x3 mat2);
CheapPointType transform_point(Matrix3x3 mat, CheapPointType pt);

/*
 * Logging
 */

void log_vector(Vector3x1 vec);
void debug_log_vector(Vector3x1 vec);
void log_matrix(Matrix3x3 mat);
void debug_log_matrix(Matrix3x3 mat);

#endif
