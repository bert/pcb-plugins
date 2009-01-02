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

/*
 * This file implements Delaunay triangulation in O(n log n) time, using the 
 * randomized incremental method described in:
 *
 * De Berg, M. and van Kreveld, M. and Overmars, M. and Schwarzkopf, O.
 * "Computational geometry: Algorithms and applications", 2008,
 * Springer-Verlag.
 *
 * Related papers:
 *
 * Geoff Leach. Improving worst-case optimal Delaunay triangulation algorithms.
 * In 4th Canadian Conference on Computational Geometry, 1992.
 *
 * Guibas, L. and Stolfi, J. 1985. Primitives for the manipulation of general
 * subdivisions and the computation of Voronoi. ACM Trans. Graph. 4, 2 (Apr.
 * 1985), 74-123.
 * 
 * Also implemented in this file is Euclidean minimum spanning tree, computed
 * as a subset of the Delaunay triangulation in O(n) time, using Kruskal's
 * algorithm:
 *
 * Joseph. B. Kruskal: On the Shortest Spanning Subtree of a Graph and the
 * Traveling Salesman Problem. In: Proceedings of the American Mathematical
 * Society, Vol 7, No. 1 (Feb, 1956), pp. 48–50.
 *
 */

#include "triangle.h"


void
edges_heap_swap(edge_t **edges, int x, int y) {
  edge_t *temp = edges[x];
  edges[x] = edges[y];
  edges[y] = temp;
}


void
edges_heap_sift_down(triangulation_t *par, int start, int end) {
  int n;
  for(n = start; HEAP_LEFT(n) <= end; ) {
    int child = HEAP_LEFT(n);
    if(child<end && par->sorted_edges[child]->length < par->sorted_edges[child+1]->length) 
      child++;
    if(par->sorted_edges[n]->length < par->sorted_edges[child]->length) {
      edges_heap_swap( par->sorted_edges, n, child );
      n = child;
    }
    else return;
  }
}

void
edges_heapify(triangulation_t *par) {
  int n;
  for(n=HEAP_PARENT(par->edges_n); n >= 0; n--) {
    edges_heap_sift_down(par, n, par->edges_n-1);
  }
}

void
triangulation_sort_edges(triangulation_t *t) {
  if(t->sorted_edges != NULL) {
    free(t->sorted_edges);
    t->sorted_edges = NULL;
  }

  t->sorted_edges = calloc(t->edges_n, sizeof(edge_t *));


  TAR_EDGES_LOOP(t);
  {
    t->sorted_edges[edge_count] = edge;
  }
  TAR_EDGES_ENDLOOP;
  
  
  edges_heapify(t);

  int n;
  for(n=t->edges_n-1;n>0;n--) {
    edges_heap_swap(t->sorted_edges, 0, n);  
    edges_heap_sift_down(t, 0, n-1);
  }

}


void
triangulation_compute_mst(autorouter_t *ar, triangulation_t *t, int draw) {
  triangulation_sort_edges(t);

  if(t->mst_edges != NULL) {
    free(t->mst_edges);
    t->mst_edges = NULL;
  }

  t->mst_edges = calloc(t->points_n-1, sizeof(edge_t*));

  TAR_FOREACH_POINTS(t);
  {
    point->cluster = n;
  }
  TAR_END_FOREACH;

  int n,mst_n=0;
  for(n=0;n<t->edges_n;n++) { 
    if(mst_n >= t->points_n - 1) break;
    edge_t *edge = t->sorted_edges[n];

    if(edge->p1->cluster != edge->p2->cluster) {
      t->mst_edges[mst_n++] = edge;
      int old_cluster = edge->p2->cluster;

      if(draw) {
        int temppointsn = t->points_n;
        t->points_n = 0;
        
        trace(ar, ar->root_problem, "mst", DRAW_POINTS | DRAW_TRIAGS | DRAW_MSTS, -1);

        t->points_n = temppointsn;
      }

      TAR_FOREACH_POINTS(t);
      {
        if(point->cluster == old_cluster) {
          point->cluster = edge->p1->cluster;
        }
      }
      TAR_END_FOREACH;
    }

  }
  
}

