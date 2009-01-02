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


#ifndef __TYPES_INCLUDED__
#define __TYPES_INCLUDED__

#define MALLOC_INC 64 
#define EDGE_POOL_MALLOC_INC 64
#define POINT_POOL_MALLOC_INC 64
#define PROBLEM_POOL_MALLOC_INC 64
#define TRIANGLE_POOL_MALLOC_INC 64

#define HEAP_PARENT(root) ((root-1)/2)
#define HEAP_LEFT(root) (2*root+1)
#define HEAP_RIGHT(root) (2*root+2)

#ifdef TRACE
  #include <cairo.h>
#endif

/*
 * pointclass_t enumerates the possible types
 * for a point used throughout the autorouter.
 */
typedef enum { 
  NORMAL = 0,
  VIA = 1, 
  PAD = 2, 
  PIN = 3, 
  STEINER = 4, 
  CROSSPOINT = 5,
  INTERSECT = 6,
  VIA_CAND = 7,
  CORNER = 8,
  TCSPOINT = 9
} pointclass_t;

typedef enum {
  CLOCKWISE = -1,
  COLINEAR = 0,
  COUNTERCLOCKWISE = 1,
  UNKNOWN = 9
} dir_t;	

/* just a vector in 3-space.. */
typedef struct vector_t { double i, j, k; } vector_t;

/* 
 * point_t is a point on the place with a few
 * other attributes 
 */
typedef struct point_t {
  /* x, y are the location on the plane */
  int x, y;

  /* 
   * points are grouped into clusters 
   * during Kruskal's algorithm for MST.
   * This should probably be removed and stored
   * in a temporary parallel array 
   */
  int cluster;

  /* the type of point.. */
  pointclass_t type;

  /* the netlist this point belongs to
   * -1 if unassigned
   */
  int netlist;

  /* the layer this point is assigned to or
   * -1 if unassigned or applys to all layers
   *  e.g., a VIA or PIN.
   * prevlayer is used during path scoring
   */
  int layer, prevlayer;
 
  /* these two pointers are for A* path search algo. */
  struct edge_t *edge;
  struct point_t *parent;

  struct point_t *pull_point;

  int clearance_radius;
  int thickness, clearance; 

} point_t;

/*
 * triangle_t is the primary data structure 
 * used during triangulation
 */
typedef struct triangle_t {
  /* the three vectices */
  point_t * points[3];

  /* the three adjacent triangles */
  struct triangle_t* adj[3];

  /* in the incremental search algorithm, a history
   * of all the triangles is maintained. each
   * triangle may have 2 or 3 children if it 
   * has been split into other triangles as part of 
   * a point insertion at some stage */
  struct triangle_t* children[3];
  int children_n;

} triangle_t;

/*
 * A slab allocator pool for triangles. 
 * This will be replaced with the generic 
 * pool structure 
 */
typedef struct triangle_pool_t {
  
  struct triangle_t *triangles;
  int triangles_n;
 
  struct triangle_pool_t *next;
} triangle_pool_t;


/*
 * A slab allocator pool for edges. 
 * This will be replaced with the generic 
 * pool structure 
 */
typedef struct edge_pool_t {
  
  struct edge_t *edges;
  int edges_n;
 
  struct edge_pool_t *next;
} edge_pool_t;

/*
 * triangulation_t contains the data for a 
 * triangulation and mst
 */
typedef struct triangulation_t {
  /* root triangle is the original triangle 
   * containing the 3 special points.
   * it will not be seen by users of the 
   * triangulation
   */
  triangle_t *root_triangle;

  /* triangles are slab allocated */
  triangle_pool_t *triangle_pools;
  int triangles_max, triangles_n;

  /* edges are slab allocated */
  edge_pool_t *edge_pools;
  int edges_max, edges_n;

  /* a sorted array of the edges */
  struct edge_t **sorted_edges;
  /* an array containing the MST subset of the
   * triangulation
   */
  struct edge_t **mst_edges;

  /* the points of this triangulation are also
   * pointed to by this array 
   */
  struct point_t **points;
  int points_max, points_n;

  /* the three special points (particular 
   * to the incremental search algorithm) 
   */
  struct point_t spoints[3];

  /* the maximum co-ordinate must be known
   * in order to place the special points 
   */
  int max_coord;
#ifdef DEBUG
  int debug;
#endif

} triangulation_t;

/*
 * holds a triangulation of a netlist and 
 * its corrosponding name, width and spacing
 * rules (incomplete..)
 */
typedef struct netlist_t {

  triangulation_t *triangulation;

  int width, spacing;

  char *name;
  char *style;
} netlist_t;

#define EDGEFLAG_HILIGHT      (1<<0)
#define EDGEFLAG_INACTIVE     (1<<1)
#define EDGEFLAG_CONSTRAINED  (1<<2)
#define EDGEFLAG_FORCEVIA     (1<<3)
#define EDGEFLAG_EXPORTED     (1<<4)
#define EDGEFLAG_NOTROUTED    (1<<5)

/*
 * edge_t is a line segment defined by two
 * points
 */
typedef struct edge_t {
  /* the two points.. */
  point_t *p1, *p2;

  /* length is cached */
  double length;

  /* the layer, if assigned (otherwise -1) */
  int layer;
  /* the routing order, defined by the NOP */
	int order;
  
  /* flags for this edge, as above */
  int flags;

  /* mark is used to detect closed nets */
  int mark;

  /* a list of TCS cross points assigned */
  struct linklist_t *tcs_points;

  struct linklist_t *segments;
} edge_t;

