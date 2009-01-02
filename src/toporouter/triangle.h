/*
 *                            COPYRIGHT
 *
 *  Topological Autorouter for 
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2008 Anthony Blake
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and email:
 *  Anthony Blake, Dept. Computer Science, University of Waikato, 
 *  Hamilton, New Zealand
 *  amb33@cs.waikato.ac.nz
 *
 */


#ifndef __TRIANGLE_INCLUDED__
#define __TRIANGLE_INCLUDED__

#include "types.h"
#include "toporouter.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define EDGE_PARENT(root,e) (&(root)[(e-1)/2])
#define EDGE_LEFT(root,e) (&(root)[(2*e)+1])
#define EDGE_RIGHT(root,e) (&(root)[(2*e)+2])

//#define NULL_TRI -31337
//#define NULL_PNT -31337

int wind(point_t *p1, point_t *p2, point_t *p3);
double length(double p1x, double p1y, double p2x, double p2y) ;
inline double area2D_triangle(point_t *v0, point_t *v1, point_t *v2 );

void triangulation_create(int max_coord, triangulation_t *t);
void triangulation_destroy(triangulation_t *t);
void triangulation_reset(triangulation_t *t);
int triangulation_insert_point(triangulation_t *par, point_t *p);
//void triangulation_insert_points(triangulation_t *t, netlist_t *nl);

//void triangulation_rollback(triangulation_t *t, int points_n);

void triangulation_compute_mst(autorouter_t *ar, triangulation_t *t, int draw);

edge_t *triangulation_alloc_edge(triangulation_t *t);

int root_point(triangulation_t *par, point_t *p);
int root_edge(triangulation_t *par, edge_t *e);

#endif /* __TRIANGLE_INCLUDED__ */