double
length(double p1x, double p1y, double p2x, double p2y) {
  return sqrt( pow(p1x - p2x, 2) + pow(p1y - p2y, 2) );
}

edge_pool_t *
alloc_edge_pool(triangulation_t *t) {
  edge_pool_t *ep = calloc(1, sizeof(edge_pool_t));

  ep->edges = calloc(EDGE_POOL_MALLOC_INC, sizeof(edge_t));
  ep->next = NULL;
  ep->edges_n = 0;

//  printf("NEW EDGE POOL\n");

  return ep;
}

edge_t *
triangulation_alloc_edge(triangulation_t *t) {
  edge_pool_t *ep = t->edge_pools;
  
  if(ep->edges_n >= EDGE_POOL_MALLOC_INC) {
    t->edge_pools = alloc_edge_pool(t); 
    t->edge_pools->next = ep;
    ep = t->edge_pools;
  }
  
  t->edges_n++;

  return (ep->edges + ep->edges_n++);
}

void
destroy_edge_pool(edge_pool_t *ep) {
  free(ep->edges);
  if(ep->next != NULL) destroy_edge_pool(ep->next);
  free(ep);
}

triangle_pool_t *
alloc_triangle_pool(triangulation_t *t) {
  triangle_pool_t *tp = calloc(1, sizeof(triangle_pool_t));

  tp->triangles = calloc(POINT_POOL_MALLOC_INC, sizeof(triangle_t));
  tp->next = NULL;
  tp->triangles_n = 0;
  
//  printf("NEW TRIANGLE POOL\n");

  return tp;
}

triangle_t *
triangulation_alloc_triangle(triangulation_t *t) {
  triangle_pool_t *tp = t->triangle_pools;
  
  if(tp->triangles_n >= TRIANGLE_POOL_MALLOC_INC) {
    t->triangle_pools = alloc_triangle_pool(t); 
    t->triangle_pools->next = tp;
    tp = t->triangle_pools;
  }
  
  t->triangles_n++;

  return (tp->triangles + tp->triangles_n++);
}

void
destroy_triangle_pool(triangle_pool_t *tp) {
  free(tp->triangles);
  if(tp->next != NULL) destroy_triangle_pool(tp->next);
  free(tp);
}

triangle_t *
triangle_create(triangulation_t *par, point_t *p1, point_t *p2, point_t *p3,
                triangle_t *t1, triangle_t *t2, triangle_t *t3)
{
  triangle_t *rval = triangulation_alloc_triangle(par);

  rval->points[0] = p1; 
  rval->points[1] = p2; 
  rval->points[2] = p3;

  rval->children[0] = NULL; 
  rval->children[1] = NULL; 
  rval->children[2] = NULL;
  rval->children_n = 0;

  rval->adj[0] = t1;
  rval->adj[1] = t2; 
  rval->adj[2] = t3;

  return rval; 
}


edge_t *
triangulation_insert_edge(triangulation_t *t, point_t *p1, point_t *p2)
{
  TAR_EDGES_LOOP(t);
  {
    if( (edge->p1 == p1 && edge->p2 == p2) || (edge->p2 == p1 && edge->p1 == p2) ) {
      printf("triangulation_insert_edge: Attempt to insert duplicate edge\n");
      return NULL;   
    }
  }
  TAR_EDGES_ENDLOOP;

  edge_t *e = triangulation_alloc_edge(t); 

  e->p1 = p1; e->p2 = p2;
  e->length = length( p1->x, p1->y, p2->x, p2->y );
//  e->age = t->points_n-1;
  e->flags = 0;
//  edge_sift_up(t, t->edges_n-1);

  return e;
}


void
triangulation_reset(triangulation_t *t) 
{
  triangulation_destroy(t);
  triangulation_create(t->max_coord, t);
}