/* 
 * a generic linklist data structure
 */
typedef struct linklist_t {
  void *data;

  struct linklist_t *next;

} linklist_t;

/*
 * a generic pool for the slab allocator
 */
typedef struct generic_pool_t {
  void *data;
  int data_n;
 
  struct generic_pool_t *next;
} generic_pool_t;

/*
 * each problem contains n solutions, where n is the number
 * of layers
 */
typedef struct solution_t {

  generic_pool_t *edge_pools;
  int pool_edges_n;

} solution_t;


/*
 * the board is divided into sub-problems, the 
 * data of which is stored in these structures 
 */
typedef struct problem_t {
  /* the co-ordinates of the sub-problem */
  int x1,y1,x2,y2; 
  int width, height;

  /* a problem can be split into two child 
   * problems, which are pointed to by 
   * left and right 
   */
  struct problem_t *left, *right;

  /* an array of pointers to the points 
   * contained within this subproblem
   */
  point_t **points;
  int points_n, points_max;

  /* a pointer to special points */
  point_t *spoints;

  /* an array of pointers to edges
   * (usually contained within a triangulation) 
   */
  edge_t **edges;
  int edges_n, edges_max;

  /* edges are slab allocated */ 
  edge_pool_t *edge_pools;
  int pool_edges_n;

  /* triangulation pointers. 
   * at some stages, there is a triangulation 
   * for each netlist with the problem. at 
   * other stages, there is a triangulation 
   * for each layer. 
   */
  triangulation_t *ts;
  int ts_n;

  /* the cut is either CUTX or CUTY */ 
  int cut_orientation;
  /* the position of the cut */
  int cut_loc;
  /* the flow density of chosen cut */
  float cut_flow_density;

  /* these keep track of points crossing the cut */
  int *cut_crossing_netlists;
  int cut_crossing_netlists_n;
  int *cut_crossing_netlist_slots;
  int *cut_crossing_netlist_layers;

  /* solutions */
  solution_t *solns;

  linklist_t *closelist;
  linklist_t *openlist;

} problem_t;


#define ROUTING_DIR_HORZ 1
#define ROUTING_DIR_VERT 2

/*
 * drawing_context_t contains data for drawing 
 * the traces with cairo 
 */
#ifdef TRACE
typedef struct drawing_context_t {
  cairo_t *cr;
  cairo_surface_t *surface;

  char *prefix;
  int x_offset, y_offset;
  int w, h;
  int resx, resy;
} drawing_context_t;
#endif /* TRACE */ 


/*
 * A slab allocator pool for points. 
 * This will be replaced with the generic 
 * pool structure 
 */
typedef struct point_pool_t {
  
  struct point_t *points;
  int points_n;
 
  struct point_pool_t *next;
} point_pool_t;

/*
 * A slab allocator pool for problems. 
 * This will be replaced with the generic 
 * pool structure 
 */
typedef struct problem_pool_t {
  
  struct problem_t *problems;
  int problems_n;
 
  struct problem_pool_t *next;
} problem_pool_t;

/*
 * the main autorouter data structure. 
 * this contains state data, 
 * and also parameters.
 */
typedef struct autorouter_t {
  /* number of layers.. */
  int board_layers;
  /* board dimensions */
  int board_width, board_height;

  /* an array of routing directions for each layer
   * i don't think this is currently being used 
   * by the cost functions 
   */
  int *routing_direction;

  /*
   * data maintained by the tracing 
   * procedures
   */
#ifdef TRACE
  char *last_prefix;
  int traceseq;
#endif /* TRACE */
  
  /* trace parameters */
  int trace_width, trace_height;
  char *trace_prefix;

  /* PARAMETERS: */

  /* if the critical flow threshold is reached, 
   * a maximum rather than a minimum cut is chosen. 
   * this is so that areas of dense wiring are 
   * considered earlier rather than later
   */
  float critical_flow_threshold; 
  
  /* the min netlists for a problem */
  int min_sub_prob_netlists;
  
  /* the min points for a problem */
  int min_sub_prob_points;
  
  /* the min aspect ratio of a problem */
  float min_sub_prob_aspect_ratio;
  
  /* min clearance between a cut and a point within 
   * a problem 
   */
  int min_cut_to_point_seperation;
  
  /* coeffcient applied to segments violating 
   * routing direction suggestion
   */
  float routing_direction_penalty;
  
  /* via cost... */
  int via_cost;

  /* these are defaults if the data isn't 
   * contained in PCB for a net 
   */
  int default_spacing, default_width;

  /* the max number of segments a netline will
   * be diced into (and thus the max number of
   * vias that will be considered)
   */
  int max_assign_segments;
  
  /* these are incomplete, data should come from 
   * somewhere */
  int via_clearance, via_radius;

  /* used for detecting closed nets */
  int rcounter;

  /* data */
  netlist_t *netlists;
  int netlists_n; int netlists_max;
 
  /* the primary points slab allocator */
  point_pool_t *point_pools;
  int points_n;

  /* sub-problems are allocated by a slab 
   * allocater 
   */
  problem_pool_t *problem_pools;
  int problems_n;
  
  /* pointer to the root problem */
  problem_t *root_problem;

  /* keeps count of route errors */
  int bad_routes;


  int default_keepaway;
  int default_linethickness;
  int default_viathickness;

} autorouter_t;


#endif /* __TYPES_INCLUDED__ */
