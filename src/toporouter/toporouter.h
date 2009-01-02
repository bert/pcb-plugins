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

#ifndef __TOPOROUTER_INCLUDED__
#define __TOPOROUTER_INCLUDED__

#include "types.h"
#include "triangle.h"

#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

/* trace stuff */
#ifdef TRACE
  #include <cairo.h>
#endif

/* PCB includes */
#ifndef TEST
#include "data.h"
#include "macro.h"
#include "autoroute.h"
#include "box.h"
/*#include "clip.h"*/
#include "create.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "heap.h"
#include "rtree.h"
#include "misc.h"
//#include "mtspace.h"
#include "mymem.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "thermal.h"
#include "undo.h"
//#include "vector.h"
#include "global.h"
//  #include "rats.h"
//  #include "data.h"
//  #include "hid.h"
//  #include "misc.h"
//  #include "create.h"
//  #include "rtree.h"
//  #include "undo.h"
//  #include "find.h"
#else
  #define MIN(x,y) ((x<y)?x:y)
  #define MAX(x,y) ((x>y)?x:y)
  #define M_PI 3.14159265358979323846
#endif

#ifdef TRACE
  #include <cairo.h>
#endif
#define DRAW_POINTS       (1<<0)
#define DRAW_MSTS         (1<<1)
#define DRAW_TRIAGS       (1<<2)
#define DRAW_EDGES        (1<<3)
#define DRAW_ORDERS       (1<<4)
#define DRAW_PROB         (1<<5)
#define DRAW_SOLN_EDGES   (1<<6)
#define DRAW_CLOSELIST    (1<<7)

#define TRCE_POINTS                 (1<<0)
#define TRCE_EDGES                  (1<<1)
#define TRCE_CIRCUMCIRCLES          (1<<2)
#define TRCE_CIRCUMCIRCLE_LABELS    (1<<3)
#define TRCE_TRIANGULATIONS         (1<<4)

#define MAXDIM(r) (1*((r->board_width > r->board_height) ? r->board_width : r->board_height))

#define TAR_END_LOOP }} while (0)
#define TAR_END_FOREACH }} while (0)

#define LTRIAG(p,i) (&(p)->l_triags[(i)])
#define RTRIAG(p,i) (&(p)->r_triags[(i)])

#define CUTX 1
#define CUTY 2
#define CUT_NONE 3
/*
#define TAR_NETLIST_LOOP(top) do { \
  int n; \
  netlist_t *netlist; \
  for(n = 0; n < (top)->netlists_n; n++) { \
    netlist = &(top)->netlists[n];

#define TAR_POINTS_LOOP(top) do { \
  int n; \
  point_t *point; \
  for(n = (top)->points_n-1; n!=-1; n--) { \
    point = &(top)->points[n];

#define TAR_POINTS_FWD_LOOP(top) do { \
  int n; \
  point_t *point; \
  for(n = 0; n<(top)->points_n; n++) { \
    point = (top)->points[n];
*/

#define TAR_PROB_POINTS_LOOP(top) do { \
  int n; \
  point_t *point; \
  for(n = 0; n<(top)->points_n; n++) { \
    point = (top)->points[n];

#define TAR_AR_POINTS_LOOP(top) do { \
  int n; \
  point_t *point; \
  for(n = 0; n<(top)->points_n; n++) { \
    point = &(top)->points[n];

#define TAR_LTRIAGS_LOOP(top) do { \
  int n; \
  triangulation_t *triag; \
  for(n = 0; n<(top)->l_triags_n; n++) { \
    triag = &(top)->l_triags[n];

#define TAR_RTRIAGS_LOOP(top) do { \
  int n; \
  triangulation_t *triag; \
  for(n = 0; n<(top)->r_triags_n; n++) { \
    triag = &(top)->r_triags[n];

#define TAR_CUTCANDS_LOOP(top) do { \
  int n; \
  cutcand_t *cc; \
  for(n = 0; n<(top)->cutcands_n; n++) { \
    cc = &(top)->cutcands[n];

#define TAR_FOREACH_POINTS(top) do { \
  int n; \
  point_t *point; \
  for(n = 0; n < (top)->points_n; n++) { \
    point = (top)->points[n];

#define TAR_EDGES_LOOP(top) do { \
  int n,edge_count=0; \
  edge_pool_t *cur_edge_pool = (top)->edge_pools; \
  while(cur_edge_pool != NULL) { \
    edge_t *edge; \
    for(n=cur_edge_pool->edges_n-1;n!=-1;n--) { \
      edge = &cur_edge_pool->edges[n]

#define TAR_EDGES_ENDLOOP \
      edge_count++;\
      } \
    cur_edge_pool = cur_edge_pool->next; \
  } } while(0)

#define TAR_MST_EDGES_LOOP(top) do{\
  int n; \
  edge_t *edge; \
  if((top)->mst_edges != NULL) \
  for(n=0;n < (top)->points_n-1;n++) { \
    edge = (top)->mst_edges[n];


#define MAX_BOARD_DIM(x) ((x->board_width > x->board_height) ? x->board_width : x->board_height)


point_t *netlist_alloc_point(netlist_t *n);
void netlist_destroy(netlist_t *nl); 
void autorouter_destroy(autorouter_t *r); 

void trace(autorouter_t *ar, problem_t *prob, char *prefix, int flags, int layer);
void trace_pause(autorouter_t *ar); 


#endif /* __CORE_INCLUDED__ */