void 
triangulation_destroy(triangulation_t *t) 
{
  destroy_edge_pool(t->edge_pools);
  destroy_triangle_pool(t->triangle_pools);

/*
  TAR_POINTS_LOOP(t);
  {
    //free(point->edges);
  }
  TAR_END_LOOP;
*/
  free(t->points);
  t->points_n = 0; t->points_max = 0;

  if(t->mst_edges != NULL) {
    free(t->mst_edges);
    t->mst_edges = NULL;
  }
  if(t->sorted_edges != NULL) {
    free(t->sorted_edges);
    t->sorted_edges = NULL;
  }

  //free(t);
}
/* wind:
 * returns 1,0,-1 for counterclockwise, collinear or clockwise, respectively.
 */
int 
wind(point_t *p1, point_t *p2, point_t *p3) {
  float rval,dx1, dx2, dy1, dy2;
  dx1 = p2->x - p1->x; dy1 = p2->y - p1->y;
  dx2 = p3->x - p2->x; dy2 = p3->y - p2->y;
  rval = (dx1*dy2)-(dy1*dx2);
  return (rval > 0.0001) ? 1 : ((rval < -0.0001) ? -1 : 0);
}

/*
 * same as wind..
 */
inline int
isleft(point_t *p0, point_t *p1, point_t *p2)
{
    return ( (p1->x - p0->x) * (p2->y - p0->y) - (p2->x - p0->x) * (p1->y - p0->y) );
}

inline double
area2D_triangle(point_t *v0, point_t *v1, point_t *v2 )
{
  double a = length(v0->x, v0->y, v1->x, v1->y);
  double b = length(v1->x, v1->y, v2->x, v2->y);
  double c = length(v2->x, v2->y, v0->x, v0->y);

  double s = (a + b + c) / 2.0f;
  
 
  double area = sqrt( s * (s-a) * (s-b) * (s-c) );

  return area;
//    return ( abs( (double)isleft(v0, v1, v2) ) / 2.0 );
}

/* on_linesegment:
 * returns 1 if point c is on the line segment defined by points a & b.
 */
int
on_linesegment(point_t *a, point_t *b, point_t *c) {

  /* collinear check */
  if(wind(a,b,c) != 0) return 0;

  if(c->x >= MIN(a->x,b->x) && c->x <= MAX(a->x,b->x))
    if(c->y >= MIN(a->y,b->y) && c->y <= MAX(a->y,b->y))
      return 1; 
  return 0;
}

int
root_point(triangulation_t *par, point_t *p) {
  int i;
  for(i=0;i<3;i++) {
    if(&par->spoints[i] == p) return i-3; 
  }
  return 1;
}

int 
root_edge(triangulation_t *par, edge_t *e) {
	return (root_point(par, e->p1)<0 || root_point(par, e->p2)); 
}

int
root_points(triangulation_t *par, triangle_t *t) {
  int rval=0;
  int i;
  for(i=0;i<3;i++) {
    rval += (root_point(par, t->points[i])<0)?1:0;
  }
  return rval;
}

/* determinant:
 * returns the determinant of the 3x3 matrix m.
 */
double
determinant(double m[3][3]) {
  double aei = m[0][0] * m[1][1] * m[2][2];
  aei += m[1][0] * m[2][1] * m[0][2];
  aei += m[2][0] * m[0][1] * m[1][2];

  double ceg = m[2][0] * m[1][1] * m[0][2];
  ceg += m[0][0] * m[2][1] * m[1][2];
  ceg += m[1][0] * m[0][1] * m[2][2];
  return aei-ceg; 
}


vector_t *create_vector(double i, double j, double k) {
  vector_t *r = calloc(1, sizeof(vector_t));
  r->i = i; r->j = j; r->k = k;
  return r;
}

double
dotproduct(vector_t *a, vector_t *b) {
  return (a->i * b->i)+(a->j * b->j)+(a->k * b->k);
}

void
crossproduct(vector_t *a, vector_t *b, vector_t *r) {
  r->k = (a->j * b->k) - (a->k * b->j);
  r->j = (a->k * b->i) - (a->i * b->k);
  r->i = (a->i * b->j) - (a->j * b->i);
}


int 
sameside(point_t *p1, point_t *p2, point_t *a, point_t *b) {
  vector_t *b_min_a = create_vector( b->x - a->x, b->y - a->y, 0.0f );
  vector_t *p1_min_a = create_vector( p1->x - a->x, p1->y - a->y, 0.0f );
  vector_t *p2_min_a = create_vector( p2->x - a->x, p2->y - a->y, 0.0f );
  vector_t *cp1 = calloc(1, sizeof(vector_t));
  vector_t *cp2 = calloc(1, sizeof(vector_t));
  crossproduct(b_min_a, p1_min_a, cp1);
  crossproduct(b_min_a, p2_min_a, cp2);
  int rval = dotproduct(cp1, cp2) >= 0;
  free(b_min_a); free(p1_min_a), free(p2_min_a); free(cp1); free(cp2);
  return rval;
}

int
point_inside_triangle(triangulation_t *par, triangle_t *t, point_t *p) {
  return 
    sameside(p,t->points[0],t->points[1],t->points[2]) && 
    sameside(p,t->points[1],t->points[0],t->points[2]) &&
    sameside(p,t->points[2],t->points[0],t->points[1]);
}


void
circumcentre(triangulation_t *par, triangle_t *t, double *x, double *y, double *r) {

  point_t *p0 = t->points[0];
  point_t *p1 = t->points[1];
  point_t *p2 = t->points[2];

  double bxm[3][3] = {
    { pow(p0->x,2) + pow(p0->y,2), p0->y, 1},
    { pow(p1->x,2) + pow(p1->y,2), p1->y, 1},
    { pow(p2->x,2) + pow(p2->y,2), p2->y, 1} };

  double bym[3][3] = {
    { pow(p0->x,2) + pow(p0->y,2), p0->x, 1},
    { pow(p1->x,2) + pow(p1->y,2), p1->x, 1},
    { pow(p2->x,2) + pow(p2->y,2), p2->x, 1} };

  double am[3][3] = {
    { p0->x, p0->y, 1},
    { p1->x, p1->y, 1},
    { p2->x, p2->y, 1} };

  double cm[3][3] = {
    { pow(p0->x,2) + pow(p0->y,2), p0->x, p0->y},
    { pow(p1->x,2) + pow(p1->y,2), p1->x, p1->y},
    { pow(p2->x,2) + pow(p2->y,2), p2->x, p2->y} };

  double bx = -determinant(bxm);
  double by = determinant(bym);
  double a = determinant(am);
  double c = -determinant(cm);

  *x = -0.5 * bx / a;
  *y = -0.5 * by / a;
  *r =  sqrt(pow(bx,2) + pow(by,2) - (4*a*c))/(2*fabs(a));

}



/* point_inside_circumcircle:
 * returns 1 if the point p lies inside the circumcircle of triangle t.
 */
int
point_inside_circumcircle(triangulation_t *par, triangle_t *t, point_t *p) {
  point_t *p0 = t->points[0];
  point_t *p1 = t->points[1];
  point_t *p2 = t->points[2];

  int c = wind(p0,p1,p2); 
  
  double m[3][3];

  m[0][0] = p0->x - p->x; 
  m[0][1] = p0->y - p->y; 
  m[0][2] = pow(m[0][0],2) + pow(m[0][1],2);

  if(c==1) {
    m[1][0] = p1->x - p->x; 
    m[1][1] = p1->y - p->y; 
  }else{ 
    m[1][0] = p2->x - p->x; 
    m[1][1] = p2->y - p->y; 
  }

  m[1][2] = pow(m[1][0],2) + pow(m[1][1],2);

  if(c==1) {
    m[2][0] = p2->x - p->x; 
    m[2][1] = p2->y - p->y; 
  }else { 
    m[2][0] = p1->x - p->x; 
    m[2][1] = p1->y - p->y; 
  }

  m[2][2] = pow(m[2][0],2) + pow(m[2][1],2);
  return (determinant(m)>0);
}

bool
contains_edge(triangle_t *t, edge_t *e) {
  int i;
  for(i=0;i<3;i++) {
    if(t->points[i] == e->p1 && t->points[i%3] == e->p2) return true; 
    if(t->points[i] == e->p2 && t->points[i%3] == e->p1) return true; 
  }
  return false;
}

bool
contains_points(triangle_t *t, point_t *p1, point_t *p2) {
  if(t == NULL) return false;
  if(t->points[0] == p1 && t->points[1] == p2) return true; 
  if(t->points[0] == p2 && t->points[1] == p1) return true; 
  if(t->points[1] == p1 && t->points[2] == p2) return true; 
  if(t->points[1] == p2 && t->points[2] == p1) return true; 
  if(t->points[2] == p1 && t->points[0] == p2) return true; 
  if(t->points[2] == p2 && t->points[0] == p1) return true;
  return false;
}

point_t *
other_point(triangle_t *t, point_t *p1, point_t *p2) {
  int i;
  for(i=0;i<3;i++) {
    if(t->points[i] == p1 && t->points[(i+1)%3] == p2) return t->points[(i+2)%3]; 
    if(t->points[i] == p2 && t->points[(i+1)%3] == p1) return t->points[(i+2)%3]; 
  }
//  printf("other_point: couldn't find other point\n");
  return NULL;
}

/* oppisate_point:
 * returns the far away point of the triangle adjacent to t on edge e
 */
point_t *
oppisate_point_from_edge(triangulation_t *par, triangle_t *t, edge_t *e) {
  if(t == NULL) {
    printf("oppisate_point: NULL triangle\n");
    return NULL;
  }

  int i;
  for(i=0;i<3;i++) {
    if(contains_edge(t->adj[i], e)) return other_point(t->adj[i], e->p1, e->p2);
  }
  
//  printf("oppisate_point: couldn't find point (probably special point)\n");
  return NULL;
}

point_t *
oppisate_point_from_points(triangulation_t *par, triangle_t *t, point_t *p1, point_t *p2) {
  if(t == NULL) {
    printf("oppisate_point: NULL triangle\n");
    return NULL;
  }

  int i;
  for(i=0;i<3;i++) {
    if(contains_points(t->adj[i], p1, p2)) return other_point(t->adj[i], p1, p2);
  }
  
//  printf("oppisate_point: couldn't find point (probably special point)\n");
  return NULL;
}

triangle_t *
oppisate_triangle_from_edge(triangulation_t *par, triangle_t *t, edge_t *e) {
  if(t == NULL) {
    printf("oppisate_triangle: NULL triangle\n");
    return NULL;
  }

  int i;
  for(i=0;i<3;i++) {
    if(contains_edge(t->adj[i], e)) return t->adj[i];
  }
  
//  printf("oppisate_triangle: couldn't find triangle (probably special point)\n");
  return NULL;
}

triangle_t *
oppisate_triangle_from_points(triangulation_t *par, triangle_t *t, point_t *p1, point_t *p2) {
  if(t == NULL) {
    printf("oppisate_triangle: NULL triangle\n");
    return NULL;
  }

  int i;
  for(i=0;i<3;i++) {
    if(contains_points(t->adj[i], p1, p2)) return t->adj[i];
  }
  
//  printf("oppisate_triangle: couldn't find triangle (probably special point)\n");
  return NULL;
}

/* is_illegal:
 * returns non-zero if the far point from edge e lies in the circumcircle of triangle t
 */
bool
is_illegal(triangulation_t *par, triangle_t *t, point_t *p1, point_t *p2) {
  if(t == NULL) {
    printf("is_illegal: NULL triangle\n");
    return NULL;
  }
  
  point_t *p = oppisate_point_from_points(par, t, p1, p2);

  if(p == NULL) return false;

  /* special point counter */
  int c = 0;

  int i = root_point(par, p1); //TRI(par,t).points[e]-3;
  int j = root_point(par, p2); //TRI(par,t).points[(e+1)%3]-3;
  int k = root_point(par, other_point(t, p1, p2)); // TRI(par,t).points[(e+2)%3]-3;
  int l = root_point(par, p); 
  if(i<0) c++;
  if(j<0) c++;
  if(k<0) c++;
  if(l<0) c++;

  if(i<0 && j<0) {
    return false;
  }else if(i>=0 && j>=0 && k>=0 && l>=0) {
    return point_inside_circumcircle(par, t, p);
  }else if(c==1) {
    if(i<0 || j<0) return point_inside_circumcircle(par, t, p);//1;
    return false;
  }else if(c==2) {
    int n = (i<j) ? i : j;
    int m = (k<l) ? k : l;
    if(n>m) return false;
    return point_inside_circumcircle(par, t, p);
  }

  printf("is_illegal: three negative points occurred");
  return false;
}
/*
triangle_index
get_adj(triangulation_t *par, triangle_index t, point_index p1, point_index p2) {

  if(TRI(par,t).points[0] == p1) {
    if(TRI(par,t).points[1] == p2) return TRI(par,t).adj[0];
    if(TRI(par,t).points[2] == p2) return TRI(par,t).adj[2];
  }else if(TRI(par,t).points[1] == p1) {
    if(TRI(par,t).points[0] == p2) return TRI(par,t).adj[0];
    if(TRI(par,t).points[2] == p2) return TRI(par,t).adj[1];
  }else if(TRI(par,t).points[2] == p1) {
    if(TRI(par,t).points[0] == p2) return TRI(par,t).adj[2];
    if(TRI(par,t).points[1] == p2) return TRI(par,t).adj[1];
  }
  log_debug("get_adj","can't find point for triangle in gen %d\n", TRI(par,t).age);
  return NULL_TRI;
}
*/
void
update_adj_link(triangulation_t *par, triangle_t *t, triangle_t *old, triangle_t *new) {
  if(t == NULL) return;
  int i;
  for(i=0;i<3;i++) {
    if(t->adj[i] != NULL) {
      if(t->adj[i] == old) {
        t->adj[i] = new;
        return;
      }
    }
  }
}

/* legalize_edge:
 * if edge is illegal, flip edge & legalize ik and kj
 */
void
legalize_edge(triangulation_t *par, point_t *p, triangle_t *t, point_t *i, point_t *j) {
  triangle_t *n = oppisate_triangle_from_points(par, t, i, j);

//  point_t *i = e->p1;
//  point_t *j = e->p2;
  point_t *k = oppisate_point_from_points(par, t, i, j);

  if(is_illegal(par, t, i, j)) {
    
    triangle_t *t1 = triangle_create(par, p, i, k, NULL, NULL, NULL);
    triangle_t *t2 = triangle_create(par, p, j, k, NULL, NULL, NULL);

    n->children[0] = t->children[0] = t1;
    n->children[1] = t->children[1] = t2;
    n->children_n = t->children_n = 2;

    t1->adj[0] = oppisate_triangle_from_points(par,t,p,i);
    t1->adj[1] = oppisate_triangle_from_points(par,n,i,k);

    update_adj_link(par, t1->adj[0], t, t1);
    update_adj_link(par, t1->adj[1], n, t1);

    t2->adj[0] = oppisate_triangle_from_points(par,t,p,j);
    t2->adj[1] = oppisate_triangle_from_points(par,n,j,k);

    update_adj_link(par, t2->adj[0], t, t2);
    update_adj_link(par, t2->adj[1], n, t2);

    t1->adj[2] = t2;
    t2->adj[2] = t1;

    /* update edge */
    TAR_EDGES_LOOP(par);
    {
      if((edge->p1 == i && edge->p2 == j) || (edge->p1 == j && edge->p2 == i)) {
        edge->p1 = p;
        edge->p2 = k;
        edge->length = length( p->x, p->y, k->x, k->y );

//        edge_sift_up(par, n);	
//        edge_sift_down(par, n);
        break;
      }
    }
    TAR_EDGES_ENDLOOP;

    legalize_edge(par, p, t1, i, k);
    legalize_edge(par, p, t2, j, k);
  }
}
/*
point_t * 
triangle_centre(triangulation_t *par, triangle_index t) {
  point_t h1;
  h1.x = (PNT(par,TRI(par,t).points[0])->x + PNT(par,TRI(par,t).points[1])->x)/2;
  h1.y = (PNT(par,TRI(par,t).points[0])->y + PNT(par,TRI(par,t).points[1])->y)/2;
  point_t *r = calloc(1, sizeof(point_t));
  r->x = (h1.x + PNT(par,TRI(par,t).points[2])->x)/2;
  r->y = (h1.y + PNT(par,TRI(par,t).points[2])->y)/2;
  return r;
}
*/
/* point_on_edge:
 * performs delaunay calculation for the case when a point in inserted
 * on an edge.
 *
 */
int
point_on_edge(triangulation_t *par, triangle_t *t, point_t *p) {
  point_t *p0 = t->points[0];
  point_t *p1 = t->points[1];
  point_t *p2 = t->points[2];


  triangle_t *nbr;

  /* i and j are the points on the line segment p intersects
   * with. 
   * k is the point on the triangle {i,j,k}, and l is the 
   * point in the triangle also containing points {i,k}.
   */
  point_t *i, *j, *k, *l;

  if(on_linesegment(p0, p1, p)) {
    i = p0; j = p1; k = p2;
    l = oppisate_point_from_points(par,t,p0,p1);
    nbr = oppisate_triangle_from_points(par,t,p0,p1);
  }else if(on_linesegment(p1, p2, p)) {
    i = p1; j = p2; k = p0;
    l = oppisate_point_from_points(par,t,p1,p2);
    nbr = oppisate_triangle_from_points(par,t,p1,p2);
  }else if(on_linesegment(p2, p0, p)) {    
    i = p2; j = p0; k = p1;
    l = oppisate_point_from_points(par,t,p2,p0);
    nbr = oppisate_triangle_from_points(par,t,p2,p0);
  }else return 0;
  
//  if(p->type == CORNER) printf("EDGE p=%d,%d\n", p->x, p->y);

  triangle_t *jk, *ik, *jl, *il;

  jk = oppisate_triangle_from_points(par, t, j, k); 
  ik = oppisate_triangle_from_points(par, t, i, k);
  jl = oppisate_triangle_from_points(par, nbr, j, l); 
  il = oppisate_triangle_from_points(par, nbr, i, l);

  t->children[0] = triangle_create(par, j, k, p, jk, NULL, NULL); 
  t->children[1] = triangle_create(par, i, k, p, ik, NULL, NULL); 
  nbr->children[0] = triangle_create(par, j, l, p, jl, NULL, NULL); 
  nbr->children[1] = triangle_create(par, i, l, p, il, NULL, NULL); 

  t->children[0]->adj[1] = t->children[1];
  t->children[1]->adj[1] = t->children[0];
  nbr->children[0]->adj[1] = nbr->children[1];
  nbr->children[1]->adj[1] = nbr->children[0];

  t->children[0]->adj[2] = nbr->children[0];
  t->children[1]->adj[2] = nbr->children[1];
  nbr->children[0]->adj[2] = t->children[0];
  nbr->children[1]->adj[2] = t->children[1];

  update_adj_link(par, jk, t, t->children[0]);
  update_adj_link(par, ik, t, t->children[1]);
  update_adj_link(par, jl, nbr, nbr->children[0]);
  update_adj_link(par, il, nbr, nbr->children[1]);

  t->children_n = 2;
  nbr->children_n = 2;

  if(k==NULL) printf("k == NULL\n");
  if(l==NULL) printf("l == NULL\n");

  triangulation_insert_edge(par, k, p); 
  triangulation_insert_edge(par, l, p); 


  TAR_EDGES_LOOP(par);
  {
    if( (edge->p1 == i && edge->p2 == j) ) {
      edge->p1 = p;
      edge->length = length( edge->p1->x, edge->p1->y, edge->p2->x, edge->p2->y );
      triangulation_insert_edge(par, i, p);
      break; 
    }

    if( (edge->p1 == j && edge->p2 == i) ) {
      edge->p1 = p;
      edge->length = length( edge->p1->x, edge->p1->y, edge->p2->x, edge->p2->y );
      triangulation_insert_edge(par, j, p);
    }
  }
  TAR_EDGES_ENDLOOP;

  legalize_edge(par, p, t->children[0], i, l);
  legalize_edge(par, p, t->children[1], l, j);
  legalize_edge(par, p, nbr->children[0], j, k);
  legalize_edge(par, p, nbr->children[1], k, i);

  return 1;
}


void
point_in_interior(triangulation_t *par, triangle_t *t, point_t *p) {
  
  triangle_t *t0 = oppisate_triangle_from_points(par, t, t->points[0], t->points[1]);
  triangle_t *t1 = oppisate_triangle_from_points(par, t, t->points[1], t->points[2]);
  triangle_t *t2 = oppisate_triangle_from_points(par, t, t->points[2], t->points[0]);

  t->children[0] = triangle_create(par, t->points[0], t->points[1], p, t0, NULL, NULL);
  t->children[1] = triangle_create(par, t->points[1], t->points[2], p, t1, NULL, NULL);
  t->children[2] = triangle_create(par, t->points[2], t->points[0], p, t2, NULL, NULL);
  
  t->children[0]->adj[1] = t->children[1]; 	
  t->children[0]->adj[2] = t->children[2]; 	

  t->children[1]->adj[1] = t->children[2]; 	
  t->children[1]->adj[2] = t->children[0]; 	

  t->children[2]->adj[1] = t->children[0]; 	
  t->children[2]->adj[2] = t->children[1]; 	

  t->children_n = 3;

  update_adj_link( par, t0, t, t->children[0] );
  update_adj_link( par, t1, t, t->children[1] );
  update_adj_link( par, t2, t, t->children[2] );

  triangulation_insert_edge(par, t->points[0], p); 
  triangulation_insert_edge(par, t->points[1], p); 
  triangulation_insert_edge(par, t->points[2], p); 

  legalize_edge(par, p, t->children[0], t->points[0], t->points[1]);
  legalize_edge(par, p, t->children[1], t->points[1], t->points[2]);
  legalize_edge(par, p, t->children[2], t->points[2], t->points[0]);
}

point_t **
triangulation_alloc_point(triangulation_t *par) {
  point_t **p = par->points;

  if(par->points_n >= par->points_max) {
    par->points_max += MALLOC_INC;
    p = realloc(p, par->points_max * sizeof(point_t *));
    if(p == NULL) printf("triangulation_alloc_point: realloc failure\n");
    par->points = p;
    memset(p + par->points_n, 0, MALLOC_INC * sizeof(point_t *));
  }

  return (p + par->points_n++);
}

int
triangle_insert_point(triangulation_t *par, triangle_t *t, point_t *p) {
//  printf("triangle_insert_point %d,%d\n", p->x, p->y);
  if(!point_inside_triangle(par,t,p)) {
    return 0;
  }

  if(t->children_n > 0) {
    int i;
    for(i=0;i<t->children_n;i++) {
      if(triangle_insert_point(par, t->children[i], p )) return 1;
    }

  }else if(point_on_edge(par, t, p)) {
  }else{
    point_in_interior(par, t, p);
  }
  return 1;	
}

int
triangulation_insert_point(triangulation_t *par, point_t *p) {
  point_t **pp = triangulation_alloc_point(par);
  *pp = p;
  return triangle_insert_point(par, par->root_triangle, p);
}

void
triangulation_create(int max_coord, triangulation_t *par) {

#ifdef DEBUG
  par->debug = false;
#endif

  par->triangle_pools = alloc_triangle_pool(par);
  
  par->edge_pools = alloc_edge_pool(par);
  
  par->points = calloc(MALLOC_INC, sizeof(point_t *));
  par->points_n = 0; par->points_max = MALLOC_INC; 

  par->mst_edges = NULL;
  par->sorted_edges = NULL;
  par->max_coord = max_coord;

  int m = max_coord * 3;

  par->spoints[0].x = m;  par->spoints[0].y = 0;
  par->spoints[1].x = 0;  par->spoints[1].y = m;
  par->spoints[2].x = -m; par->spoints[2].y = -m;

  par->root_triangle = 
    triangle_create(par, &par->spoints[2], &par->spoints[1], &par->spoints[0], NULL, NULL, NULL);

  triangulation_insert_edge(par, &par->spoints[1], &par->spoints[0]); 
  triangulation_insert_edge(par, &par->spoints[1], &par->spoints[2]); 
  triangulation_insert_edge(par, &par->spoints[2], &par->spoints[0]); 
}
