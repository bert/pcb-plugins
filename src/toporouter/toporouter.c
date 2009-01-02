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
 * This file implements a topological autorouter, and uses techniques from the
 * following publications:
 *
 * Dayan, T. and Dai, W.W.M., "Layer Assignment for a Rubber Band Router" Tech
 * Report UCSC-CRL-92-50, Univ. of California, Santa Cruz, 1992.
 *
 * Dai, W.W.M and Dayan, T. and Staepelaere, D., "Topological Routing in SURF:
 * Generating a Rubber-Band Sketch" Proc. 28th ACM/IEEE Design Automation
 * Conference, 1991, pp. 39-44.
 *
 * David Staepelaere, Jeffrey Jue, Tal Dayan, Wayne Wei-Ming Dai, "SURF:
 * Rubber-Band Routing System for Multichip Modules," IEEE Design and Test of
 * Computers ,vol. 10, no. 4,  pp. 18-26, October/December, 1993.
 *
 * Dayan, T., "Rubber-band based topological router" PhD Thesis, Univ. of
 * California, Santa Cruz, 1997.
 *
 * David Staepelaere, "Geometric transformations for a rubber-band sketch"
 * Master's thesis, Univ. of California, Santa Cruz, September 1992.
 *
 */


#include "toporouter.h"

#ifdef TRACE
//  #define TRACE_REPEATS 1
//  #define TRACE_CAPTIONS 1
#endif 

void
problem_destroy_ts(problem_t *prob) {
  if(prob->ts != NULL) {
    int i;
    for(i=0;i<prob->ts_n;i++) {
      triangulation_destroy( &prob->ts[i] ); 
    }
    free(prob->ts);
    prob->ts = NULL;
  }
}

edge_t **
problem_alloc_edge_ptr(problem_t *prob) {
  edge_t **ep = prob->edges;

  if(prob->edges_n >= prob->edges_max) {
    prob->edges_max += MALLOC_INC;
    ep = realloc(ep, prob->edges_max * sizeof(edge_t *));
    if(ep == NULL) printf("problem_alloc_edge_ptr: realloc failure\n");
    prob->edges = ep;
    memset(ep + prob->edges_n, 0, MALLOC_INC * sizeof(edge_t *));
  }

  return (ep + prob->edges_n++);
}

edge_pool_t *
problem_alloc_edge_pool(problem_t *prob) {
  edge_pool_t *ep = calloc(1, sizeof(edge_pool_t));

  ep->edges = calloc(EDGE_POOL_MALLOC_INC, sizeof(edge_t));
  ep->next = NULL;
  ep->edges_n = 0;

//  printf("NEW EDGE POOL\n");

  return ep;
}

edge_t *
problem_alloc_edge(problem_t *prob) {
  edge_pool_t *ep = prob->edge_pools;
  
  if(ep->edges_n >= EDGE_POOL_MALLOC_INC) {
    prob->edge_pools = problem_alloc_edge_pool(prob); 
    prob->edge_pools->next = ep;
    ep = prob->edge_pools;
  }
  
  prob->pool_edges_n++;
    
  edge_t **epp = problem_alloc_edge_ptr(prob);
  *epp =  (ep->edges + ep->edges_n++);

	epp[0]->segments = NULL;
	epp[0]->tcs_points = NULL;

  return epp[0];
}

void
problem_destroy_edge_pool(edge_pool_t *ep) {
  free(ep->edges);
  if(ep->next != NULL) problem_destroy_edge_pool(ep->next);
  free(ep);
}

/*
 * A generic slab allocator pool
 */
generic_pool_t *
alloc_generic_pool(int datasize) {
  generic_pool_t *gp = calloc(1, sizeof(generic_pool_t));

  gp->data = calloc(MALLOC_INC, datasize);
  gp->next = NULL;
  gp->data_n = 0;

  return gp;
}

/*
 * Destroys a generic slab allocator pool
 */
void
destroy_generic_pool(generic_pool_t *gp) {
  free(gp->data);
  if(gp->next != NULL) destroy_generic_pool(gp->next);
  free(gp);
}

/*
 * Allocates a generic object from the generic slab allocators
 */
void *
alloc_generic(generic_pool_t **gpool, int *datacount, int datasize) {
  generic_pool_t *gp = *gpool;
  
  if(gp->data_n >= MALLOC_INC) {
    *gpool = alloc_generic_pool(datasize);
    (*gpool)->next = gp;
    gp = *gpool;
  }
  
  datacount ++;

  return (gp->data + (gp->data_n++ * datasize) );
}

/*
 * Allocates an edge from a solution
 *
inline edge_t *
solution_alloc_edge(solution_t *soln) {
  return ((edge_t *) alloc_generic(&soln->edge_pools, &(soln->pool_edges_n), sizeof(edge_t)));
}
*/

void
netlist_destroy(netlist_t *nl) {
  if(nl->triangulation != NULL) {
    triangulation_destroy(nl->triangulation);
    free(nl->triangulation);
  }
}
/*
void
destroy_ptr_edge(edge_t *e) {
  //collison_t *col = e->cols;

  while(col != NULL) {
    collison_t *temp = col->next;
    free(col);
    col = temp;
  }
}
*/
void
destroy_problem(autorouter_t *ar, problem_t *prob) {
  problem_destroy_ts(prob);
  
  problem_destroy_edge_pool(prob->edge_pools);

  if(prob->edges != NULL) free(prob->edges);

  if(prob->points != NULL) free(prob->points);
//  if(prob->cut_crossing_netlists != NULL) free(prob->cut_crossing_netlists);
  if(prob->cut_crossing_netlist_slots != NULL) free(prob->cut_crossing_netlist_slots);
  if(prob->cut_crossing_netlist_layers != NULL) free(prob->cut_crossing_netlist_layers);
}


point_t **
problem_alloc_point(problem_t *prob) {
  point_t **p = prob->points;

  if(prob->points_n >= prob->points_max) {
    prob->points_max += MALLOC_INC;
    p = realloc(p, prob->points_max * sizeof(point_t *));
    if(p == NULL) printf("problem_alloc_point: realloc failure\n");
    prob->points = p;
    memset(p + prob->points_n, 0, MALLOC_INC * sizeof(point_t *));
  }

  return (p + prob->points_n++);
}

void
destroy_point_pool(point_pool_t *pp) {
  free(pp->points);
  if(pp->next != NULL) destroy_point_pool(pp->next);
  free(pp);
}

point_pool_t *
alloc_point_pool(autorouter_t *ar) {
  point_pool_t *pp = calloc(1, sizeof(point_pool_t));

  pp->points = calloc(POINT_POOL_MALLOC_INC, sizeof(point_t));
  pp->next = NULL;
  pp->points_n = 0;

  return pp;
}

point_t *
alloc_point(autorouter_t *ar) {
  point_pool_t *pp = ar->point_pools;
  
  if(pp->points_n >= POINT_POOL_MALLOC_INC) {
    ar->point_pools = alloc_point_pool(ar); 
    ar->point_pools->next = pp;
    pp = ar->point_pools;
  }
  
  ar->points_n++;
  
  pp->points[pp->points_n].pull_point = NULL;

  return (pp->points + pp->points_n++);
}

problem_pool_t *
autorouter_alloc_problem_pool(autorouter_t *ar) {
  problem_pool_t *pp = calloc(1, sizeof(problem_pool_t));

  pp->problems = calloc(PROBLEM_POOL_MALLOC_INC, sizeof(problem_t));
  pp->next = NULL;
  pp->problems_n = 0;

  return pp;
}

void
autorouter_destroy_problem_pool(autorouter_t *ar, problem_pool_t *pp) {
  int i; 
  for(i=0;i<pp->problems_n;i++) 
    destroy_problem( ar, &pp->problems[i] );
  
  free(pp->problems);
  if(pp->next != NULL) autorouter_destroy_problem_pool(ar,pp->next);
  free(pp);
}

problem_t *
autorouter_alloc_problem(autorouter_t *ar) {
  problem_pool_t *pp = ar->problem_pools;
  
  if(pp->problems_n >= PROBLEM_POOL_MALLOC_INC) {
    ar->problem_pools = autorouter_alloc_problem_pool(ar); 
    ar->problem_pools->next = pp;
    pp = ar->problem_pools;
  }
  
  ar->problems_n++;

  problem_t *rval = (pp->problems + pp->problems_n++);
  
  rval->points_n = 0; rval->points_max = MALLOC_INC;
  rval->points = calloc(MALLOC_INC, sizeof(point_t*));

  rval->left = NULL;
  rval->right = NULL;

  rval->edges_n = 0; rval->edges_max = MALLOC_INC;
  rval->edges = calloc(MALLOC_INC, sizeof(edge_t *));

  rval->x1 = 0; 
  rval->y1 = 0; 
  rval->x2 = ar->board_width; 
  rval->y2 = ar->board_height; 
  rval->width  = ar->board_width; 
  rval->height = ar->board_height; 
  rval->cut_orientation = CUT_NONE;

  rval->cut_crossing_netlists = NULL;
  rval->cut_crossing_netlist_slots = NULL;
  rval->cut_crossing_netlist_layers = NULL;

  rval->edge_pools = problem_alloc_edge_pool(rval);
  rval->pool_edges_n = 0;

  return rval;
}

void
autorouter_destroy(autorouter_t *ar) {
  destroy_point_pool(ar->point_pools);
  autorouter_destroy_problem_pool(ar, ar->problem_pools);
  
  free(ar->netlists);

  free(ar->trace_prefix);
  free(ar->routing_direction);
  free(ar);
}

#ifdef TRACE

double layer_colors[8][4] = {
  { 0.2, 0.2, 1.0, 0.6},
  { 0.2, 1.0, 0.2, 0.6},
  { 1.0, 0.2, 0.2, 0.6},
  { 1.0, 0.2, 1.0, 0.6},

  { 0.2, 1.0, 1.0, 0.6},
  { 1.0, 1.0, 0.2, 0.6},
  { 1.0, 0.5, 0.0, 0.6},
  { 0.0, 0.5, 1.0, 0.6}
};


#define PROB_MAXDIM ((prob->width > prob->height) ? prob->width : prob->height)
#define SCALEX(x) (((double)(x-c->x_offset)/(double)c->w) * c->resx)
#define SCALEY(x) (((double)(x-c->y_offset)/(double)c->h) * c->resy)

void
init_drawing(drawing_context_t *c) {

  c->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, c->resx, c->resy);
  c->cr = cairo_create (c->surface);

  cairo_rectangle (c->cr, 0, 0, c->resx, c->resy);
  cairo_set_source_rgb (c->cr, 0, 0, 0);
  cairo_fill (c->cr);

}

void
finish_drawing(autorouter_t *ar, drawing_context_t *c) {
  char *filename = calloc(256, sizeof(char));
  sprintf(filename, "trace/%05d%s.png", ar->traceseq++, c->prefix);
  cairo_surface_write_to_png (c->surface, filename);
  free(filename);
  cairo_destroy (c->cr);
  cairo_surface_destroy (c->surface);
  free(c);

  ar->last_prefix = c->prefix;
}

drawing_context_t *
create_problem_drawing_context(autorouter_t *ar, problem_t *prob) {
  drawing_context_t *rval = calloc(1, sizeof(drawing_context_t));
  
  int nx = prob->width / 10;
  int ny = prob->height / 10;
  
  rval->x_offset = prob->x1 - nx;
  rval->y_offset = prob->y1 - ny;
  rval->w = prob->width + (2 * nx);
  rval->h = prob->height + (2 * ny);



  rval->resx = 1024;
  rval->resy = 768;
  rval->prefix = "prob\0";

  init_drawing(rval);  

  return rval;
}

drawing_context_t *
create_autorouter_drawing_context(autorouter_t *ar) {
  drawing_context_t *rval = calloc(1, sizeof(drawing_context_t));

  rval->x_offset = 0;
  rval->y_offset = 0;
  rval->w = ar->board_width;
  rval->h = ar->board_height;

  rval->resx = 1024;
  rval->resy = 768;
  rval->prefix = "ar\0";

  init_drawing(rval);

  return rval;
}

void
draw_triags(autorouter_t *ar, drawing_context_t *c, problem_t *prob, int layer) {
  if(prob->ts == NULL) return;  

  int i;
  for(i=0;i<prob->ts_n;i++) {
    
    if(layer >= 0 && i != layer) continue;
    
    TAR_EDGES_LOOP( &prob->ts[i] );
    {
      if(edge->flags & EDGEFLAG_INACTIVE) continue;
      if(root_point(&prob->ts[i], edge->p1)<0 || root_point(&prob->ts[i],edge->p2) < 0 ) continue;
      
      if(edge->flags & EDGEFLAG_HILIGHT) 
        cairo_set_source_rgba (c->cr, 1, 0, 0, 1);
      else
        cairo_set_source_rgba (c->cr, 1, 1, 1, 0.3);

      cairo_move_to( c->cr, SCALEX(edge->p1->x), SCALEY(edge->p1->y) );
      cairo_line_to( c->cr, SCALEX(edge->p2->x), SCALEY(edge->p2->y) );
      cairo_stroke( c->cr );
    }
    TAR_EDGES_ENDLOOP;
    
  }

}

void
draw_triag(autorouter_t *ar, drawing_context_t *c, triangulation_t *t) {

  TAR_EDGES_LOOP( t );
  {
    if(edge->flags & EDGEFLAG_INACTIVE) continue;
    if(root_point( t, edge->p1)<0 || root_point(t,edge->p2) < 0 ) continue;
    cairo_set_source_rgba (c->cr, 1, 1, 1, 0.3);
    cairo_move_to( c->cr, SCALEX(edge->p1->x), SCALEY(edge->p1->y) );
    cairo_line_to( c->cr, SCALEX(edge->p2->x), SCALEY(edge->p2->y) );
    cairo_stroke( c->cr );
  }
  TAR_EDGES_ENDLOOP;

}


void
draw_msts(autorouter_t *ar, drawing_context_t *c, problem_t *prob, int layer) {
  if(prob->ts == NULL) return;  
  triangulation_t *netlist_ts = prob->ts; 
  int i;
  for(i=0;i<prob->ts_n;i++) {

    if(layer != -1 && i != layer) continue;

    TAR_MST_EDGES_LOOP( &netlist_ts[i] );
    {
      if(root_point(&netlist_ts[i], edge->p1)<0 || root_point(&netlist_ts[i],edge->p2) < 0 ) continue;
      
      cairo_set_source_rgba (c->cr, 1, 0, 0, 0.6);
      cairo_move_to( c->cr, SCALEX(edge->p1->x), SCALEY(edge->p1->y) );
      cairo_line_to( c->cr, SCALEX(edge->p2->x), SCALEY(edge->p2->y) );
      cairo_stroke( c->cr );

    }
    TAR_END_LOOP;
  }


}

void
draw_points(autorouter_t *ar, drawing_context_t *c, problem_t *prob, int layer) {

  double radius = 3.0f;


  int i;
  for(i=0;i<prob->points_n;i++) {
    point_t *point = prob->points[i];

    if(layer != -1 && point->layer != layer && 
       point->type != PIN && point->type != VIA && point->type != CORNER) continue;

    if(point->type == PIN) radius = 3.0f;
    else radius = 2.0f;

    if(point->type == VIA || point->type == PIN) {
      cairo_set_source_rgba(c->cr, 1, 0.2, 1, 0.8);
    }else if(point->type == CROSSPOINT) {
      if(point->layer != -1) {
        continue;
      }else 
        cairo_set_source_rgba(c->cr, 0.2, 1, 1, 0.8);
    }else if(point->layer == -1) {
      cairo_set_source_rgba(c->cr, 1, 0, 0, 0.8);
    }else {
      cairo_set_source_rgba(c->cr, layer_colors[point->layer][0],
          layer_colors[point->layer][1],
          layer_colors[point->layer][2],
          layer_colors[point->layer][3] );
    }

    cairo_arc(c->cr, SCALEX(point->x), SCALEY(point->y), radius, 0, 2 * M_PI);
    cairo_fill(c->cr);

  }

}



void
draw_problem(autorouter_t *ar, drawing_context_t *c, problem_t *prob, int flags, int layer) {
  edge_t **edges = prob->edges;
  int edges_n = prob->edges_n;

  if(flags & DRAW_POINTS) {
    draw_points(ar, c, prob, layer);
  }

  if(flags & DRAW_MSTS) {
    draw_msts(ar, c, prob, layer);
  }

  if(flags & DRAW_TRIAGS) {
    //if(flags & DRAW_PROB) 
    draw_triags(ar, c, prob, layer);
  }

  if(flags & DRAW_CLOSELIST) {
    cairo_set_source_rgba(c->cr, 1.0f, 0.0f, 0.0f, 0.75f);
    linklist_t *curll = prob->closelist;

    while(curll != NULL) {
      point_t *p = (point_t *)curll->data;
      
      cairo_arc(c->cr, SCALEX(p->x), SCALEY(p->y), 4.0f, 0, 2 * M_PI);
      cairo_fill(c->cr);

      curll = curll->next;
    }
    
    cairo_set_source_rgba(c->cr, 0.0f, 1.0f, 0.0f, 0.75f);
    curll = prob->openlist;

    while(curll != NULL) {
      point_t *p = (point_t *)curll->data;
      
      cairo_arc(c->cr, SCALEX(p->x), SCALEY(p->y), 4.0f, 0, 2 * M_PI);
      cairo_fill(c->cr);

      curll = curll->next;
    }

  }

  if(flags & DRAW_SOLN_EDGES) {
    
    int j;
    for(j=((layer<0)?0:layer);j<((layer<0)?ar->board_layers:layer+1);j++) {

      solution_t *soln = &prob->solns[j];


      generic_pool_t *curpool = soln->edge_pools;

      while(curpool != NULL) {
        int i;
        for(i=0;i<curpool->data_n;i++) {
          edge_t *edge =  &((edge_t *) curpool->data)[i];

          if(!(edge->flags & EDGEFLAG_INACTIVE)) {

            cairo_set_source_rgba(c->cr, 
                layer_colors[j][0],
                layer_colors[j][1],
                layer_colors[j][2],
                layer_colors[j][3]);
            cairo_move_to( c->cr, SCALEX(edge->p1->x), SCALEY(edge->p1->y) );
            cairo_line_to( c->cr, SCALEX(edge->p2->x), SCALEY(edge->p2->y) );
            cairo_stroke(c->cr);


            if(edge->p1->type == TCSPOINT && edge->p1->pull_point != NULL) {
              cairo_set_source_rgba(c->cr, 0.0, 1.0, 1.0, 0.25);
              cairo_move_to( c->cr, SCALEX(edge->p1->x), SCALEY(edge->p1->y) );
              cairo_line_to( c->cr, SCALEX(edge->p1->pull_point->x), SCALEY(edge->p1->pull_point->y) );
              cairo_stroke(c->cr);
            }

            if(edge->p2->type == TCSPOINT && edge->p2->pull_point != NULL) {
              cairo_set_source_rgba(c->cr, 0.0, 1.0, 1.0, 0.25);
              cairo_move_to( c->cr, SCALEX(edge->p2->x), SCALEY(edge->p2->y) );
              cairo_line_to( c->cr, SCALEX(edge->p2->pull_point->x), SCALEY(edge->p2->pull_point->y) );
              cairo_stroke(c->cr);
            }

          }
        }

        curpool = curpool->next;
      }

    }



  }

  if(flags & DRAW_EDGES) {
    int i;
    for(i=0; i < edges_n; i++) {
      edge_t *edge = edges[i];
      
      if(layer >= 0 && edge->layer != layer) continue;
      if(edge->flags & EDGEFLAG_INACTIVE) continue;
      if(edge->flags & EDGEFLAG_HILIGHT) {
        cairo_set_source_rgba(c->cr, 1, 0.5, 0, 1.0);
      }else if(edge->layer == -1) {
        cairo_set_source_rgba(c->cr, 1, 1, 1, 0.3);
//      }else if(edge->marked) {
//        cairo_set_source_rgba(c->cr, 1, 0, 0, 0.8);
      }else{
        cairo_set_source_rgba(c->cr, 
            layer_colors[edge->layer][0],
            layer_colors[edge->layer][1],
            layer_colors[edge->layer][2],
            layer_colors[edge->layer][3]);
      }
      cairo_move_to( c->cr, SCALEX(edge->p1->x), SCALEY(edge->p1->y) );
      cairo_line_to( c->cr, SCALEX(edge->p2->x), SCALEY(edge->p2->y) );
      cairo_stroke(c->cr);
    }
  }

  if(flags & DRAW_ORDERS) {
    int i;
    for(i=0; i < edges_n; i++) {
      edge_t *edge = edges[i];
      if(layer >= 0 && edge->layer != layer) continue;

      int txt_x = (edge->p1->x + edge->p2->x) / 2;
      int txt_y = (edge->p1->y + edge->p2->y) / 2;

      char *caption = calloc(32, sizeof(char));
      sprintf(caption, "%d", edge->order);

      cairo_text_extents_t extents;
      cairo_set_source_rgba(c->cr, 1, 1, 1, 1.0);

      cairo_select_font_face (c->cr, "Sans",
          CAIRO_FONT_SLANT_NORMAL,
          CAIRO_FONT_WEIGHT_NORMAL);

      cairo_set_font_size (c->cr, 15.0);
      cairo_text_extents (c->cr, caption, &extents);

      cairo_move_to (c->cr, SCALEX(txt_x), SCALEY(txt_y));
      cairo_show_text (c->cr, caption);

      free(caption);
    }
      
  }
}

void 
repeat_frame(autorouter_t *ar, int count) {
#ifdef TRACE_REPEATS
  if(ar->last_prefix == NULL) {
    printf("repeat_frame: last_prefix == NULL\n");
    return;
  }
  
  FILE *from, *to;
  char *from_filename = calloc(32, sizeof(char));
  char *to_filename = calloc(32, sizeof(char));

  while(--count > 0) {

    sprintf(from_filename, "trace/%05d%s.png", ar->traceseq - 1, ar->last_prefix);
    if((from = fopen(from_filename, "rb"))==NULL) {
      printf("Cannot open %s.\n", from_filename);
      exit(1);
    }

    sprintf(to_filename, "trace/%05d%s.png", ar->traceseq++, ar->last_prefix);
    if((to = fopen(to_filename, "wb"))==NULL) {
      printf("Cannot open %s.\n", to_filename);
      exit(1);
    }

    char ch;

    while(!feof(from)) {
      ch = fgetc(from);
      if(ferror(from)) {
        printf("Error reading %s.\n", from_filename);
        exit(1);
      }
      if(!feof(from)) fputc(ch, to);
      if(ferror(to)) {
        printf("Error writing %s.\n", to_filename);
        exit(1);
      }
    }

    if(fclose(from)==EOF) {
      printf("Error closing %s.\n", from_filename);
      exit(1);
    }

    if(fclose(to)==EOF) {
      printf("Error closing %s.\n", to_filename);
      exit(1);
    }

  }
  free(from_filename);  free(to_filename);
#endif
  return;
}

void
draw_problems(autorouter_t *ar, drawing_context_t *c, problem_t *prob, int flags, char *caption, int layer) {

  cairo_set_source_rgba(c->cr, 1, 1, 1, 0.5);
  if(prob->cut_orientation == CUTX) {
    cairo_move_to( c->cr, SCALEX(prob->cut_loc), SCALEY(prob->y1) );
    cairo_line_to( c->cr, SCALEX(prob->cut_loc), SCALEY(prob->y2) );
  }else{
    cairo_move_to( c->cr, SCALEX(prob->x1), SCALEY(prob->cut_loc) );
    cairo_line_to( c->cr, SCALEX(prob->x2), SCALEY(prob->cut_loc) );
  }

  cairo_stroke( c->cr );

  if(prob->left > 0 && prob->right > 0) {
    draw_problems(ar, c, prob->left, flags, caption, layer);
    draw_problems(ar, c, prob->right, flags, caption, layer);
  }else{
    draw_problem(ar, c, prob, flags, layer);
  }

  cairo_text_extents_t extents;

  cairo_set_source_rgba(c->cr, 0.3, 1, 0.3, 1.0);

  cairo_select_font_face (c->cr, "Sans",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size (c->cr, 15.0);
  cairo_text_extents (c->cr, caption, &extents);

  cairo_move_to (c->cr, SCALEX(30), SCALEY(30) );
  cairo_show_text (c->cr, caption);
}

#endif /* TRACE */

void
draw_caption(autorouter_t *ar, char *caption) {

#ifdef TRACE
#ifdef TRACE_CAPTIONS
  drawing_context_t *c = create_autorouter_drawing_context(ar);
  c->prefix = "caption";
  cairo_text_extents_t extents;
  cairo_set_source_rgba(c->cr, 0.3, 1, 0.3, 1.0);

  cairo_select_font_face (c->cr, "Sans",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size (c->cr, 50.0);
  cairo_text_extents (c->cr, caption, &extents);
  int x = (c->resx - extents.width)/2; //+ extents.x_bearing);
  int y = (c->resy - extents.height)/2;// + extents.y_bearing);
  cairo_move_to (c->cr, x, y);
  cairo_show_text (c->cr, caption);

  finish_drawing(ar, c); 

  repeat_frame(ar, 40); 
#endif 
#endif
  return;
}

void
trace(autorouter_t *ar, problem_t *prob, char *prefix, int flags, int layer) {
#ifdef TRACE
  drawing_context_t *c;
  if(flags & DRAW_PROB) {
    c = create_problem_drawing_context(ar,prob);
    draw_problem(ar, c, prob, flags, layer);
  } else {
    c = create_autorouter_drawing_context(ar);
    draw_problems(ar, c, ar->root_problem, flags, prefix, layer);
  }
  c->prefix = prefix; 
  finish_drawing(ar,c);
#endif
  return;
}

void
trace_pause(autorouter_t *ar) {
#ifdef TRACE
  repeat_frame(ar, 40);
#endif 
  return;
}

netlist_t *
autorouter_create_netlist(autorouter_t *r) {
  netlist_t *t = r->netlists;  

  if(r->netlists_n >= r->netlists_max) {
    r->netlists_max += MALLOC_INC;
    t = realloc(t, r->netlists_max * sizeof(netlist_t));
    if(t == NULL) printf("autorouter_alloc_netlist: realloc failure\n");
    r->netlists = t;
    memset(t + r->netlists_n, 0, MALLOC_INC * sizeof(netlist_t));
  }
  netlist_t *rval = t + r->netlists_n++;

  rval->width = 500;
  rval->spacing = 500;

  return rval;
}

autorouter_t*
autorouter_create()
{
  autorouter_t *x = calloc(1,sizeof(autorouter_t));

  /* board parameters */
  x->board_layers = 0;
  x->board_width = 0; x->board_height = 0;

  x->routing_direction = NULL;

  /* trace parameters */
  x->trace_width = 1024; x->trace_height = 768;
  x->trace_prefix = calloc(16, sizeof(char));
  sprintf(x->trace_prefix, "trace");

  /* autorouter parameters */
//  x->slot_spacing = 2000; // == 20 mil
  x->critical_flow_threshold = 0.75f;
  x->min_sub_prob_netlists = 2;
  x->min_sub_prob_points = 20;
  x->min_sub_prob_aspect_ratio = 0.10f;
  x->min_cut_to_point_seperation = 4000; 
  x->routing_direction_penalty = 2.0f;
  x->via_cost = 10000;
  x->default_width = 4000;
  x->default_spacing = 4000;

  x->max_assign_segments = 6;
  x->via_clearance = 500;
  x->via_radius = 1000;

  x->netlists_n = 0; x->netlists_max = MALLOC_INC;
  x->netlists = calloc(MALLOC_INC, sizeof(netlist_t));

  /* initial problem */

  //x->problems = calloc(MALLOC_INC, sizeof(problem_t));
  //x->problems_n = 0; x->problems_max = MALLOC_INC;

  x->point_pools = alloc_point_pool(x);

  x->problem_pools = autorouter_alloc_problem_pool(x); x->problems_n = 0;
  x->root_problem = autorouter_alloc_problem(x);

#ifdef TRACE
  x->traceseq = 0;
  x->last_prefix = NULL;
#endif

  x->rcounter = 10;

  x->bad_routes = 0;

  return x;
}

#ifdef TEST
int 
autorouter_import_text_netlist(autorouter_t *r, char *filename) {

  FILE *in;
  if((in = fopen(filename, "r"))==NULL) {
    printf("Cannot open %s\n", filename);
    exit(1);
  }
  fscanf(in, "%d,%d,%d,%d,%d,%d\n", &r->board_width, &r->board_height, &r->board_layers,
      &r->default_keepaway, &r->default_linethickness, &r->default_viathickness);

  int rval=5;
  int lnetlist = 0;
  
  r->routing_direction = calloc(r->board_layers, sizeof(int));

  int i;
  for(i=0;i<r->board_layers;i++) {
    r->routing_direction[i] = ((i % 2) ? ROUTING_DIR_HORZ : ROUTING_DIR_VERT );
  }

  while(rval==5) {
    point_t *p = alloc_point(r);
    int type;
    rval = fscanf(in, "%d,%d,%d,%d,%d\n", &p->x, &p->y, &p->netlist, (int *) &type, &p->layer);
    p->type = type;
    if(p->type == PIN) p->layer = -1;
    if(rval==5) {
      point_t **pp = problem_alloc_point(r->root_problem);
      *pp = p;
      if(p->netlist != lnetlist) { 
        autorouter_create_netlist(r);
      }
    }else{
      r->points_n--;
    }
  }
  fclose(in);
  return 0;
}
#endif /* TEST */

#ifndef TEST
int
autorouter_import_pcb_data(autorouter_t *r) {
  /* board parameters */
  r->board_width = PCB->MaxWidth;
  r->board_height = PCB->MaxHeight;
  r->board_layers = max_layer;

  r->routing_direction = calloc(r->board_layers, sizeof(int));
  int i;
  for(i=0;i<r->board_layers;i++) {
    r->routing_direction[i] = ((i % 2) ? ROUTING_DIR_HORZ : ROUTING_DIR_VERT );
  }

  int cur_nl = 0;
  
  r->default_keepaway = Settings.Keepaway;
  r->default_linethickness = Settings.LineThickness;
  r->default_viathickness = Settings.ViaThickness;

#ifdef DEBUG 
  FILE *out;
  if((out = fopen("netlist.txt", "w"))==NULL) {
    printf("Cannot open netlist.txt\n");
    exit(1);
  }

  fprintf(out, "%d,%d,%d,%d,%d,%d\n", r->board_width, r->board_height, r->board_layers,
      r->default_keepaway, r->default_linethickness, r->default_viathickness);
#endif
  
  ELEMENT_LOOP(PCB->Data);
  {
    printf("Element Descriptions:\n");
    printf("\t- %s\n", element->Name[0].TextString);
    printf("\t- %s\n", element->Name[1].TextString);
    printf("Element Pins:\n");

    PIN_LOOP(element);
    {
      point_t *p = alloc_point(r);
      *problem_alloc_point(r->root_problem) = p;

      p->x = pin->X;
      p->y = pin->Y;
      p->thickness = pin->Thickness;
      p->clearance = pin->Clearance;
      p->type = PIN;
      p->layer = -1;
      p->netlist = -1;

      printf("\t- %d,%d\n", pin->X, pin->Y);
    }
    END_LOOP;

  }
  END_LOOP;

  ResetFoundPinsViasAndPads(False);
  NetListListType nets = CollectSubnets(False);
  NETLIST_LOOP(&nets);
  {
    netlist_t *nl = autorouter_create_netlist(r);
    nl->name = netlist->Net->Connection->menu->Name;
    nl->style = netlist->Net->Connection->menu->Style;
    
    NET_LOOP(netlist);
    {
      bool found = false;

      TAR_PROB_POINTS_LOOP(r->root_problem);
      {
        if(point->x == net->Connection->X && point->y == net->Connection->Y) {
          point->netlist = cur_nl;
          found = true;
#ifdef DEBUG 
      fprintf(out, "%d,%d,%d,%d,%d\n", point->x, point->y, point->netlist, point->type, point->layer);
#endif
          break;
        }
          
      }
      TAR_END_LOOP;

      if(!found) printf("ERROR: Couldn't find point %d,%d contained in netlist...\n", 
          net->Connection->X, net->Connection->Y);

    }
    END_LOOP;

    cur_nl++;
  }
  END_LOOP;
  FreeNetListListMemory(&nets);
      
  TAR_PROB_POINTS_LOOP(r->root_problem);
  {
    if(point->netlist == -1) {
      point->netlist = cur_nl++;
      netlist_t *nl = autorouter_create_netlist(r);
      nl->name = "DEFAULT"; 
      nl->style = "DEFAULT";
#ifdef DEBUG 
      fprintf(out, "%d,%d,%d,%d,%d\n", point->x, point->y, point->netlist, point->type, point->layer);
#endif

    }

  }
  TAR_END_LOOP;

#ifdef DEBUG
  fclose(out);
#endif

  return 0;
}
#endif /* TEST */

void
heap_swap(point_t **p, int x, int y) {
  point_t *temp = p[x];
  p[x] = p[y];
  p[y] = temp;
}

void
heapify(problem_t *nl, void (*heap_sift_func)(problem_t *,int,int) ) {
  int n;
  for(n=HEAP_PARENT(nl->points_n); n >= 0; n--) {
    heap_sift_func(nl, n, nl->points_n-1);
  }
}

void
heap_sort(problem_t *nl, void (*heap_sift_func)(problem_t *,int,int) ) {
  heapify(nl, heap_sift_func);

  int n;
  for(n=nl->points_n-1;n>0;n--) {
//    heap_swap(nl->points[0], nl->points[n]);  
    heap_swap(nl->points, 0, n);  
    heap_sift_func(nl, 0, n-1);
  }

}

void
dump_points(problem_t *prob) {
  printf("Points dump:\n");
  int i;
  for(i=0;i<prob->points_n;i++) {
    point_t *p = prob->points[i];
    printf("%d,%d\n", p->x, p->y); 
  }
}

void
check_y_sort(problem_t *prob) {
  int lasty = 0;
  int i;
  for(i=0;i<prob->points_n;i++) {
    point_t *p = prob->points[i];
    
    if(p->y < lasty) {
      printf("ERROR (ysort)\n");
      dump_points(prob);
    }

    lasty = p->y;
  }
}

void
check_x_sort(problem_t *prob) {
  int lastx = 0;
  int i;
  for(i=0;i<prob->points_n;i++) {
    point_t *p = prob->points[i];
    
    if(p->x < lastx) {
      printf("ERROR (xsort)\n");
      dump_points(prob);
    }

    lastx = p->x;
  }
}

void
heap_sift_down_y(problem_t *nl, int start, int end) {

  int n;
  for(n = start; HEAP_LEFT(n) <= end; ) {
    int child = HEAP_LEFT(n);
    if(child<end && nl->points[child]->y < nl->points[child+1]->y) 
      child++;
    if(nl->points[n]->y < nl->points[child]->y) {
//      heap_swap( nl->points[n], nl->points[child] );
      heap_swap( nl->points, n, child );
      n = child;
    }
    else return;
  }
}

void
heap_sift_down_x(problem_t *nl, int start, int end) {

  int n;
  for(n = start; HEAP_LEFT(n) <= end; ) {
    int child = HEAP_LEFT(n);
    if(child<end && nl->points[child]->x < nl->points[child+1]->x) 
      child++;
    if(nl->points[n]->x < nl->points[child]->x) {
      //heap_swap( nl->points[n], nl->points[child] );
      heap_swap( nl->points, n, child );
      n = child;
    }
    else return;
  }
}

  double **
cross_point_cost(autorouter_t *ar, problem_t *prob, point_t *p) 
{
  double **rval = calloc(ar->board_layers, sizeof(double *));

  int i;
  for(i=0;i<ar->board_layers;i++) {
    rval[i] = calloc(prob->cut_crossing_netlists_n, sizeof(double));
  }

  triangulation_t *ts = calloc(2*prob->cut_crossing_netlists_n, sizeof(triangulation_t));

  for(i=0;i<2*prob->cut_crossing_netlists_n;i++) {
    triangulation_create(MAX_BOARD_DIM(ar), &ts[i]);
  }

  TAR_PROB_POINTS_LOOP(prob);
  {
    int j;
    for(j=0;j<prob->cut_crossing_netlists_n;j++) {
      if(point->netlist == prob->cut_crossing_netlists[j]) {

        if(((prob->cut_orientation==CUTX)?point->x:point->y) < prob->cut_loc) {
          triangulation_insert_point(&ts[j],point);
        }else{
          triangulation_insert_point(&ts[j + prob->cut_crossing_netlists_n],point);
        }
        break;
      }
    }

  }
  TAR_END_LOOP;

  for(i=0;i<2*prob->cut_crossing_netlists_n;i++) {
    triangulation_insert_point(&ts[i],p);
    //triangulation_compute_edges(&ts[i]);
    triangulation_compute_mst(ar, &ts[i], false);

    TAR_MST_EDGES_LOOP(&ts[i]);
    {
      int j;
      for(j=0;j<ar->board_layers;j++) 
        rval[j][i % prob->cut_crossing_netlists_n] += edge->length;
    }
    TAR_END_LOOP;

    triangulation_destroy(&ts[i]);

  }
    
  free(ts);

  // compute routability penalty
  for(i=0;i<prob->cut_crossing_netlists_n;i++) {
    int j;
    for(j=0;j<ar->board_layers;j++) { 
      if(prob->cut_orientation == CUTX && ar->routing_direction[j] == ROUTING_DIR_VERT) {
        rval[j][i] *= ar->routing_direction_penalty;
      }else 
        if(prob->cut_orientation == CUTY && ar->routing_direction[j] == ROUTING_DIR_HORZ) {
          rval[j][i] *= ar->routing_direction_penalty;
        }
    }
  } 
  return rval;
}

  void
assign_cross_points(autorouter_t *ar, problem_t *prob, problem_t *left, problem_t *right)
{
  prob->cut_crossing_netlist_slots = calloc(prob->cut_crossing_netlists_n, sizeof(int));
  prob->cut_crossing_netlist_layers = calloc(prob->cut_crossing_netlists_n, sizeof(int));


  int slot_width = ar->default_spacing + ar->default_width;

  int slots_n = ((prob->cut_orientation==CUTX)?prob->height:prob->width) / slot_width;

  // [slot] [layer] [netlist]
  double ***wire_costs = calloc(slots_n, sizeof(double **));

  int i;
  for(i=0;i<slots_n;i++) {
    point_t p;
    p.x = (prob->cut_orientation==CUTX) ? prob->cut_loc : (prob->x1 + (i * slot_width) + (slot_width / 2));
    p.y = (prob->cut_orientation==CUTY) ? prob->cut_loc : (prob->y1 + (i * slot_width) + (slot_width / 2));

    wire_costs[i] = cross_point_cost(ar, prob, &p);

  }

  if(prob->cut_crossing_netlists_n <= 0) {
    printf("- cut_crossing_netlists_n <= 0 \n");
  }

  int *assigned_slot = calloc(prob->cut_crossing_netlists_n, sizeof(int));
  int *assigned_layer = calloc(prob->cut_crossing_netlists_n, sizeof(int));

  for(i=0;i<prob->cut_crossing_netlists_n;i++) {

    double min_cost = -1.0f;
    int min_slot = -1, min_layer = -1;

    int j;
    for(j=0;j<slots_n;j++) {

      int k;
      for(k=0;k<ar->board_layers;k++) {
        int skip = 0;

        int i2;
        for(i2=0;i2<i;i2++) {
          if(assigned_slot[i2] == j && assigned_layer[i2] == k) {
            skip=1;
            break;
          }
        }

        if(skip) break;

        if(wire_costs[j][k][i] < min_cost || min_cost < 0) {
          min_cost = wire_costs[j][k][i];
          min_slot = j;
          min_layer = k;
        }

      }

    }

    assigned_slot[i] = min_slot;
    assigned_layer[i] = min_layer;

  }

  int j;
  point_t **pp, *cp;

  for(j=0;j<prob->cut_crossing_netlists_n;j++) {
    cp = alloc_point(ar);

    //point_t *pl = problem_alloc_point(left);
    //point_t *pr = problem_alloc_point(right);

    if(prob->cut_orientation == CUTX) {
      cp->x = prob->cut_loc;
      cp->y = prob->y1 + (slot_width/2) + (assigned_slot[j] * slot_width);
    } else {
      cp->y = prob->cut_loc;
      cp->x = prob->x1 + (slot_width/2) + (assigned_slot[j] * slot_width);
    }
    cp->layer = -1;//assigned_layer[j];
    cp->type = CROSSPOINT;
//    pr->dual = pl;
//    pl->dual = pr;
    cp->netlist = prob->cut_crossing_netlists[j];
    
    pp = problem_alloc_point(left);
    *pp = cp;
    pp = problem_alloc_point(right);
    *pp = cp;
  }

  free(assigned_slot);
  free(assigned_layer);

  for(j=0;j<slots_n;j++) {
    int k;
    for(k=0;k<ar->board_layers;k++) {
      free(wire_costs[j][k]);
    }
    free(wire_costs[j]);
  }
  free(wire_costs);

}

void
laa_triangulate(autorouter_t *ar, problem_t *prob) {

//  heap_sort(prob,heap_sift_down_x);

  prob->ts = calloc(ar->netlists_n, sizeof(triangulation_t));
  prob->ts_n = ar->netlists_n;
  triangulation_t *netlist_ts = prob->ts;
  
  int i;
  for(i=0;i<ar->netlists_n;i++) {
    triangulation_create(MAX_BOARD_DIM(ar), &netlist_ts[i]);
  }

  TAR_PROB_POINTS_LOOP(prob);
  {
    triangulation_insert_point(&netlist_ts[point->netlist], point);

    trace(ar, ar->root_problem, "triangulate", DRAW_POINTS | DRAW_TRIAGS, -1);

  }
  TAR_END_LOOP;


}

void
laa_mst(autorouter_t *ar, problem_t *prob) {
  
  triangulation_t *netlist_ts = prob->ts;
  
  int i;
  for(i=0;i<ar->netlists_n;i++) {
    //triangulation_compute_edges( &netlist_ts[i] );
    triangulation_compute_mst( ar, &netlist_ts[i], true );

    TAR_MST_EDGES_LOOP( &netlist_ts[i] );
    {
      edge_t *newedge = problem_alloc_edge(prob);
      memcpy(newedge, edge, sizeof(edge_t));

//      edge_t **ep = problem_alloc_edge_ptr(prob);
//      *ep = newedge;
      newedge->layer = -1;
    }
    TAR_END_LOOP;

  }  


}

  int
divide(autorouter_t *r, problem_t *prob)
{
  
 // laa_sort_and_triangulate(r, prob);

  problem_t *left_child = NULL, *right_child = NULL;

  /* pre-divide checks: */

  if(prob->points_n < r->min_sub_prob_points) {
    printf("divide: points_n less than threshold\n");
    return 0;
  }

  int cut = CUTX;

  if(prob->width > prob->height) {
    heap_sort(prob,heap_sift_down_x);
    //TODO: remove this check.. seems to catch some unexpected cases
    check_x_sort(prob);
    cut = CUTX;
  }else{
    heap_sort(prob,heap_sift_down_y);
    //TODO: remove this check.. seems to catch some unexpected cases
    check_y_sort(prob);
    cut = CUTY;
  }

  int last_point = 0;

  int *left_points = calloc(r->netlists_n, sizeof(int));
  int *right_points = calloc(r->netlists_n, sizeof(int));

  int min_cut = 0, max_cut = 0;
  float min_flow_density = 1.0, max_flow_density = 0.0;

  int *min_crossing_netlists, *max_crossing_netlists;
  int min_crossing_netlists_n = 0, max_crossing_netlists_n = 0;
  min_crossing_netlists = calloc(r->netlists_n, sizeof(int));
  max_crossing_netlists = calloc(r->netlists_n, sizeof(int));


  TAR_PROB_POINTS_LOOP(prob);
  {
    //printf("point %d,%d netlist %d\n", point->x, point->y, point->netlist);
    
    /* consider cut suitability */

    int cut_point = (((cut==CUTX) ? point->x : point->y) + last_point)/2;
    int left_width, right_width, left_height, right_height;
    if(cut==CUTX) {
      left_width = cut_point - prob->x1;
      right_width = prob->x2 - cut_point;
      right_height = left_height = prob->height;
    }else{
      left_height = cut_point - prob->y1;
      right_height = prob->y2 - cut_point;
      right_width = left_width = prob->width;
    }

    int left_netlists = 0;
    int right_netlists = 0;

    int left_crosspoints = 0;
    int right_crosspoints = 0;

    memset(left_points, 0, r->netlists_n * sizeof(int));
    memset(right_points, 0, r->netlists_n * sizeof(int));
    TAR_PROB_POINTS_LOOP(prob);
    {
      if(((cut==CUTX)?point->x:point->y) < cut_point) {
        if(point->type == CROSSPOINT)
          left_crosspoints++;
        left_points[point->netlist] ++;
      }else{
        if(point->type == CROSSPOINT)
          right_crosspoints++;
        right_points[point->netlist] ++;
      }
    }
    TAR_END_LOOP;

    int nlc;
    for(nlc = 0; nlc < r->netlists_n; nlc++) {
      if(left_points[nlc] > 0) left_netlists++;
      if(right_points[nlc] > 0) right_netlists++;
    }

    float left_aspect_ratio = (float)MIN(left_width, left_height) / (float)MAX(left_width,left_height);
    float right_aspect_ratio = (float)MIN(right_width, right_height) / (float)MAX(right_width,right_height);
    //float aspect_ratio = (float)MIN(prob->width, prob->height) / (float)MAX(prob->width,prob->height);

    /* ensure enough points in left half */
    if(n - left_crosspoints >= r->min_sub_prob_points)
      /* ensure enough points in right half */
      if(prob->points_n - n - right_crosspoints  >= r->min_sub_prob_points)
        /* ensure enough netlists */
        if(left_netlists >= r->min_sub_prob_netlists)
          if(right_netlists >= r->min_sub_prob_netlists)
            /* ensure aspect ratio */
            if( left_aspect_ratio > r->min_sub_prob_aspect_ratio )
              if( right_aspect_ratio > r->min_sub_prob_aspect_ratio )
                /* ensure min seperation between cut and other points */
                if( ((cut==CUTX) ? point->x : point->y) - last_point > r->min_cut_to_point_seperation )
                {

                  int flow=0;


                  int u;
                  for(u=0;u<r->netlists_n;u++) {
                    if(left_points[u] > 0 && right_points[u] > 0) {
                      flow += r->netlists[u].width + r->netlists[u].spacing;
                    }
                  }

                  int capacity = ((cut==CUTX) ? prob->height : prob->width) * r->board_layers;

                  float density = (float)flow / (float) capacity;

                  if(density < 1.0) {

                    if(density < min_flow_density) {
                      min_flow_density = density;
                      min_cut = cut_point;

                      //memset(min_crossing_netlists, 0, r->netlists_n * sizeof(int));
                      min_crossing_netlists_n = 0;
                      int u;
                      for(u=0;u<r->netlists_n;u++) {
                        if(left_points[u] > 0 && right_points[u] > 0) {
                          min_crossing_netlists[min_crossing_netlists_n] = u;
                          min_crossing_netlists_n += 1; 
                        }
                      }

                    }else if(density > max_flow_density){
                      max_flow_density = density;
                      max_cut = cut_point;

                      //memset(max_crossing_netlists, 0, r->netlists_n * sizeof(int));
                      max_crossing_netlists_n = 0;
                      int u;
                      for(u=0;u<r->netlists_n;u++) {
                        if(left_points[u] > 0 && right_points[u] > 0) {
                          max_crossing_netlists[min_crossing_netlists_n] = u;
                          max_crossing_netlists_n += 1; 
                        }
                      }
                    }
                  }else{
                    printf("Warning: flow density > 1.0\n");

                  }
                }

    last_point = ((cut==CUTX) ? point->x : point->y );
  }
  TAR_END_LOOP;

  if(min_cut > 0 || max_cut > 0) {
    prob->left = left_child = autorouter_alloc_problem(r);
    prob->right = right_child = autorouter_alloc_problem(r);

    prob->cut_orientation = cut;

    if(max_flow_density > r->critical_flow_threshold) {
      //      printf("Selecting max cut with %d netlists crossing\n", max_crossing_netlists_n);
      prob->cut_loc = max_cut;
      prob->cut_flow_density = max_flow_density;
      prob->cut_crossing_netlists = max_crossing_netlists;
      prob->cut_crossing_netlists_n = max_crossing_netlists_n;
    }else{
      //      printf("Selecting min cut with %d netlists crossing\n", min_crossing_netlists_n);
      prob->cut_loc = min_cut;
      prob->cut_flow_density = min_flow_density;
      prob->cut_crossing_netlists = min_crossing_netlists;
      prob->cut_crossing_netlists_n = min_crossing_netlists_n;
    }

    if(cut == CUTX) {
      left_child->x1 = prob->x1;
      left_child->x2 = prob->cut_loc;
      right_child->x1 = prob->cut_loc;
      right_child->x2 = prob->x2;

      left_child->y1 = right_child->y1 = prob->y1;
      left_child->y2 = right_child->y2 = prob->y2;
    }else{
      left_child->y1 = prob->y1;
      left_child->y2 = prob->cut_loc;
      right_child->y1 = prob->cut_loc;
      right_child->y2 = prob->y2;

      left_child->x1 = right_child->x1 = prob->x1;
      left_child->x2 = right_child->x2 = prob->x2;
    }
    left_child->width = left_child->x2 - left_child->x1;
    left_child->height = left_child->y2 - left_child->y1;
    right_child->width = right_child->x2 - right_child->x1;
    right_child->height = right_child->y2 - right_child->y1;

    point_t **pp;
    TAR_PROB_POINTS_LOOP(prob);
    {
      if(((cut==CUTX) ? point->x : point->y) < prob->cut_loc) {
        pp = problem_alloc_point(left_child);
        *pp = point;
//        memcpy(problem_alloc_point(left_child), point, sizeof(point_t));
      }else{
        pp = problem_alloc_point(right_child);
        *pp = point;
//        memcpy(problem_alloc_point(right_child), point, sizeof(point_t));
      }
    }
    TAR_END_LOOP;

    assign_cross_points(r, prob, left_child, right_child);  

    printf("Cut selected at %d with flow density of %f\n", prob->cut_loc, prob->cut_flow_density);
  }else{

    /* No Suitable cut foudn */
  }
  free(left_points);
  free(right_points);
  free(min_crossing_netlists);
  free(max_crossing_netlists);

  trace(r, r->root_problem, "divide", DRAW_POINTS | DRAW_TRIAGS, -1);

  if(prob->cut_loc > 0) {
    divide(r, prob->left);
    divide(r, prob->right);
  }

  return 1;
}

int
intersect_prop(point_t *a, point_t *b, point_t *c, point_t *d) {

  if( wind(a, b, c) == 0 || 
      wind(a, b, d) == 0 ||
      wind(c, d, a) == 0 || 
      wind(c, d, b) == 0 ) return 0;

  return ( wind(a, b, c) ^ wind(a, b, d) ) && 
    ( wind(c, d, a) ^ wind(c, d, b) );
}

/*
 * returns true if c is between a and b 
 */
int
between(point_t *a, point_t *b, point_t *c) {
  if( wind(a, b, c) != 0 ) return 0;

  if( a->x != b->x ) {
    return ((a->x <= c->x) &&
        (c->x <= b->x)) ||
      ((a->x >= c->x) &&
       (c->x >= b->x));
  }
  return ((a->y <= c->y) &&
      (c->y <= b->y)) ||
    ((a->y >= c->y) &&
     (c->y >= b->y));
}

int 
intersect(point_t *a, point_t *b, point_t *c, point_t *d) {
  if( intersect_prop(a, b, c, d) ) {
    return 1;
  }
  else if( between( a, b, c) ||
      between( a, b, d) || 
      between( c, d, a) ||
      between( c, d, b) ) {
    return 1;
  }
  return 0;
}

void
intersect_point(point_t *a, point_t *b, point_t *c, point_t *d, point_t *rval) {
  double ua_top = (((double)d->x - (double)c->x) * ((double)a->y - (double)c->y)) - (((double)d->y - (double)c->y) * ((double)a->x - (double)c->x));
  double ua_bot = (((double)d->y - (double)c->y) * ((double)b->x - (double)a->x)) - (((double)d->x - (double)c->x) * ((double)b->y - (double)a->y));
  double ua = ua_top / ua_bot;
  double rx = a->x + (ua * (b->x - a->x));
  double ry = a->y + (ua * (b->y - a->y));

  rval->x = (int) rx;
  rval->y = (int) ry;

  rval->type = INTERSECT;
}

double 
detour(int lastx, int lasty, int nextx, int nexty, edge_t *e2) {
  point_t *c = e2->p1; point_t *d = e2->p2;
  double len1 = length(lastx, lasty, c->x, c->y) + length(c->x, c->y, nextx, nexty);
  double len2 = length(lastx, lasty, d->x, d->y) + length(d->x, d->y, nextx, nexty);
  return MIN(len1, len2);
}

void
apply_direction_penaltys(autorouter_t *ar, edge_t *edge, double *column){
  int j;
  for(j=0;j<ar->board_layers;j++) {
    if(abs(edge->p2->x - edge->p1->x) > abs(edge->p2->y - edge->p1->y)) {
      if(ar->routing_direction[j] == ROUTING_DIR_VERT) {
        column[j] *= ar->routing_direction_penalty;
      }
    }else{
      if(ar->routing_direction[j] == ROUTING_DIR_HORZ)
        column[j] *= ar->routing_direction_penalty;
    }
  }
}

int
count_point_edges(autorouter_t *ar, problem_t *prob, point_t *p, int layer) {
  int count = 0;

  int i;
  for(i=0;i<prob->edges_n;i++) {
    edge_t *edge = prob->edges[i];

    if(edge->layer == layer)
      if(edge->p1 == p || edge->p2 == p) count++;
  }

  return count;
}


/* this should be unnessacary. should be maintained by code modifying edges */
inline void
compute_lengths(autorouter_t *ar, problem_t *prob) {
  int i;
  for(i=0;i<prob->edges_n;i++) {
    edge_t *edge = prob->edges[i];
    edge->length = length(edge->p1->x, edge->p1->y, edge->p2->x, edge->p2->y); 
  }
}


void
assign_path(autorouter_t *ar, problem_t *prob, edge_t *edge, int segs, int *path) {
  int i;
/*
  if((edge->p1->layer != path[0] && edge->p1->type != VIA && edge->p1->type != PIN && edge->p1->layer >= 0)
  || (edge->p2->layer != path[segs-1] && edge->p2->type != VIA && edge->p2->type != PIN && edge->p2->layer >=0))
  printf("edge p1 layer = %d -> %d, p2 layer = %d -> %d\n", edge->p1->layer, path[0], 
    edge->p2->layer, path[segs-1]);
*/
  point_t *lastpoint = edge->p2;

  edge_t *prevedge = edge;
  prevedge->layer = path[0];

  for(i=1;i<segs;i++) {
    if(path[i] != prevedge->layer) {
      point_t *p = alloc_point(ar);

      p->x = edge->p1->x + ( (lastpoint->x - edge->p1->x) / segs * i ); 
      p->y = edge->p1->y + ( (lastpoint->y - edge->p1->y) / segs * i ); 
      p->type = VIA;
      p->thickness = ar->default_viathickness;
      p->netlist = edge->p1->netlist;
      p->layer = -1;
      
      point_t **pp = problem_alloc_point(prob);
      *pp = p;
      
      edge_t *newedge = problem_alloc_edge(prob);
      newedge->p1 = p;
      newedge->p2 = lastpoint;
      newedge->layer = path[i];

      prevedge->p2 = p;

      prevedge = newedge;

    }

  }

  //if(edge->p1->layer == -1) 
  if(edge->p1->type != PIN && edge->p1->type != VIA) { 
    if(edge->p1->type == PAD && edge->p1->layer >= 0 && edge->p1->layer != path[0]) 
      printf("reassigning p1 pad layer..!!\n");
    edge->p1->layer = path[0];
  }
  //if(prevedge->p2->layer == -1) 
  if(lastpoint->type != PIN && lastpoint->type != VIA) { 
    if(lastpoint->type == PAD && lastpoint->layer >= 0 && lastpoint->layer != path[segs-1]) 
      printf("reassigning p2 pad layer..!!\n");
    lastpoint->layer = path[segs-1];
  }
/*    
  collison_t *col = edge->cols;
  while(col != NULL) {
//    printf("col @ %d,%d:\t", col->x, col->y);
    int prevx = edge->p1->x; 
    int prevy = edge->p1->y;

    for(i=1;i<=segs;i++) {
      int x = edge->p1->x + ( (lastpoint->x - edge->p1->x) / segs * i ); 
      int y = edge->p1->y + ( (lastpoint->y - edge->p1->y) / segs * i );
//      if(MIN(prevx,x) < col->x && col->x <= MAX(prevx,x)) 
//        if(MIN(prevy,y) < col->y && col->y <= MAX(prevy,y)) {
      if(prevx < col->x && col->x <= x) 
        if(prevy < col->y && col->y <= y) {
//          printf("assigning to layer %d", path[i-1]);
          col->layer = path[i-1];
      }
      prevx = x; prevy = y;
    }
//    printf("\n");
    col = col->next;
  }
*/
}

int *
permutation(int k, int s) {
  int *seq = calloc(s, sizeof(int));
  
  int j;
  for(j=0;j<s;j++) {
    seq[j] = j;
  }

  for(j=2; j<s; j++) {
    k = k / (j-1);
    int temp = seq[(k % j) + 1];
    seq[(k % j) + 1] = seq[j];
    seq[j] = temp;
  }
  return seq;
}

int 
factorial(int n) {
  int r = n;
  int i = n-1;
  while(i > 0) {
    r *= i;  
    i--;
  }
  return r;
}

void
print_cost_matrix(autorouter_t *ar, double **m, int segs) {
  printf("\n************* COST MATRIX **************\n");	
  int j;
  for(j=0;j<ar->board_layers;j++) {
    int mi;
    for(mi=0;mi<segs;mi++) {
      printf("%f\t", m[mi][j]);

    }
    printf("\n");
  }
}

void
print_path(autorouter_t *ar, int *path, int segs) {
  printf("*** %d segs PATH: ", segs);
  int j;
  for(j=0;j<segs;j++) {
    printf("%d\t", path[j]); 
  }
  printf("\n");
}


double 
path_score(autorouter_t *ar, double **m, int *path, int segs) {
  double r = 0;
  
  int prevlayer = path[0];

  int i;
  for(i=1;i<segs;i++) {
    if(path[i] != prevlayer) 
      r += (double) ar->via_cost;
    r += m[i][ path[i] ];
    prevlayer = path[i];
  }

  return r;
}

int 
count_collisons(autorouter_t *ar, problem_t *prob, point_t *p1, point_t *p2, int layer) {
  int count = 0;
  int i;
  for(i=0;i<prob->edges_n;i++) {
    edge_t *e2 = prob->edges[ i ];

    if(e2->layer == layer) {
      if( intersect(p1, p2, e2->p1, e2->p2) && 
          ( p1 != e2->p1 && p1 != e2->p2 && p2 != e2->p1 && p2 != e2->p2 ) ) {
      
        count++;
      }
    }
  }
  return count;
}

inline bool
pointcmp(point_t *p1, point_t *p2) {
  return (p1->x == p2->x && p1->y == p2->y);
}

inline bool
lines_joined(point_t *a1, point_t *a2, point_t *b1, point_t *b2) {
  return pointcmp(a1,b1) || pointcmp(a1,b2) || pointcmp(a2,b1) || pointcmp(a2,b2); 
}

int 
count_net_crosspoints(autorouter_t *ar, problem_t *prob, edge_t *edge, int layer) {
//                      edge_t *caller_edge, point_t *caller_point, int layer) {

  int count = 0;

//  if(edge->p1 != caller_point && edge->p1->type == CROSSPOINT) count++;
//  if(edge->p2 != caller_point && edge->p2->type == CROSSPOINT) count++;
  if( edge->p1->type == CROSSPOINT ) count++;
  if( edge->p2->type == CROSSPOINT ) count++;

  int i;
  for(i=0;i<prob->edges_n;i++) {
    edge_t *e2 = prob->edges[ i ];
    
    if(e2->mark == ar->rcounter) continue;
    e2->mark = ar->rcounter;

    if(e2 == edge) continue;
//    if(e2 == caller_edge) continue;
    if(e2->layer == -1 || e2->layer != layer) continue;
    if(e2->p1->netlist != edge->p1->netlist) continue;

    if( pointcmp(edge->p1, e2->p1) ) {
//      count += count_net_crosspoints(ar,prob,e2,edge,edge->p1,layer);
      count += count_net_crosspoints(ar,prob,e2,layer);
    }else if( pointcmp(edge->p1, e2->p2) ) {
//      count += count_net_crosspoints(ar,prob,e2,edge,edge->p1,layer);
      count += count_net_crosspoints(ar,prob,e2,layer);
    }else if( pointcmp(edge->p2, e2->p1) ) {
//      count += count_net_crosspoints(ar,prob,e2,edge,edge->p2,layer);
      count += count_net_crosspoints(ar,prob,e2,layer);
    }else if( pointcmp(edge->p2, e2->p2) ) {
//      count += count_net_crosspoints(ar,prob,e2,edge,edge->p2,layer);
      count += count_net_crosspoints(ar,prob,e2,layer);
    }
  }

  return count;
}

double
seg_cost(autorouter_t *ar, problem_t *prob, int *path, edge_t *edge, int segs, int depth, int layer, bool logged) {

  if(depth == 0 && edge->p1->layer >= 0 && edge->p1->layer != layer) {
    if(edge->p1->type == CROSSPOINT || edge->p1->type == PAD) {
      return -1.0f;
    }
  }
  
  if(depth == (segs-1) && edge->p2->layer >= 0 && edge->p2->layer != layer) {
    if(edge->p2->type == CROSSPOINT || edge->p2->type == PAD) { 
//      if(logged && depth==6 && layer==1) printf("EXIT\n");
      return -1.0f;
    }
  }

  double r = 0.0f;
  /* check segment against all edges for collison */

  int x_inc = (edge->p2->x - edge->p1->x) / segs;
  int y_inc = (edge->p2->y - edge->p1->y) / segs;	

  point_t p1, p2;
  p1.x = (depth * x_inc) + edge->p1->x;
  p1.y = (depth * y_inc) + edge->p1->y;
  p2.x = ((depth + 1) * x_inc) + edge->p1->x;
  p2.y = ((depth + 1) * y_inc) + edge->p1->y;	

  int i;
  for(i=0;i<prob->edges_n;i++) {
    edge_t *e2 = prob->edges[ i ];

    if(e2->layer == layer) {
      if( intersect(&p1, &p2, e2->p1, e2->p2) && !lines_joined(&p1, &p2, e2->p1, e2->p2) 
        && wind(&p1, &p2, e2->p1) != 0
        && wind(&p1, &p2, e2->p2) != 0) { 

        printf("INTERSECT\n");
        ar->rcounter++;

        /* WTF is going on here? I don't think this prevents failure */
        if(count_net_crosspoints(ar,prob,e2,e2->layer) >= 2) {
          if(logged) printf("count_net_crosspoints >= 2\n");
          bool discard = true;
          /* backtrack looking for via */
          int j;
          for(j=depth-1;j>0;j--) {
            if(path[j] != layer) {
              discard = false;
              break;
            }
          }

          edge->flags |= EDGEFLAG_FORCEVIA;

          //if(discard) return -1.0f;
        }

        point_t ip;
        intersect_point(edge->p1, edge->p2, e2->p1, e2->p2, &ip);

        double p1_detour = -1.0f, p2_detour = -1.0f;

        if(e2->p1->type == VIA || e2->p1->type == PAD || e2->p1->type == PIN ) {

            p1_detour = length(p1.x, p1.y, e2->p1->x, e2->p1->y) +
                        length(e2->p1->x, e2->p1->y, p2.x, p2.y);

            p1_detour *= 10.0f;

            if(e2->p1->type == PAD || e2->p1->type == PIN) {
              double temp1 = (double) count_point_edges(ar,prob,e2->p1,layer);
              double temp2 = (double) count_collisons(ar,prob,&ip,e2->p1,layer);
              if(temp1 < 1) temp1 = 1.0f;
              if(temp2 < 1) temp2 = 1.0f;
              p1_detour *= temp1;
              p1_detour *= temp2;
            }
        }
        
        if(e2->p2->type == VIA || e2->p2->type == PAD || e2->p2->type == PIN ) {

            p2_detour = length(p1.x, p1.y, e2->p2->x, e2->p2->y) +
                        length(e2->p2->x, e2->p2->y, p2.x, p2.y);
            
            p2_detour *= 10.0f;

            if(e2->p2->type == PAD || e2->p2->type == PIN) {
              double temp1 = count_point_edges(ar,prob,e2->p2,layer);
              double temp2 = count_collisons(ar,prob,&ip,e2->p2,layer);
              if(temp1 < 1) temp1 = 1.0f;
              if(temp2 < 1) temp2 = 1.0f;
              p2_detour *= temp1;
              p2_detour *= temp2;
            }
        }

        printf("intersect.. detours = %f %f\n", p1_detour, p2_detour);

        if(p1_detour < 0 && p2_detour < 0) { 
          if(logged) {
            printf("both detours invalid depth=%d layer=%d p1_detour=%f p2_detour=%f e2 types=%d-%d\n", 
              depth, layer, p1_detour, p2_detour, e2->p1->type, e2->p2->type);
//            edge->marked = true;
          }
          return -1.0f;
        }
        else if(p1_detour < 0) r += p2_detour;
        else if(p2_detour < 0) r += p1_detour;
        else r += MIN(p1_detour,p2_detour);
      }
    }
  }
  

//  if(logged) printf("reached end.. r =%f\n", r);
  /* if collison, assign min detour (or illegal seg), otherwise return seg length */
  if(r == 0.0f) r = length(p1.x, p1.y, p2.x, p2.y);
  return r;
}

void
find_best_path(autorouter_t *ar, problem_t *prob, edge_t *edge,
               int segs, int depth, int *path, double score, 
							 int *minpath, double *minscore, bool logged) {

  double prevscore = score;

  int i;
  for(i=0;i<ar->board_layers;i++) {

    double segcost = seg_cost(ar,prob,path,edge,segs,depth,i,logged);

    if(logged) printf("segcost1 = %f\n", segcost);
		
    if(segcost < 0) continue;

    score = prevscore + segcost;
    path[depth] = i;

    if(depth>0) if(path[depth-1] != path[depth]) score += (double) ar->via_cost;

    if(logged) printf("segcost2 = %f\n", segcost);

    if(depth < segs-1) 
      find_best_path(ar, prob, edge, segs, depth+1, path, score, minpath, minscore, logged);
    else {
//      printf("path cost = %f :  ", score);
//      print_path(ar, path, segs);

      if(minscore[0] < 0 || score <= minscore[0]) {
        bool update_path = true;
        if(edge->flags & EDGEFLAG_FORCEVIA) {
          update_path = false;

          int j;
          for(j=1;j<segs;j++) {
            if(path[j] != path[j-1]) update_path = true;
          }

        }

        if(update_path) {
          minscore[0] = score;
          memcpy(minpath, path, segs * sizeof(int));
        }

      }
      //return;
    }
  }

}

void 
spread_vias(autorouter_t *ar, problem_t *prob) {

  TAR_PROB_POINTS_LOOP(prob);
  {
    if(point->type == VIA) {
      
      edge_t *e1 = NULL, *e2 = NULL;
        
      TAR_EDGES_LOOP(prob);
      {
        if(edge->p1 == point || edge->p2 == point) {
          if(e1 == NULL) e1 = edge;
          else {
            e2 = edge;
            continue;
          }
        }
      }
      TAR_EDGES_ENDLOOP;

      if(e2 == NULL) {
        printf("Couldn't find the two edges connected to a via...\n");
        return;
      }
      
      point_t *p1 = (e1->p1 == point) ? e1->p2 : e1->p1;
      point_t *p2 = (e2->p1 == point) ? e2->p2 : e2->p1;

      point->x = (p1->x + p2->x) / 2;
      point->y = (p1->y + p2->y) / 2;

    }

  }
  TAR_END_LOOP;



}

void 
laa(autorouter_t *ar, problem_t *prob) {

  int i,j;

  //compute_intersections(ar, prob);
  compute_lengths(ar, prob);

  int edges_n = prob->edges_n;
   
  for(i=0;i<edges_n;i++) {
    edge_t *edge = prob->edges[ i ];
    
    int denom = (2*(ar->via_clearance + ar->via_radius));
    int segs = MIN( ((int)edge->length)/denom, ar->max_assign_segments ) + 1;

    int *path = calloc(segs, sizeof(int));
    int *minpath = calloc(segs, sizeof(int));
    double minscore = -1.0f;
  
    for(j=0;j<segs;j++) {
      path[j] = -1;
      minpath[j] = -1;
    }
    
    find_best_path(ar, prob, edge, segs, 0, path, 0, minpath, &minscore, false);

    /* was there space for a via if needed? */
    if(segs == 1 && minpath[0] == -1) {

      /* try making a crosspoint a via */
      if(edge->p1->type == CROSSPOINT && edge->p2->layer != -1) {
        minpath[0] = edge->p2->layer;
        printf("Not enough space for VIA, made CROSSPOINT into VIA\n");
      }else if(edge->p2->type == CROSSPOINT && edge->p1->layer != -1) {
        minpath[0] = edge->p1->layer;
        printf("Not enough space for VIA, made CROSSPOINT into VIA\n");
      }else {

        printf("Not enough space for VIA, and couldn't find CROSSPOINT (bad)\n");
      }

    }else{
      /* check for other degenerate cases */
      for(j=0;j<segs;j++) {
        if(minpath[j] == -1) {
          printf("\nBAD PATH : \n");
          print_path(ar, minpath, segs);
          edge->flags |= EDGEFLAG_HILIGHT;
          printf("edge->p1->layer = %d edge->p2->layer = %d\n", 
              edge->p1->layer, edge->p2->layer);

          printf("\nlogged run:\n");
          find_best_path(ar, prob, edge, segs, 0, path, 0, minpath, &minscore, true);
          printf("\n\n");
          trace(ar, prob, "badpath", DRAW_PROB | DRAW_EDGES | DRAW_POINTS, -1);
          break;
        }
      }

    }
    assign_path(ar, prob, edge, segs, minpath);
#ifdef TRACE	
//    printf("seq num = %d\n", ar->traceseq);	
    printf("MINPATH cost = %f :  ", minscore);
    print_path(ar, minpath, segs);
#endif
    
    free(path);
    free(minpath);

    trace(ar, ar->root_problem, "laa", DRAW_EDGES | DRAW_POINTS, -1);

  }

}

void
laa_sort_edges(autorouter_t *ar, problem_t *prob) {

  int i;
  for(i=0;i<prob->edges_n;i++) {
    edge_t *e1 = prob->edges[i];
    
    if(e1->p1->x < e1->p2->x ||( e1->p1->x == e1->p2->x && e1->p1->y < e1->p2->y )) {

    }else{
      point_t *temp = e1->p1;
      e1->p1 = e1->p2;
      e1->p2 = temp;
    }
  }
}

double
ordered_pairwise_detour(point_t *b1, point_t *b2, point_t *a1, point_t *a2) {
  
  if( intersect(a1, a2, b1, b2) && !lines_joined(a1, a2, b1, b2) ) { 
  
    double d1=-1.0f, d2=-1.0f;

    if(b1->type == VIA || b1->type == PAD || b1->type == PIN ) {
      d1 = length(a1->x, a1->y, b1->x, b1->y) + 
        length(b1->x, b1->y, a2->x, a2->y);
    }

    if(b2->type == VIA || b2->type == PAD || b2->type == PIN ) {
      d2 = length(a1->x, a1->y, b2->x, b2->y) + 
        length(b2->x, b2->y, a2->x, a2->y);
    }

    if(d1 < 0 && d2 < 0) return -1.0f;
    else if(d1 < 0) return d2;
    else if(d2 < 0) return d1;

    return MIN(d1,d2);
  }

  return 0.0f;
}

bool
is_net_closed(autorouter_t *ar, problem_t *prob, edge_t *e) {
  ar->rcounter++;
  return (count_net_crosspoints(ar,prob,e,e->layer) >= 2);
}

double 
ordered_set_detour(autorouter_t *ar, problem_t *prob, edge_t *e, 
  edge_t **edges, int edges_n) 
{

  double r = 0.0f;

  int i;
  for(i=0;i<edges_n;i++) {
    edge_t *e2 = edges[i];

//    printf("e2 = %d,%d -> %d,%d\n", e2->p1->x, e2->p1->y,
//      e2->p2->x, e2->p2->y);

    if(e != e2) {
      double opd = ordered_pairwise_detour(e->p1, e->p2, e2->p1, e2->p2);
      if(opd >= 0) r += opd;
      else {
//        printf("warning: ordered_pairwise_detour returned < 0\n");
        return 999999999.0f;
      }
    }

  }
  return r;
}

void
nop(autorouter_t *ar, problem_t *prob, int layer) {

  int i,count = 0;

  for(i=0;i<prob->edges_n;i++) {
    edge_t *edge = prob->edges[i];
    if(edge->layer == layer) count++;
  }

  edge_t **edges = malloc(count * sizeof(edge_t*));
  int edges_n = 0;

  for(i=0;i<prob->edges_n;i++) {
    edge_t *edge = prob->edges[i];
    if(edge->layer == layer) {
      edges[edges_n++] = edge;    
    }
  }
  
  printf("\n\n*** NOP BEFORE: ***\n");
  for(i=0;i<edges_n;i++) {
    
      
    double osd = ordered_set_detour(ar,prob,edges[i],&edges[i],edges_n-i);

    printf("osd = %f\n", osd);
  }

  for(i=0;i<edges_n;i++) {
    
    double minscore = -1.0f;
    int minedge = -1;
    int j;
    for(j=i;j<edges_n;j++) {
      
      double osd = ordered_set_detour(ar,prob,edges[j],&edges[i],edges_n-i);
      
      //printf("osd = %f\n", osd);

      if(minscore < 0 || osd < minscore) {
        minscore = osd;
        minedge = j;
      }

    }
    //printf("minedge/minscore = %d/%f\n\n", minedge, minscore);

    if(i != minedge && minscore >= 0) {
  //    printf("swapping %d and %d\n\n", i, minedge);
      edge_t *temp = edges[i];
      edges[i] = edges[minedge];
      edges[minedge] = temp;
    }

  }

  printf("\n\n*** NOP AFTER ***\n");
  for(i=0;i<edges_n;i++) {
    double osd = ordered_set_detour(ar,prob,edges[i],&edges[i],edges_n-i);

    printf("osd = %f\n", osd);
  }
  
  //return;

  int closed_edges_n = 0;
  int open_edges_n = 0;

  for(i=0;i<edges_n;i++) {
    if(is_net_closed(ar,prob,edges[i])) closed_edges_n ++;
    else open_edges_n++;
  }
  
  edge_t **edges_planar = malloc(count * sizeof(edge_t*));
  
  int j=0,k=0;
  for(i=0;i<edges_n;i++) {
    if(is_net_closed(ar,prob,edges[i])) {
      edges_planar[open_edges_n+j++] = edges[i];
    }else{
      edges_planar[k++] = edges[i];
    }
  }
  
  for(i=0;i<edges_n;i++) {
    edges_planar[i]->order = i;
  }

//  printf("layer %d: closed = %d, open = %d\n", layer, closed_edges_n, open_edges_n);
  free(edges);
  free(edges_planar);
}



/*
 * Inserts data into the linked list. Does not insert duplicate pointers. 
 */
void
linklist_insert(linklist_t **ll, void *data) {
/*
  if(*ll == NULL) {
    linklist_t *new = malloc(sizeof(linklist_t));

    new->next = NULL;
    new->data = data;

    *ll = new;
    return;
  }

  if((*ll)->data == data) return;

  linklist_insert(&((*ll)->next), data);
*/
  linklist_t *new = malloc(sizeof(linklist_t));

  new->next = *ll;
  new->data = data;

  *ll = new;
}

void
linklist_edge_point_insert(linklist_t **ll, point_t *p1, point_t *newpoint) {
  if(*ll == NULL) {
    linklist_t *new = malloc(sizeof(linklist_t));

    new->next = NULL;
    new->data = newpoint;

    *ll = new;
    return;
  }

  if((*ll)->data == newpoint) return;

  point_t *p = (point_t *)(*ll)->data;
  if(length(p1->x, p1->y, p->x, p->y) > length(p1->x, p1->y, newpoint->x, newpoint->y)) {
    linklist_t *new = malloc(sizeof(linklist_t));

    new->next = *ll;
    new->data = newpoint;

    *ll = new;
    return;
  }

  linklist_edge_point_insert(&((*ll)->next), p1, newpoint);
  
}

/*
 * Inserts data onto the tail of the linklist. 
 */
void
linklist_tailinsert(linklist_t **ll, void *data) {
  if(*ll == NULL) {
    linklist_insert(ll, data);
    return;
  }
  if((*ll)->data == data) return;

  linklist_tailinsert(&((*ll)->next), data);
  
}

/*
 *  Removes a pointer from a linked list. 
 */
void
linklist_remove(linklist_t **ll, void *data) {

  if((*ll)->data == data) {
    *ll = (*ll)->next;
    return;
  }
  linklist_remove(&((*ll)->next), data);
}

/*
 * Frees all the memory allocated in a linked list.
 */
void
linklist_destroy(linklist_t *ll) {
  if(ll != NULL) {
    linklist_destroy(ll->next);
    free(ll);
  }
}

void
print_point_linklist(linklist_t *ll) {
  if(ll == NULL) {
    printf("\n");
    return;
  }

  point_t *point = (point_t *) ll->data;

  printf("%d,%d ", point->x, point->y);
  print_point_linklist(ll->next);

}


/*
 * This calculates the "g"-cost, or generated cost (the cost of the path
 * so far), for use in the A* path finding algorithm.
 */
double
g_cost(linklist_t *closelist, point_t *curpoint) {
  
  double r = 0;

  linklist_t *curll = closelist;

  while(curll != NULL) {
    point_t *point = (point_t *) curll->data;
    
    r += length(point->x, point->y, curpoint->x, curpoint->y);
    
    curpoint = point;

    curll = curll->next;
  }

  return r;
}

/*
 * This calculates the "h"-cost, or heuristic cost, for use in the
 * A* path finding algorithm. 
 */
double
h_cost(solution_t *soln, point_t *curpoint, point_t *dest) {
  
  if(curpoint == dest) return 0;

  double r = length(curpoint->x, curpoint->y, dest->x, dest->y);
  
  generic_pool_t *curpool = soln->edge_pools;
  while(curpool != NULL) {
    int i;
    for(i=0;i<curpool->data_n;i++) {
      edge_t *edge =  &((edge_t *) curpool->data)[i];

      if( intersect(edge->p1, edge->p2, curpoint, dest) && 
          !lines_joined(edge->p1, edge->p2, curpoint, dest) ) { 
        
        double det1 = length(curpoint->x, curpoint->y, edge->p1->x, edge->p1->y) + 
                      length(edge->p1->x, edge->p1->y, dest->x, dest->y);
        double det2 = length(curpoint->x, curpoint->y, edge->p2->x, edge->p2->y) + 
                      length(edge->p2->x, edge->p2->y, dest->x, dest->y);

        r += MIN(det1,det2);

      }
    }

    curpool = curpool->next;
  }
/*
  TAR_EDGES_LOOP(t);
  {
    if(edge->layer >= 0) {
      double temp = ordered_pairwise_detour(edge->p1, edge->p2, curpoint, dest);
      if(temp < 0) {
        printf("ERROR: ordered_pairwise detour < 0 (probably unroutable)\n");
        return temp;
      }
      r += temp;
    }
  }
  TAR_EDGES_ENDLOOP;
*/ 

  return r;
}

/*
 * This calculates the "f"-cost, which = g + h
 * so far), for use in the A* path finding algorithm.
 */
double
f_cost(linklist_t *closelist, solution_t *soln, triangulation_t *t, point_t *curpoint, point_t *dest) {
  double g = g_cost(closelist, curpoint);
  double h = h_cost(soln, curpoint, dest);
  double f = g + h;

//  printf("fcost: point %d,%d\t\tf=%f\tg=%f\th=%f\n", curpoint->x, curpoint->y, f, g, h);
  
  return f;
}

edge_t *
edge_search(triangulation_t *t, point_t *p1, point_t *p2) {
  
  TAR_EDGES_LOOP(t);
  {
    if(root_point(t, edge->p1) >= 0 && root_point(t, edge->p2) >= 0)
//      if(! (edge->flags & EDGEFLAG_CONSTRAINED)) { 
        if(edge->p1 == p1 && edge->p2 == p2) return edge;
        if(edge->p1 == p2 && edge->p2 == p1) return edge;
//    }
  }
  TAR_EDGES_ENDLOOP;

  return NULL;
}

/*
 * Finds the two edges _forward_ of curpoint and lastpoint
 */
linklist_t *
get_forward_edges(triangulation_t *t, point_t *curpoint, point_t *lastpoint) {

  linklist_t *r = NULL;
  double a = 0.0f;

  edge_t *paredge = curpoint->edge;

  TAR_EDGES_LOOP(t);
  {

    if(root_point(t, edge->p1) >= 0 && root_point(t, edge->p2) >= 0)
        
        if(paredge->p1 == edge->p1) {
          edge_t *e2 = edge_search(t, edge->p2, paredge->p2);
          if(e2 != NULL) {
            if( wind(paredge->p1, paredge->p2, edge->p2) != wind(paredge->p1, paredge->p2, lastpoint) ) {
              double temparea = area2D_triangle(paredge->p1, paredge->p2, edge->p2);
              if(r == NULL || temparea < a) {
                linklist_destroy(r);
                r = NULL;
                linklist_insert(&r, edge);
                linklist_insert(&r, e2);
                a = temparea;
              }
            }
          }
        }

        if(paredge->p1 == edge->p2) {
          edge_t *e2 = edge_search(t, edge->p1, paredge->p2);
          if(e2 != NULL) {
            if( wind(paredge->p1, paredge->p2, edge->p1) != wind(paredge->p1, paredge->p2, lastpoint) ) {
              double temparea = area2D_triangle(paredge->p1, paredge->p2, edge->p1);
              if(r == NULL || temparea < a) {
                linklist_destroy(r);
                r = NULL;
                linklist_insert(&r, edge);
                linklist_insert(&r, e2);
                a = temparea;
              }
            }
          }
        }

  }
  TAR_EDGES_ENDLOOP;

  return r;
}


/*
 * Creates a linklist containing all the edges oppisate to point p 
 * in the triangulation t.
 */
linklist_t *
get_oppisate_edges(triangulation_t *t, point_t *p) {
  
  linklist_t *r = NULL;

  /* first find all points adjacent to p in the triangulation */
  linklist_t *points = NULL;

  TAR_EDGES_LOOP(t);
  {
    if(edge->p1 == p)
      linklist_insert(&points, edge->p2);      
    else if(edge->p2 == p)
      linklist_insert(&points, edge->p1);
  }
  TAR_EDGES_ENDLOOP;

  /* check each permutation of these points to determine if there is an edge */
    
  linklist_t *curll = points;
  while(curll != NULL) {
    point_t *point1 = (point_t *) curll->data;

    linklist_t *curll2 = curll->next;
    while(curll2 != NULL) {
      point_t *point2 = (point_t *) curll2->data;
      
      TAR_EDGES_LOOP(t);
      {
        if(edge->p1 == point1 && edge->p2 == point2) {
          if(root_point(t, edge->p1) >= 0 && root_point(t, edge->p2) >= 0)
            if(! (edge->flags & EDGEFLAG_CONSTRAINED)) 
              linklist_insert(&r, edge);
        }else if(edge->p1 == point2 && edge->p2 == point1) { 
          if(root_point(t, edge->p1) >= 0 && root_point(t, edge->p2) >= 0)
            if(! (edge->flags & EDGEFLAG_CONSTRAINED)) 
              linklist_insert(&r, edge);
        }
      }
      TAR_EDGES_ENDLOOP;

      curll2 = curll2->next;
    }

    curll = curll->next;
  }
  
  linklist_destroy(points);

  return r;

}

/*
 * checks for a collison between the line defined by points p1 and p2
 * and any edges in the solution
 */
int
check_collison(solution_t *soln, point_t *p1, point_t *p2) {
  
  generic_pool_t *curpool = soln->edge_pools;
  while(curpool != NULL) {
    int i;
    for(i=0;i<curpool->data_n;i++) {
      edge_t *edge =  &((edge_t *) curpool->data)[i];
      if(!(edge->flags & EDGEFLAG_INACTIVE))
      if(intersect(edge->p1, edge->p2, p1, p2) && !lines_joined(edge->p1, edge->p2, p1, p2) )
        return 1;
    }

    curpool = curpool->next;
  }

  return 0;
}

/*
 * returns true if an edge (defined by point coordinates) is in 
 * a solution
 */
int
check_soln_for_edge(solution_t *soln, edge_t *e) {

  generic_pool_t *curpool = soln->edge_pools;
  while(curpool != NULL) {
    int i;
    for(i=0;i<curpool->data_n;i++) {
      edge_t *edge =  &((edge_t *) curpool->data)[i];
      if(!(edge->flags & EDGEFLAG_INACTIVE)) {
        if(edge->p1 == e->p1 && edge->p2 == e->p2) return 1;
        if(edge->p1 == e->p2 && edge->p2 == e->p2) return 1;
      }
    }

    curpool = curpool->next;
  }

  return 0;
}

void
consider_point_between_int(solution_t *soln, linklist_t **closelist, linklist_t **openlist, 
    point_t *curpoint, point_t *p1, point_t *p2, edge_t *edge, point_t *newpoint) {

  /* check for collison in solution */
  if(check_collison(soln, curpoint, newpoint)==1) {
//    printf("collision...\n");
    free(newpoint);
  }else{
    /* check its not on the close list already */
    linklist_t *llp2 = *closelist;
    while(llp2 != NULL) {
      point_t *cl_point = (point_t *) llp2->data;
      if(cl_point->x == newpoint->x && cl_point->y == newpoint->y) {
        /* it is, so ignore it */
//        printf("on closelist already..\n");
        free(newpoint);
        return; 
      }

      llp2 = llp2->next;
    }

    /* check if its on the open list already */
    llp2 = *openlist;
    while(llp2 != NULL) {
      point_t *ol_point = (point_t *) llp2->data;
      if(ol_point->x == newpoint->x && ol_point->y == newpoint->y) {
        /* it is, see if newpoint g-cost is lower */
//        printf("on openlist already...\n");
        if(g_cost(*closelist, newpoint) < g_cost(*closelist, ol_point)) {
          ol_point->parent = curpoint;
        }
        free(newpoint);
        return;
      }
      llp2 = llp2->next;
    }
    /* it isn't, so add it to openlist */
//    printf("adding to openlist...\n");
    linklist_insert(openlist, newpoint);
    newpoint->parent = curpoint;

  }

}

void
consider_point_between(solution_t *soln, linklist_t **closelist, linklist_t **openlist, 
    point_t *curpoint, point_t *p1, point_t *p2, edge_t *edge) {

//  printf("considering point between %d,%d and %d,%d\n", p1->x, p1->y, p2->x, p2->y);

  point_t *np1 = calloc(1, sizeof(point_t));
  point_t *np2 = calloc(1, sizeof(point_t));
  np1->type = TCSPOINT;
  np2->type = TCSPOINT;
  np1->edge = edge;
  np2->edge = edge;

  np1->x = p1->x; np1->y = p1->y;
  np2->x = p2->x; np2->y = p2->y;

  double sintheta = (double) abs(p1->y - p2->y) / length(p1->x, p1->y, p2->x, p2->y);
  double costheta = (double) abs(p1->x - p2->x) / length(p1->x, p1->y, p2->x, p2->y);

  /* TODO: 500, the radius from the nearest point, should be determined from 
   * the netclass matrix
   */

  int diffx = 500 * costheta;
  int diffy = 500 * sintheta;

  /* quadrant I */
  if(p2->x >= p1->x && p2->y >= p1->y) {
  
    np1->x += diffx;
    np1->y += diffy;
    np2->x -= diffx;
    np2->y -= diffy;
  
  /* quadrant II */
  }else if(p2->x < p1->x && p2->y >= p1->y) { 
  
    np1->x -= diffx;
    np1->y += diffy;
    np2->x += diffx;
    np2->y -= diffy;
  
  /* quadrant III */
  }else if(p2->x < p1->x && p2->y < p1->y) {

    np1->x -= diffx;
    np1->y -= diffy;
    np2->x += diffx;
    np2->y += diffy;

  /* quadrant IV */
  }else{

    np1->x += diffx;
    np1->y -= diffy;
    np2->x -= diffx;
    np2->y += diffy;
  
  }

  consider_point_between_int(soln, closelist, openlist, curpoint, p1, p2, edge, np1);
  consider_point_between_int(soln, closelist, openlist, curpoint, p1, p2, edge, np2);

}

void
balance_edges(triangulation_t *t) {
  
  TAR_EDGES_LOOP(t);
  {
    
    int count = 0;

    linklist_t *curll = edge->tcs_points;
    while(curll != NULL) {
      count++;
      curll = curll->next;
    }
    
    int xint = (edge->p2->x - edge->p1->x) / (count + 1);
    int yint = (edge->p2->y - edge->p1->y) / (count + 1);

    count = 1;
    curll = edge->tcs_points;
    while(curll != NULL) {
      point_t *point = (point_t *) curll->data;
      point->x = edge->p1->x + (count * xint);
      point->y = edge->p1->y + (count * yint);
      count++;
      curll = curll->next;
    }
  }
  TAR_EDGES_ENDLOOP;

}


int
edge_flow(autorouter_t *ar, edge_t *edge, int additional_traces) {
  
  int flow = 0;
  /* TODO: use clearance radius from edge points */

  int point_radius           = ar->default_viathickness;
  int point_clearance        = ar->default_keepaway;

  int trace_width            = ar->default_linethickness;
  int trace_clearance        = ar->default_keepaway;

  flow += point_radius;

  int prev_clearance = point_clearance;

  linklist_t *curll = edge->tcs_points;
  while(curll != NULL) {
    //point_t *p = (point_t *) curll->data;
   
    flow += MAX(prev_clearance, trace_clearance);
    flow += trace_width;

    prev_clearance = trace_clearance;

    curll = curll->next;
  }
 
  int i;
  for(i=0;i<additional_traces;i++) {
    flow += MAX(prev_clearance, trace_clearance);
    flow += trace_width;
    prev_clearance = trace_clearance;
  }

  flow += MAX(prev_clearance, point_clearance);
  flow += point_radius;

  return flow;
}

void
route_edge(autorouter_t *ar, problem_t *prob, edge_t *e, int layer) {
  
  solution_t *soln = &prob->solns[layer];
  triangulation_t *t = &prob->ts[layer];

  printf("route_edge: %d,%d-%d,%d order %d\n", e->p1->x, e->p1->y, 
    e->p2->x, e->p2->y, e->order);
    
  /* first look for direct edge */
  TAR_EDGES_LOOP(t);
  {

    if(edge->layer >= 0) continue;
    if( (edge->p2 == e->p1 && edge->p1 == e->p2) || (edge->p1 == e->p1 && edge->p2 == e->p2) ) {
      if(check_collison(soln, e->p1, e->p2) == 0) {
        edge_t *newedge = (edge_t *) alloc_generic(&soln->edge_pools, &soln->pool_edges_n, sizeof(edge_t));
        memcpy(newedge, edge, sizeof(edge_t));
        linklist_tailinsert(&e->segments, newedge);
        return;
      }
    }

  }
  TAR_EDGES_ENDLOOP;
 
  /* there is no direct edge, use A* search */

  linklist_t *openlist = NULL;
  linklist_t *closelist = NULL;
  point_t *curpoint = NULL;

  /* put the starting point on the open list */
  linklist_insert(&openlist, e->p1);   
  e->p1->parent = NULL;
  
  linklist_t *curll;
  
  bool loopcond = true;
  while(loopcond) {
    
    /* look for lowest F-cost point on open list */
    double min_fcost = 0.0f;
    point_t *min_fcost_p = NULL;

    int openlistcount = 0;

    curll = openlist;
    while(curll != NULL) {
      point_t *p = (point_t *) curll->data;
      double p_fcost = f_cost(closelist, soln, t, p, e->p2);
      
      if(min_fcost_p == NULL || p_fcost < min_fcost) {
        min_fcost = p_fcost;
        min_fcost_p = p;
      }
      openlistcount++;
      curll = curll->next;
    }

    if(openlistcount == 0) {
      ar->bad_routes++;
      e->flags |= EDGEFLAG_NOTROUTED;

			printf("Openlist count == 0.. no path? \n");
      //exit(0);
      return;
      //goto path_found;
    }
    
//    printf("min fcost point @ %d,%d, fcost = %f\n", 
//      min_fcost_p->x, min_fcost_p->y, min_fcost);

    /* switch min F-cost point to close list */ 
    linklist_remove(&openlist, min_fcost_p);
    linklist_insert(&closelist, min_fcost_p);
//    min_fcost_p->parent = curpoint;

    curpoint = min_fcost_p;

    if(curpoint == e->p2) goto path_found;

    linklist_t *edgecands = NULL;

    /* is this the initial point? */
    if(curpoint == e->p1) {
      edgecands = get_oppisate_edges(t, curpoint);

    /* otherwise it must be a TCS point on an edge */
    }else{
      edgecands = get_forward_edges(t, curpoint, curpoint->parent);

      int count = 0;

      curll = edgecands;
      while(curll != NULL) {
        edge_t *edge = (edge_t *) curll->data;

        /* see if the target point is available */
        if(edge->p1 == e->p2 || edge->p2 == e->p2) {
//          printf("target point @ %d,%d visible... \n", e->p2->x, e->p2->y);
          if(check_collison(soln, e->p2, curpoint) == 0) {
            linklist_insert(&openlist, e->p2);
            e->p2->parent = curpoint;
            break;
          }//else printf(". . . but there is a collison\n");
        }
        count++;
        curll = curll->next;
      }
//      printf("got %d forward edges..\n", count);
      
    }
    
    TAR_EDGES_LOOP(t);
    {
      edge->flags &= ~EDGEFLAG_HILIGHT;
    }
    TAR_EDGES_ENDLOOP;

    curll = edgecands;
    while(curll != NULL) {
      edge_t *edge = (edge_t *) curll->data;
     
      edge->flags |= EDGEFLAG_HILIGHT;

      if(!check_soln_for_edge(soln, edge) && !(edge->flags & EDGEFLAG_CONSTRAINED) ) {

//        printf("considering edge...\n"); 
        
        /* would the edge flow exceed the capacity? 
         * (is there enough room on the edge for another net?)
         */
        int edgeflow = edge_flow(ar, edge, 1);
        int edgecap = length(edge->p1->x, edge->p1->y, edge->p2->x, edge->p2->y);

        if(edgeflow >= edgecap) {
          printf("\t edge capacity %d exceeded by flow %d\n", edgecap, edgeflow);
          
        }else {
          point_t *lastpoint = edge->p1;

          linklist_t *tcs_ll = edge->tcs_points;
          while(tcs_ll != NULL) {
            point_t *tcs_point = (point_t *) tcs_ll->data;

            consider_point_between(soln, &closelist, &openlist, curpoint, lastpoint, tcs_point, edge);

            lastpoint = tcs_point;
            tcs_ll = tcs_ll->next;
          }
          consider_point_between(soln, &closelist, &openlist, curpoint, lastpoint, edge->p2, edge);
        }
      }
      curll = curll->next;
    }
    linklist_destroy(edgecands);
/* 
  printf("Openlist: \n");
  curll = openlist;
  while(curll != NULL) {
    point_t *p = (point_t *) curll->data;
    double p_fcost = f_cost(closelist, soln, t, p, e->p2);
    double p_gcost = g_cost(closelist, p);
    double p_hcost = h_cost(soln, p, e->p2);
    printf("\t%d,%d - %f - %f - %f\n", p->x, p->y, p_fcost, p_gcost, p_hcost);
    curll = curll->next;
  }
  printf("Closelist: \n");
  curll = closelist;
  while(curll != NULL) {
    point_t *p = (point_t *) curll->data;
    double p_fcost = f_cost(closelist, soln, t, p, e->p2);
    printf("\t%d,%d - %f\n", p->x, p->y, p_fcost);
    curll = curll->next;
  }
    
*/
    //prob->closelist = closelist;
    //prob->openlist = openlist;
    //trace(ar, prob, "list", DRAW_CLOSELIST | DRAW_POINTS | DRAW_TRIAGS | DRAW_PROB | DRAW_SOLN_EDGES, layer);
    //if(ar->traceseq > 546) exit(0);
  }

path_found:
  printf("Path found.. setting..\n");
//  printf("\ne = %d,%d %d,%d\n\n", e->p1->x, e->p1->y, e->p2->x, e->p2->y);

  point_t *cp = curpoint;//e->p2;
  while(cp != NULL) {
    point_t *cp_p = cp->parent;

    if(cp->type == TCSPOINT) {

      linklist_edge_point_insert(&(cp->edge->tcs_points), cp->edge->p1, cp);

    }

//    printf("cp = %d,%d \t ", cp->x, cp->y);
    if(cp_p != NULL) {
      edge_t *newedge = (edge_t *) alloc_generic(&soln->edge_pools, &soln->pool_edges_n, sizeof(edge_t));
      newedge->p1 = cp;
      newedge->p2 = cp_p;
//      printf("cpp = %d,%d", cp_p->x, cp_p->y);
      linklist_tailinsert(&e->segments, newedge);
    }
//    printf("\n");
    cp = cp_p;
  }
  
  balance_edges(t);

  TAR_EDGES_LOOP(t);
  {
    edge->flags &= ~EDGEFLAG_HILIGHT;
  }
  TAR_EDGES_ENDLOOP;

  e->flags &= ~EDGEFLAG_NOTROUTED;

  return;
}

edge_t *
find_other_tcs_edge(solution_t *soln, point_t *point, edge_t *otheredge) {
  
  generic_pool_t *curpool = soln->edge_pools;
  while(curpool != NULL) {
    int i;
    for(i=0;i<curpool->data_n;i++) {
      edge_t *edge =  &((edge_t *) curpool->data)[i];

      if((edge->p1 == point || edge->p2 == point) && edge != otheredge) {
        return edge;
      }
    }
    curpool = curpool->next;
  }
  
  printf("Reached end of find_other_edge.. couldn't find the edge... \n");
  return NULL;
}

void
find_tcs_edges(solution_t *soln, point_t *point, edge_t **e1, edge_t **e2) {
  
  generic_pool_t *curpool = soln->edge_pools;
  while(curpool != NULL) {
    int i;
    for(i=0;i<curpool->data_n;i++) {
      edge_t *edge =  &((edge_t *) curpool->data)[i];

      if(edge->p1 == point || edge->p2 == point) {
        if(*e1 == NULL) {
          *e1 = edge;
        } else {
          *e2 = edge;
          return;
        }
      }
    }
    curpool = curpool->next;
  }
  
  printf("Reached end of find edges.. couldn't find the edge... \n");

}

double
get_rubberband_radius(autorouter_t *ar, point_t *point) {
  if(point->pull_point == NULL) return 6000.0f;

  double len_to_pullpoint = length(point->x, point->y, point->pull_point->x, point->pull_point->y);
  int order = 0;

  linklist_t *curll = point->edge->tcs_points;
  while(curll != NULL) {
    point_t *curp = (point_t *) curll->data;

    if(curp->pull_point == point->pull_point) {
      if(length(curp->x, curp->y, curp->pull_point->x, curp->pull_point->y) < len_to_pullpoint)
        order++;
  
    }

    curll = curll->next;
  }

  /* order now contains the number of arcs closer */

  double r = 0.0f;
  //printf("point->thickness = %d\n", point->thickness);
  if(point->pull_point->type != INTERSECT) { 
    r += 12000 / 2; // TODO: change back to actual thickness 
    r += 2 * ar->default_keepaway;
    r += ar->default_linethickness / 2;
  }
  int i;
  for(i=0;i<order;i++) {
    r += 2 * ar->default_keepaway;
    r += ar->default_linethickness;
  }
  return r;
}

point_t *
get_next_arc_or_term(linklist_t *ll) {

  linklist_t *curll = ll;
  while(curll != NULL) {
    edge_t *e = (edge_t *) curll->data;
//    printf("looking for next term\n");
    if(e->p2->type == TCSPOINT && e->p2->pull_point->type != INTERSECT) 
      return e->p2;
    else if(e->p2->type != TCSPOINT) 
      return e->p2;

    curll = curll->next;
  }

  return NULL;
}

/*
 * given point a, gradient m, and radius r, 
 *
 * find the 2 points along that line r away from a
 */ 
void
find_points_on_line(point_t *a, double m, double r, point_t *b0, point_t *b1) {
  if(m == INFINITY) {
    b0->y = a->y + r;
    b1->y = a->y - r;

    b0->x = a->x;
    b1->x = a->x;
    return;
  }

  double c = (double)a->y - (m * (double)a->x);

  double temp = sqrt( pow(r, 2) / ( 1 + pow(m, 2) ) );

  b0->x = a->x + (int) temp;
  b1->x = a->x - (int) temp;
  
  b0->y = (int) (((double) b0->x) * m + c);
  b1->y = (int) (((double) b1->x) * m + c);

}

double
gradient(point_t *a, point_t *b) {
  if(a->x == b->x) {
    printf("returning NAN\n");
    return INFINITY;
  }

//  point_t *c = (a->x > b->x) ? b : a;
//  point_t *d = (a->x > b->x) ? a : b;

  return (double)(b->y - a->y) / (double)(b->x - a->x);
}

void
spring_embedder_tcs(autorouter_t *ar, problem_t *prob, int layer) {
  triangulation_t *t = &prob->ts[layer];

  int m;
  for(m=0;m<100;m++) {
    
    TAR_EDGES_LOOP(t);
    {

      /* calculate the force on each vertex */ 
      point_t *prevpoint = edge->p1;

      linklist_t *curll = edge->tcs_points;
      while(curll != NULL) {
        point_t *point = (point_t *) curll->data;

        point_t *nextpoint = NULL;
        if(curll->next != NULL) {
          nextpoint = (point_t *) curll->next->data;
        }else{
          nextpoint = edge->p2;
        }

        /* find the 2 edges connected to this point */ 
        edge_t *e1 = NULL, *e2 = NULL;
        find_tcs_edges(&prob->solns[layer], point, &e1, &e2);

        /* get the points on the other ends of the edges */
        point_t *p1 = (e1->p1 == point) ? e1->p2 : e1->p1;
        point_t *p2 = (e2->p1 == point) ? e2->p2 : e2->p1;

        //point_t *pull_point = NULL;
        /* move the vertex 0.1 times force */
        
        if(intersect(p1, p2, edge->p1, edge->p2)) {
          //if(point->pull_point->type == INTERSECT) free(point->pullpoint);

          point->pull_point = calloc(1, sizeof(point_t));
          intersect_point(p1, p2, edge->p1, edge->p2, point->pull_point);

          double len1 = length(point->pull_point->x, point->pull_point->y, edge->p1->x, edge->p1->y);
          double len2 = length(point->pull_point->x, point->pull_point->y, edge->p2->x, edge->p2->y);
          double rad1 = get_rubberband_radius(ar, prevpoint); 
          double rad2 = get_rubberband_radius(ar, nextpoint); 

          if(len1 < rad1) {
            if(point->pull_point->type == INTERSECT) free(point->pull_point);
            point->pull_point = edge->p1;
          }else if(len2 < rad2){ 
            if(point->pull_point->type == INTERSECT) free(point->pull_point);
            point->pull_point = edge->p2;
          }
        }else{
          double p1len = length(p1->x, p1->y, edge->p1->x, edge->p1->y) +
            length(p2->x, p2->y, edge->p1->x, edge->p1->y);

          double p2len = length(p1->x, p1->y, edge->p2->x, edge->p2->y) +
            length(p2->x, p2->y, edge->p2->x, edge->p2->y);


          if(p1len < p2len) {
            /* pull towards edge->p1 */
            point->pull_point = edge->p1;
          }else if(p2len < p1len) {
            /* pull towards edge->p2 */
            point->pull_point = edge->p2;
          }else printf("p1len == p2len... this shouldn't happen\n");

        }

      if(point->pull_point == NULL) {
        printf("pull_point == NULL... ERROR\n");
      }else{
        double mult = (point->pull_point->type == INTERSECT) ? 1.0 : 0.1;
        int tempx = point->x + (mult * (point->pull_point->x - point->x));
        int tempy = point->y + (mult * (point->pull_point->y - point->y));
        
        int spacingprev = ((point->type == TCSPOINT) ? ar->default_linethickness : point->thickness ) / 2;
        spacingprev += ar->default_keepaway;
        spacingprev += ((prevpoint->type == TCSPOINT) ? ar->default_linethickness : prevpoint->thickness ) / 2;
        
        int spacingnext = ((point->type == TCSPOINT) ? ar->default_linethickness : point->thickness ) / 2;
        spacingnext += ar->default_keepaway;
        spacingnext += ((nextpoint->type == TCSPOINT) ? ar->default_linethickness : nextpoint->thickness ) / 2;

        /* find the prev point and next points which are moved in by the prev and next spacings */
        point_t prevpointbound, nextpointbound;
        
        point_t tp[2];
        double m = gradient(edge->p1, edge->p2);
        find_points_on_line(prevpoint, m, spacingprev, &tp[0], &tp[1]);
        
        if(length(nextpoint->x, nextpoint->y, tp[0].x, tp[0].y) < length(nextpoint->x, nextpoint->y, tp[1].x, tp[1].y)) {
          prevpointbound.x = tp[0].x; prevpointbound.y = tp[0].y;
        }else{
          prevpointbound.x = tp[1].x; prevpointbound.y = tp[1].y;
        }
        
        find_points_on_line(nextpoint, m, spacingnext, &tp[0], &tp[1]);
        
        if(length(prevpoint->x, prevpoint->y, tp[0].x, tp[0].y) < length(prevpoint->x, prevpoint->y, tp[1].x, tp[1].y)) {
          nextpointbound.x = tp[0].x; nextpointbound.y = tp[0].y;
        }else{
          nextpointbound.x = tp[1].x; nextpointbound.y = tp[1].y;
        }
       
        double len_temp_prev = length(tempx, tempy, prevpointbound.x, prevpointbound.y);
        double len_temp_next = length(tempx, tempy, nextpointbound.x, nextpointbound.y);
        double len_prev_next = length(prevpointbound.x, prevpointbound.y, nextpointbound.x, nextpointbound.y);

        /* find direction */
        if(len_temp_prev < len_temp_next) {
          // closer to prevpoint
          if(len_prev_next < len_temp_next) {
            //its outside, so make it prev bound point
            tempx = prevpointbound.x; tempy = prevpointbound.y;
          }else{
            //its inside (sweet)
          }

        } if(len_temp_prev > len_temp_next) {
          //closer to nextpoint
          if(len_prev_next < len_temp_prev) {
            //its outside, so make it next bound point
            tempx = nextpointbound.x; tempy = nextpointbound.y;
          }else{
            //its inside(sweet)
          }
       
        }else{
          //its equidistant to next and prev (sweet)
        }
       
        if(length(nextpoint->x, nextpoint->y, prevpoint->x, prevpoint->y) < (spacingprev + spacingnext)) {
//          printf("ERROR: tcs spring embedder: point can't fit..(it shouldn't have got in here..)\n");
          
        }else{

          point->x = tempx;
          point->y = tempy;
        }

      }

      if(m % 10 == 0) trace(ar, prob, "spring", DRAW_POINTS | DRAW_TRIAGS | DRAW_PROB | DRAW_SOLN_EDGES, layer);
      //if(pull_point->type == INTERSECT) free(pull_point);
      prevpoint = point;

      curll = curll->next;
      }




    }
    TAR_EDGES_ENDLOOP;

  }

}



void
route_edges(autorouter_t *ar, problem_t *prob, int layer) {

  //triangulation_t *t = &prob->ts[layer];
  int layer_edge_count = 0;

  TAR_EDGES_LOOP(prob);
  {
    if(edge->layer == layer) layer_edge_count++;
  }
  TAR_EDGES_ENDLOOP;	

  printf("There are %d edges to route on layer %d\n", layer_edge_count, layer);

  int curedge = 0;

  /*  route the edges in the order determined by the NOP */
  while(curedge < layer_edge_count) {
    TAR_EDGES_LOOP(prob);
    {
      if(edge->layer == layer)
        if(edge->order == curedge) {
          printf("\n");
          route_edge(ar, prob, edge, layer);
          break;
        }
    }
    TAR_EDGES_ENDLOOP;

    curedge++;
  }
  

}

/*
 * Creates a triangulation out of the points contained in a linked list.
 */
triangulation_t *
triangulate_linklist(autorouter_t *ar, linklist_t *ll) {
 
  triangulation_t *t = calloc(1, sizeof(triangulation_t));
  triangulation_create(MAX_BOARD_DIM(ar), t);

  linklist_t *curll = ll;

  while(curll != NULL) {
    point_t *curpoint = (point_t *) curll->data;
    triangulation_insert_point(t, curpoint);
    curll = curll->next;
  }
  
  return t; 
}


void
insert_edge_into_triangulation(autorouter_t *ar, edge_t *iedge, triangulation_t *target) {
  
  TAR_EDGES_LOOP(target);
  {
    if( (edge->p1 == iedge->p1 && edge->p2 == iedge->p2) ||
        (edge->p1 == iedge->p2 && edge->p2 == iedge->p1) ) {
      
/*      if(edge->flags & EDGEFLAG_INACTIVE) {
        edge->flags ^= EDGEFLAG_INACTIVE;
      }
      */
      edge->layer = iedge->layer;
      edge->flags = iedge->flags;
      edge->order = iedge->order;
      return;

    }

  }
  TAR_EDGES_ENDLOOP;
  
  TAR_EDGES_LOOP(target);
  {
    if(edge->flags & EDGEFLAG_INACTIVE) {
      memcpy(edge, iedge, sizeof(edge_t));
      return;
    }
  }
  TAR_EDGES_ENDLOOP;

  edge_t *newedge = triangulation_alloc_edge(target);
  memcpy(newedge, iedge, sizeof(edge_t));
  return;
}

/* 
 * Constrains an edge in a triangulation. Note, this messes with the edge 
 * structures which are maintained during triangulation, so triangulation 
 * must be done prior to this, and new points cannot be inserted after 
 * using this function.
 *
 * 1) determine the intersections between the triangulation, and the
 *    problem edges 
 * 2) each side of the edge is re-delaunay triangulated with the point from
 *    an intersecting edge 
 * 3) triangulation minus intersecting edges is unioned with the second
 *    triangulation and the constrained edges 
 */   
void
constrain_edge(autorouter_t *ar, problem_t *prob, triangulation_t *t, edge_t *cedge) {

  linklist_t *side1 = NULL;
  linklist_t *side2 = NULL;

  TAR_EDGES_LOOP(t);
  {
    
    /* 
     * we avoid the chicken and egg problem of two constrained edges 
     * by making sure the edge doesn't have a layer assigned 
     *
     */

    if((edge->flags & EDGEFLAG_CONSTRAINED) == 0)  
    if(root_point(t, edge->p1) >= 0 && root_point(t, edge->p2) >= 0)
      if(intersect(edge->p1, edge->p2, cedge->p1, cedge->p2) && 
          !lines_joined(edge->p1, edge->p2, cedge->p1, cedge->p2) ){
        
        if(wind(cedge->p1, cedge->p2, edge->p1) == 1) {
          linklist_insert(&side1, (void *) edge->p1);
          linklist_insert(&side2, (void *) edge->p2);
        }else{
          linklist_insert(&side1, (void *) edge->p2);
          linklist_insert(&side2, (void *) edge->p1);
        }
        
        edge->flags |= EDGEFLAG_INACTIVE; 

        cedge->flags |= EDGEFLAG_HILIGHT;
      }

  }
  TAR_EDGES_ENDLOOP;

  if(side1 != NULL) {
    linklist_insert(&side1, (void *) cedge->p1);
    linklist_insert(&side1, (void *) cedge->p2);
    linklist_insert(&side2, (void *) cedge->p1);
    linklist_insert(&side2, (void *) cedge->p2);
    triangulation_t *side1_t = triangulate_linklist(ar, side1);
    triangulation_t *side2_t = triangulate_linklist(ar, side2);
/*
    c = create_problem_drawing_context(ar,prob);
    c->prefix = "side1"; 
    draw_triag(ar, c, side1_t);
    finish_drawing(ar,c);
    
    c = create_problem_drawing_context(ar,prob);
    c->prefix = "side2"; 
    draw_triag(ar, c, side2_t);
    finish_drawing(ar,c);
*/
    TAR_EDGES_LOOP(side1_t);
    {
      if(root_point(side1_t, edge->p1) >= 0 && root_point(side1_t, edge->p2) >= 0)
        insert_edge_into_triangulation(ar, edge, t); 
    }
    TAR_EDGES_ENDLOOP;

    TAR_EDGES_LOOP(side2_t);
    {
      if(root_point(side2_t, edge->p1) >= 0 && root_point(side2_t, edge->p2) >= 0)
        insert_edge_into_triangulation(ar, edge, t); 
    }
    TAR_EDGES_ENDLOOP;

    triangulation_destroy( side1_t ); 
    triangulation_destroy( side2_t ); 
    free( side1_t );
    free( side2_t );
  }

  linklist_destroy(side1);
  linklist_destroy(side2);

}

void
local_router(autorouter_t *ar, problem_t *prob) {
  printf("Local router starting...\n");
  problem_destroy_ts(prob);

  prob->ts = calloc(ar->board_layers, sizeof(triangulation_t));
  prob->ts_n = ar->board_layers;

  int i;
  for(i=0;i<ar->board_layers;i++) {
    triangulation_create(MAX_BOARD_DIM(ar), &prob->ts[i]);
  }

  prob->spoints = calloc(4, sizeof(point_t));

  prob->spoints[2].x = prob->x2; prob->spoints[2].y = prob->y2;
  prob->spoints[3].x = prob->x1; prob->spoints[3].y = prob->y2;
  prob->spoints[0].x = prob->x1; prob->spoints[0].y = prob->y1;
  prob->spoints[1].x = prob->x2; prob->spoints[1].y = prob->y1;

  for(i=0;i<4;i++) {
    prob->spoints[i].type = CORNER;
    *problem_alloc_point(prob) = &prob->spoints[i];
  }

  prob->solns = calloc(ar->board_layers, sizeof(solution_t));
  for(i=0;i<ar->board_layers;i++) {
    solution_t *soln = &prob->solns[i];
    soln->edge_pools = alloc_generic_pool(sizeof(edge_t));
    soln->pool_edges_n = 0;
//    printf("allocated solution\n");
  }

  for(i=0;i<ar->board_layers;i++) {
    
    triangulation_t *curtriag = &prob->ts[i];

    TAR_PROB_POINTS_LOOP(prob);
    {

      if(point->layer == i || point->type == PIN || point->type == VIA || point->type == CORNER) { 

        triangulation_insert_point(curtriag, point);

      }
    }
    TAR_END_LOOP;
  
  
    /* make sure edges in the triangulation are layer -1 */
    /* also flag board edges as constrained */
    TAR_EDGES_LOOP(curtriag);
    {
      edge->layer = -1;
      edge->tcs_points = NULL;

      /* is edge vertical ? */
      if(edge->p1->x == edge->p2->x) {
        if(edge->p1->x == 0 || edge->p1->x ==ar->board_width)
          edge->flags |= EDGEFLAG_CONSTRAINED;
      }
      /* is edge horz? */
      else if(edge->p1->y == edge->p2->y) {
        if(edge->p1->y == 0 || edge->p1->y == ar->board_height)
          edge->flags |= EDGEFLAG_CONSTRAINED;
      }

    }
    TAR_EDGES_ENDLOOP;
    
    trace(ar, prob, "preroute", DRAW_POINTS | DRAW_TRIAGS | DRAW_PROB | DRAW_EDGES | DRAW_ORDERS, i);
  
    route_edges(ar, prob, i);
    
    trace(ar, prob, "postroute", DRAW_POINTS | DRAW_TRIAGS | DRAW_PROB | DRAW_SOLN_EDGES, i);
    
    spring_embedder_tcs(ar, prob, i);

    trace(ar, prob, "postoptimize", DRAW_POINTS | DRAW_TRIAGS | DRAW_PROB | DRAW_SOLN_EDGES, i);
  }

  //exit(0);
  //if(ar->traceseq >= 1571) exit(0);
}

point_t *
find_arc_last_point(solution_t *soln, point_t *pp, point_t *curpoint, point_t *nextpoint) {
  
  generic_pool_t *curpool = soln->edge_pools;
  while(curpool != NULL) {
    int i;
    for(i=0;i<curpool->data_n;i++) {
      edge_t *edge =  &((edge_t *) curpool->data)[i];

      if(edge->p1 == nextpoint && edge->p2 != curpoint) {
        if(edge->p2->pull_point != pp) return edge->p1;
        return find_arc_last_point(soln, pp, edge->p1, edge->p2);
      }else if(edge->p2 == nextpoint && edge->p1 != curpoint) {
        if(edge->p1->pull_point != pp) return edge->p2;
        return find_arc_last_point(soln, pp, edge->p2, edge->p1);
      }

    }
    curpool = curpool->next;
  }
  
  printf("Reached end of find_arc_end_point.. couldn't find the edge... \n");
  return NULL;
}

point_t *
find_arc_next_point(solution_t *soln, point_t *pp, point_t *curpoint, point_t *nextpoint) {
  
  generic_pool_t *curpool = soln->edge_pools;
  while(curpool != NULL) {
    int i;
    for(i=0;i<curpool->data_n;i++) {
      edge_t *edge =  &((edge_t *) curpool->data)[i];

      if(edge->p1 == nextpoint && edge->p2 != curpoint) {
        if(edge->p2->pull_point != pp) return edge->p2;
        return find_arc_next_point(soln, pp, edge->p1, edge->p2);
      }else if(edge->p2 == nextpoint && edge->p1 != curpoint) {
        if(edge->p1->pull_point != pp) return edge->p1;
        return find_arc_next_point(soln, pp, edge->p2, edge->p1);
      }

    }
    curpool = curpool->next;
  }
  
  printf("Reached end of find_arc_end_point.. couldn't find the edge... \n");
  return NULL;
}

point_t *
find_next_point(solution_t *soln, point_t *curpoint, point_t *nextpoint) {
  
  generic_pool_t *curpool = soln->edge_pools;
  while(curpool != NULL) {
    int i;
    for(i=0;i<curpool->data_n;i++) {
      edge_t *edge =  &((edge_t *) curpool->data)[i];

      if(edge->p1 == nextpoint && edge->p2 != curpoint) {
        return edge->p2;
      }else if(edge->p2 == nextpoint && edge->p1 != curpoint) {
        return edge->p1;
      }

    }
    curpool = curpool->next;
  }
  
  printf("Reached end of find_arc_end_point.. couldn't find the edge... \n");
  return NULL;
}

void
export_pcb_data_drawline(autorouter_t *ar, int layer, int x0, int y0, int x1, int y1) {

#ifndef TEST 
  LineTypePtr line;
  line = CreateDrawnLineOnLayer( LAYER_PTR(layer), x0, y0, x1, y1, 
      ar->default_linethickness, ar->default_keepaway, 
      MakeFlags (AUTOFLAG | (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)));

  AddObjectToCreateUndoList (LINE_TYPE, LAYER_PTR(layer), line, line);
#endif

}

void
export_pcb_data_drawarc(autorouter_t *ar, int layer, point_t *centre, double sa, double da, int radius) {

#ifndef TEST
  ArcTypePtr arc = CreateNewArcOnLayer(LAYER_PTR(layer), centre->x, centre->y, radius, radius,
    sa, da, ar->default_linethickness, ar->default_keepaway, 
    MakeFlags( AUTOFLAG | (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)));

  AddObjectToCreateUndoList( ARC_TYPE, LAYER_PTR(layer), arc, arc);
#endif

}





double
get_angle(point_t *p, point_t *pp, double r) {
  double sa;

  if(p->x >= pp->x && p->y >= pp->y) {
    /* quadrant I */
    sa = 90.0f + ( asin( (p->x - pp->x) / r ) * 180 / M_PI);
  }else if(p->x >= pp->x && p->y < pp->y) {
    /* quadrant IV */
    sa = 270.0f - ( asin( (p->x - pp->x) / r ) * 180 / M_PI);
  }else if(p->x < pp->x && p->y >= pp->y) {
    /* quadrant II */
    sa = 90.0f - ( asin( (pp->x - p->x) / r ) * 180 / M_PI);
  }else{
    /* quadrant III */
    sa = 270.0f + ( asin( (pp->x - p->x) / r ) * 180 / M_PI);
  }

  return sa;
}

double
get_delta_angle(double sa, double ea, dir_t dir) {
  
  if(sa < ea) {
    if(dir == CLOCKWISE) return ea - sa;
    else if(dir == COUNTERCLOCKWISE) return -(360.0f - ea + sa);
  }else if(sa > ea) {
    if(dir == CLOCKWISE) return 360.0f - sa + ea;
    else if(dir == COUNTERCLOCKWISE) return -(sa - ea);
  }
  return 0.0f;
}


void 
rbs_export_arc_to_term(autorouter_t *ar, problem_t *prob, int layer, linklist_t *segments,
          dir_t dir, double sa, double radius) {

  point_t *firstpoint = ((edge_t *) segments->data)->p1; 
  point_t *nextterm = get_next_arc_or_term(segments);

  double theta = acos(radius / length(firstpoint->pull_point->x, firstpoint->pull_point->y, nextterm->x, nextterm->y));

  double i = radius * cos(theta);
  double j = radius * sin(theta);

  double m = gradient(nextterm, firstpoint->pull_point); 
  
  point_t tp[2];
  find_points_on_line(firstpoint->pull_point, m, i, &tp[0], &tp[1]);

  point_t rp;
  if(length(tp[0].x,tp[0].y,nextterm->x,nextterm->y) < length(tp[1].x,tp[1].y,nextterm->x,nextterm->y)) {
    rp.x = tp[0].x; rp.y = tp[0].y;
  }else{
    rp.x = tp[1].x; rp.y = tp[1].y;
  } 

  m = (m == 0) ? INFINITY : -1/m;

  find_points_on_line(&rp, m, j, &tp[0], &tp[1]);
 
  if(wind(firstpoint->pull_point, nextterm, &tp[0]) == wind(firstpoint->pull_point, nextterm, firstpoint)) {
    rp.x = tp[0].x; rp.y = tp[0].y;
  }else{
    rp.x = tp[1].x; rp.y = tp[1].y;
  } 

  /* rp now contains coordinates of begin of arc */
  
  double da = get_delta_angle( sa, get_angle(&rp, firstpoint->pull_point, radius), dir);

  /* draw back to curpoint */
  export_pcb_data_drawline(ar, layer, rp.x, rp.y, nextterm->x, nextterm->y);
  export_pcb_data_drawarc(ar, layer, firstpoint->pull_point, sa, da, radius);

}



void
rbs_export_arc_to_arc(autorouter_t *ar, problem_t *prob, int layer, linklist_t *segments, 
          dir_t dir, double sa, double radius) {
  printf("rbs_export_arc_to_arc:\n");

  point_t *firstpoint = ((edge_t *) segments->data)->p1; 
  point_t *nextarc = NULL, *nextarc_end = NULL;
  linklist_t *nextarc_segments = NULL;
  dir_t nextarc_dir = UNKNOWN;

  //int nextfunc = 0;	
  void (*nextfunc)(autorouter_t *ar, problem_t *prob, int layer, linklist_t *segments, 
                   dir_t dir, double sa, double radius) = NULL;

  linklist_t *curll = segments;
  while(curll != NULL) {
    edge_t *edge = (edge_t *) curll->data;

    if(nextarc != NULL) {

      if(edge->p2->pull_point == nextarc->pull_point) {
        nextarc_end = edge->p2;
        nextarc_segments = curll->next;
        printf("settingn nextarc_segments...\n");
      }else{
        if(edge->p2->type != TCSPOINT) {
          nextfunc = rbs_export_arc_to_term;
          break;
        }else	if(edge->p2->pull_point->type != INTERSECT) {
          nextfunc = rbs_export_arc_to_arc;
          break;
        }
      }
    }	
    else	
      if(edge->p2->pull_point->type != INTERSECT) {
        nextarc = edge->p2;
        nextarc_end = edge->p2;
        nextarc_segments = curll->next;
        if(curll->next != NULL)
          nextarc_dir = wind( nextarc, ((edge_t *)curll->next->data)->p2, nextarc->pull_point );
      }

    curll = curll->next;
  }

  if(nextfunc == NULL) {
    printf("error: couldn't determine next function..\n");
    return;
  }

  if(nextarc == NULL || nextarc_dir == UNKNOWN) {
    printf("error: coulnd't find next arc...\n");
    return;
  }	

  point_t rp1, rp2;

  if(nextarc_dir == dir) {
    /* its straight */
    double r2 = get_rubberband_radius(ar, nextarc);
    double r1 = get_rubberband_radius(ar, firstpoint);
    point_t *smallpoint = NULL, *bigpoint = NULL;

    if(r1 == r2) {
      point_t tp[2];

      double m = gradient(nextarc->pull_point, firstpoint->pull_point); 

      m = (m == 0) ? INFINITY : (m == INFINITY) ? 0 : -1/m;
 
      find_points_on_line(nextarc->pull_point, m, r1, &tp[0], &tp[1]);

      if(wind(firstpoint->pull_point, nextarc->pull_point, &tp[0]) == wind(firstpoint->pull_point, nextarc->pull_point, nextarc)) {
        rp2.x = tp[0].x; rp2.y = tp[0].y;
      }else{
        rp2.x = tp[1].x; rp2.y = tp[1].y;
      } 
      
      /* rp2 now contains the start of the second arc */
      
      find_points_on_line(firstpoint->pull_point, m, r1, &tp[0], &tp[1]);
      
      if(wind(firstpoint->pull_point, nextarc->pull_point, &tp[0]) == wind(firstpoint->pull_point, nextarc->pull_point, firstpoint)) {
        rp1.x = tp[0].x; rp1.y = tp[0].y;
      }else{
        rp1.x = tp[1].x; rp1.y = tp[1].y;
      } 
      
      /* rp1 now contains the end of the first arc */

    }else{
      point_t tp[2];
      double smallr = 0.0f, bigr = 0.0f;
      if(r1 > r2) {
        smallpoint = nextarc->pull_point;
        bigpoint = firstpoint->pull_point;
        smallr = r1; bigr = r2;
      }else{
        smallpoint = firstpoint->pull_point;
        bigpoint = nextarc->pull_point;
        smallr = r2; bigr = r1;
      }

      double r3 = bigr - smallr;
      double theta = acos(r3 / length(firstpoint->pull_point->x, firstpoint->pull_point->y, nextarc->pull_point->x, nextarc->pull_point->y));

      double i2 = r3 * cos(theta);
      double j2 = r3 * sin(theta);

      double i = smallr * cos(theta);
      double j = smallr * sin(theta);
      
      double m = gradient(smallpoint, bigpoint); 
      
      find_points_on_line(bigpoint, m, i + i2, &tp[0], &tp[1]);

      if(length(tp[0].x,tp[0].y,smallpoint->x,smallpoint->y) < length(tp[1].x,tp[1].y,smallpoint->x,smallpoint->y)) {
        rp1.x = tp[0].x; rp1.y = tp[0].y;
      }else{
        rp1.x = tp[1].x; rp1.y = tp[1].y;
      } 
      
      find_points_on_line(smallpoint, m, i, &tp[0], &tp[1]);

      if(length(tp[0].x,tp[0].y,bigpoint->x,bigpoint->y) > length(tp[1].x,tp[1].y,bigpoint->x,bigpoint->y)) {
        rp2.x = tp[0].x; rp2.y = tp[0].y;
      }else{
        rp2.x = tp[1].x; rp2.y = tp[1].y;
      } 
      
      m = (m == 0) ? INFINITY : (m == INFINITY) ? 0 : -1/m;
      
      find_points_on_line(&rp1, m, j + j2, &tp[0], &tp[1]);
      
//      int tempdir = (dir==CLOCKWISE)?COUNTERCLOCKWISE:CLOCKWISE;

      if(wind(smallpoint, bigpoint, &tp[0]) == wind(smallpoint, bigpoint, firstpoint)) {
        rp1.x = tp[0].x; rp1.y = tp[0].y;
      }else{
        rp1.x = tp[1].x; rp1.y = tp[1].y;
      } 
      
      find_points_on_line(&rp2, m, j, &tp[0], &tp[1]);
      
      if(wind(smallpoint, bigpoint, &tp[0]) == wind(smallpoint, bigpoint, firstpoint)) {
        rp2.x = tp[0].x; rp2.y = tp[0].y;
      }else{
        rp2.x = tp[1].x; rp2.y = tp[1].y;
      } 

      /* rp1 now contains the bigpoint radius point, and rp2 the smallpoing radius point */
      
      if(r2 > r1) {
        point_t tp3;
        memcpy(&tp3, &rp2, sizeof(point_t));
        memcpy(&rp2, &rp1, sizeof(point_t));
        memcpy(&rp1, &tp3, sizeof(point_t));
      }
        
    }
  }else{
    /* its a twist */
      
    point_t tp[2];
    
    double r2 = get_rubberband_radius(ar, nextarc);
    double theta = acos((radius + r2) / length(firstpoint->pull_point->x, firstpoint->pull_point->y, nextarc->pull_point->x, nextarc->pull_point->y));

    double i = radius * cos(theta);
    double j = radius * sin(theta);

    //double m = gradient(firstpoint, nextarc->pull_point); 
    double m = gradient(nextarc->pull_point, firstpoint->pull_point); 
 
    find_points_on_line(firstpoint->pull_point, m, i, &tp[0], &tp[1]);

    if(length(tp[0].x,tp[0].y,nextarc->pull_point->x,nextarc->pull_point->y) < length(tp[1].x,tp[1].y,nextarc->pull_point->x,nextarc->pull_point->y)) {
      rp1.x = tp[0].x; rp1.y = tp[0].y;
    }else{
      rp1.x = tp[1].x; rp1.y = tp[1].y;
    } 
    
    m = (m == 0) ? INFINITY : (m == INFINITY) ? 0 : -1/m;
    
    find_points_on_line(&rp1, m, j, &tp[0], &tp[1]);

    if(wind(firstpoint->pull_point, nextarc->pull_point, &tp[0]) == nextarc_dir) {
//    if(wind(firstpoint->pull_point, nextarc->pull_point, &tp[0]) == wind(firstpoint->pull_point, nextarc->pull_point, firstpoint)) {
      rp1.x = tp[0].x; rp1.y = tp[0].y;
    }else{
      rp1.x = tp[1].x; rp1.y = tp[1].y;
    } 
    
    /* rp1 now contains correct coordinates */
    
    i = r2 * cos(theta);
    j = r2 * sin(theta);

    m = gradient(nextarc->pull_point, firstpoint->pull_point); 
 
    find_points_on_line(nextarc->pull_point, m, i, &tp[0], &tp[1]);

    if(length(tp[0].x,tp[0].y,firstpoint->pull_point->x,firstpoint->pull_point->y) < length(tp[1].x,tp[1].y,firstpoint->pull_point->x,firstpoint->pull_point->y)) {
      rp2.x = tp[0].x; rp2.y = tp[0].y;
    }else{
      rp2.x = tp[1].x; rp2.y = tp[1].y;
    } 
    
    m = (m == 0) ? INFINITY : (m == INFINITY) ? 0 : -1/m;
    
    find_points_on_line(&rp2, m, j, &tp[0], &tp[1]);

    if(wind(firstpoint->pull_point, nextarc->pull_point, &tp[0]) == dir) {
//    if(wind(firstpoint->pull_point, nextarc->pull_point, &tp[0]) == wind(firstpoint->pull_point, nextarc->pull_point, nextarc)) {
      rp2.x = tp[0].x; rp2.y = tp[0].y;
    }else{
      rp2.x = tp[1].x; rp2.y = tp[1].y;
    } 
    
    /* rp2 now contains correct coordinates */

  }

  
  double da = get_delta_angle( sa, get_angle(&rp1, firstpoint->pull_point, radius), dir);

  export_pcb_data_drawarc(ar, layer, firstpoint->pull_point, sa, da, radius);
  export_pcb_data_drawline(ar, layer, rp1.x, rp1.y, rp2.x, rp2.y);

  printf("rp1 = %d,%d, %s arc starting at %d,%d\n",
      rp1.x, rp1.y, 
      ((nextarc_dir == CLOCKWISE) ? "CLOCKWISE" : "COUNTERCLOCKWISE"), 
      rp2.x, rp2.y);
  
  radius = get_rubberband_radius(ar, nextarc);
  sa = get_angle(&rp2, nextarc->pull_point, radius); 

  nextfunc(ar, prob, layer, nextarc_segments, nextarc_dir, sa, radius);

}


void
rbs_export_term_to_arc(autorouter_t *ar, problem_t *prob, int layer, linklist_t *segments) {
  printf("rbs_export_term_to_arc:\n");

  point_t *firstpoint = ((edge_t *) segments->data)->p1; 
  point_t *nextarc = NULL, *nextarc_end = NULL;
  linklist_t *nextarc_segments = NULL;
  dir_t nextarc_dir = UNKNOWN;

  //int nextfunc = 0;	
  void (*nextfunc)(autorouter_t *ar, problem_t *prob, int layer, linklist_t *segments, 
                   dir_t dir, double sa, double radius) = NULL;

  linklist_t *curll = segments;
  while(curll != NULL) {
    edge_t *edge = (edge_t *) curll->data;

    if(nextarc != NULL) {

      if(edge->p2->pull_point == nextarc->pull_point) {
        nextarc_end = edge->p2;
        nextarc_segments = curll->next;
      }else{
        if(edge->p2->type != TCSPOINT) {
          nextfunc = rbs_export_arc_to_term;
          break;
        }else	if(edge->p2->pull_point->type != INTERSECT) {
          nextfunc = rbs_export_arc_to_arc;
          break;
        }
      }
    }	
    else	
      if(edge->p2->pull_point->type != INTERSECT) {
        nextarc = edge->p2;
        nextarc_end = edge->p2;
        nextarc_segments = curll->next;
        //printf("nextarc = %d,%d\n", nextarc->x, nextarc->y);
        if(curll->next != NULL)
          nextarc_dir = wind( nextarc, ((edge_t *)curll->next->data)->p2, nextarc->pull_point );
      }

    curll = curll->next;
  }

  if(nextfunc == NULL) {
    printf("error: couldn't determine next function..\n");
    return;
  }

  if(nextarc == NULL || nextarc_dir == UNKNOWN) {
    printf("error: coulnd't find next arc...\n");
    return;
  }	

  printf("starting point = %d,%d, %s arc starting at %d,%d\n",
      firstpoint->x, firstpoint->y, 
      ((nextarc_dir == CLOCKWISE) ? "CLOCKWISE" : "COUNTERCLOCKWISE"), 
      nextarc->x, nextarc->y);

  double radius = get_rubberband_radius(ar, nextarc);
  double theta = acos(radius / length(firstpoint->x, firstpoint->y, nextarc->pull_point->x, nextarc->pull_point->y));

  double i = radius * cos(theta);
  double j = radius * sin(theta);

  //double m = gradient(firstpoint, nextarc->pull_point); 
  double m = gradient(nextarc->pull_point, firstpoint); 
 
  printf("radius=%f theta=%f i=%f j=%f m=%f\n", radius, theta, i, j, m);

  point_t tp[2];
  find_points_on_line(nextarc->pull_point, m, i, &tp[0], &tp[1]);

  point_t rp;
  if(length(tp[0].x,tp[0].y,firstpoint->x,firstpoint->y) < length(tp[1].x,tp[1].y,firstpoint->x,firstpoint->y)) {
    rp.x = tp[0].x; rp.y = tp[0].y;
  }else{
    rp.x = tp[1].x; rp.y = tp[1].y;
  } 

  m = (m == 0) ? INFINITY : (m == INFINITY) ? 0 : -1/m;

  find_points_on_line(&rp, m, j, &tp[0], &tp[1]);
 
  if(wind(firstpoint, nextarc->pull_point, &tp[0]) == wind(firstpoint, nextarc->pull_point, nextarc)) {
    rp.x = tp[0].x; rp.y = tp[0].y;
  }else{
    rp.x = tp[1].x; rp.y = tp[1].y;
  } 

  /* rp now contains coordinates of begin of arc */
  
  /* draw back to curpoint */
  export_pcb_data_drawline(ar, layer, rp.x, rp.y, firstpoint->x, firstpoint->y);
 
  double sa = get_angle(&rp, nextarc->pull_point, radius);  

  nextfunc(ar, prob, layer, nextarc_segments, nextarc_dir, sa, radius);

}

void
rbs_export_term_to_term(autorouter_t *ar, problem_t *prob, int layer, linklist_t *segments) {
  printf("rbs_export_term_to_term:\n");
  
  int x0 = ((edge_t *)segments->data)->p1->x;
  int y0 = ((edge_t *)segments->data)->p1->y;
  int x1,y1;

  linklist_t *curll = segments;
  while(curll != NULL) {
    edge_t *edge = (edge_t *) curll->data;

    printf("\t%d,%d -> %d,%d\n", edge->p1->x, edge->p1->y, edge->p2->x, edge->p2->y);
    x1 = edge->p2->x;
    y1 = edge->p2->y;

    curll = curll->next;
  }
  

#ifndef TEST 
  LineTypePtr line;
  line = CreateDrawnLineOnLayer( LAYER_PTR(layer), x0, y0, x1, y1, 
      ar->default_linethickness, ar->default_keepaway, 
      MakeFlags (AUTOFLAG | (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)));

  AddObjectToCreateUndoList (LINE_TYPE, LAYER_PTR(layer), line, line);
#endif

}

void
reroute(autorouter_t *ar, problem_t *prob, int layer, edge_t *edge) {
  
  printf("reroute edge %d,%d to %d,%d\n", edge->p1->x, edge->p1->y, edge->p2->x, edge->p2->y);

  edge->segments = NULL;
  edge->tcs_points = NULL;

  balance_edges(&prob->ts[layer]);
  //trace(ar, prob, "prereroute", DRAW_POINTS | DRAW_TRIAGS | DRAW_PROB | DRAW_SOLN_EDGES, layer);
  route_edge(ar, prob, edge, layer);
  
  //trace(ar, prob, "reroute", DRAW_POINTS | DRAW_TRIAGS | DRAW_PROB | DRAW_SOLN_EDGES, layer);

}

void
ripup(autorouter_t *ar, problem_t *prob, int layer, edge_t *edge) {

  printf("ripup edge %d,%d to %d,%d\n", edge->p1->x, edge->p1->y, edge->p2->x, edge->p2->y);
  
  edge->flags |= EDGEFLAG_NOTROUTED;

  linklist_t *curll = edge->segments;
  while(curll != NULL) {
    edge_t *e = (edge_t *) curll->data;
    
    e->flags |= EDGEFLAG_INACTIVE; 
    
    if(e->p2->type == TCSPOINT) {
      
      TAR_EDGES_LOOP(&prob->ts[layer]);
      {
        linklist_t *curll = edge->tcs_points;
        while(curll != NULL) {
          point_t *p = (point_t *)curll->data;
          if(p == e->p2) linklist_remove(&edge->tcs_points, p);
          curll = curll->next;
        }
      }
      TAR_EDGES_ENDLOOP;

    }
    
    curll = curll->next;
  }
}

double
wiring_estimate(linklist_t *segments) {
  double r = 0.0f;
  linklist_t *curll = segments;
  while(curll != NULL) {
    edge_t *e2 = (edge_t *)curll->data;
    r += length(e2->p1->x, e2->p1->y, e2->p2->x, e2->p2->y);
    curll = curll->next;
  }
  return r;
}

/* stolen from PCB's report.c */
double 
net_length(point_t *p) {
  int x = p->x;
  int y = p->y;
  double len = 0.0f;

  ResetFoundPinsViasAndPads(False);
  ResetFoundLinesAndPolygons(False);

  LookupConnection (x, y, False, PCB->Grid, FOUNDFLAG);

  ALLLINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, line))
      {
	double l;
	int dx, dy;
	dx = line->Point1.X - line->Point2.X;
	dy = line->Point1.Y - line->Point2.Y;
	l = sqrt ((double)dx*dx + (double)dy*dy);
	len += l;
      }
  }
  ENDALL_LOOP;

  ALLARC_LOOP (PCB->Data);
  {
    if (TEST_FLAG (FOUNDFLAG, arc))
      {
	double l;
	/* FIXME: we assume width==height here */
	l = M_PI * 2*arc->Width * abs(arc->Delta)/360.0;
	len += l;
      }
  }
  ENDALL_LOOP;
 
//  gui->log ("Net Length for point at %d,%d = %f \n", x, y, len);
  ResetFoundPinsViasAndPads(False);
  ResetFoundLinesAndPolygons(False);

  return len;
}

void
roar(autorouter_t *ar, problem_t *prob, int layer) {
  
  bool changedone = false;

  solution_t *soln = &prob->solns[layer];

  TAR_PROB_POINTS_LOOP(prob);
  {
    linklist_t *incident_edges = NULL;
    linklist_t *attached_edges = NULL;

    double minrad = 0.0f;
    edge_t *minedge = NULL;

    TAR_EDGES_LOOP(prob);
    {
      if(edge->layer == layer) 
        if(!(edge->flags & EDGEFLAG_NOTROUTED) && !(edge->flags & EDGEFLAG_INACTIVE)) {
          if(edge->p1 == point || edge->p2 == point) {
            linklist_tailinsert(&incident_edges, edge);
          }

          linklist_t *curll = edge->segments;
          while(curll != NULL) {
            edge_t *e2 = (edge_t *)curll->data;
            if(e2->p2->pull_point == point) { 
              double r = get_rubberband_radius(ar, e2->p2);
              if(minrad == 0 || r < minrad) {
                minrad = r;
                minedge = edge;
              }
              linklist_tailinsert(&attached_edges, edge);
            }
            curll = curll->next;
          }


        }
    }
    TAR_EDGES_ENDLOOP;

    int incident_c = 0, attached_c = 0;
    double original_wiring_est = 0.0f;

//    printf("Point @ %d,%d:\n", point->x, point->y);

//    printf("\tincident edges:\n");
    linklist_t *curll = incident_edges;
    while(curll != NULL) {
      edge_t *e2 = (edge_t *)curll->data;
//      printf("\t\t%d,%d to %d,%d\n", e2->p1->x, e2->p1->y, e2->p2->x, e2->p2->y);
      incident_c++;
      //original_wiring_est += wiring_estimate(e2->segments);
      original_wiring_est += net_length(e2->p1);
      curll = curll->next;
    }
    
//    printf("\tattached edges:\n");
    curll = attached_edges;
    while(curll != NULL) {
      edge_t *e2 = (edge_t *)curll->data;
//      printf("\t\t%d,%d to %d,%d\n", e2->p1->x, e2->p1->y, e2->p2->x, e2->p2->y);
      attached_c++;
      //original_wiring_est += wiring_estimate(e2->segments);
      original_wiring_est += net_length(e2->p1);
      curll = curll->next;
    }
    
    if(incident_c >= 1 && attached_c >= 2) {
      
      curll = incident_edges;
      while(curll != NULL) {
        ripup(ar, prob, layer, (edge_t *)curll->data);
        curll = curll->next;
      }

      ripup(ar, prob, layer, minedge);
      trace(ar, prob, "ripup", DRAW_POINTS | DRAW_TRIAGS | DRAW_PROB | DRAW_SOLN_EDGES, layer);

      double new_wiring_est = 0.0f;

      reroute(ar, prob, layer, minedge);
      

      curll = incident_edges;
      while(curll != NULL) {
        edge_t *e2 = (edge_t *)curll->data;
        reroute(ar, prob, layer, e2);
        curll = curll->next;
      }

      spring_embedder_tcs(ar, prob, layer);
      
      //new_wiring_est += wiring_estimate(minedge->segments);
      new_wiring_est += net_length(minedge->p1);
      
      curll = incident_edges;
      while(curll != NULL) {
        edge_t *e2 = (edge_t *)curll->data;
        //new_wiring_est += wiring_estimate(e2->segments);
        new_wiring_est += net_length(e2->p1);
        curll = curll->next;
      }
     
      printf("ORIGINAL WIRING = %f \t NEW WIRING = %f\n", original_wiring_est, new_wiring_est);

      if(original_wiring_est <= new_wiring_est) {
        // revert.. 
        curll = incident_edges;
        while(curll != NULL) {
          ripup(ar, prob, layer, (edge_t *)curll->data);
          curll = curll->next;
        }

        ripup(ar, prob, layer, minedge);

        while(incident_edges != NULL) {
          int mincount = -1;
          edge_t *minedge2 = NULL;

          curll = incident_edges;
          while(curll != NULL) {
            if(mincount < 0 || ((edge_t *)curll->data)->order < mincount) {
              mincount = ((edge_t *)curll->data)->order;
              minedge2 = (edge_t *)curll->data;
              linklist_remove(&incident_edges, curll->data);
            }
            curll = curll->next;
          }

          reroute(ar, prob, layer, minedge2);
        }
        reroute(ar, prob, layer, minedge);
        spring_embedder_tcs(ar, prob, layer);
        
      }else{
        changedone = true;

      }
    }
    
    linklist_destroy(incident_edges);
    linklist_destroy(attached_edges);

  }
  TAR_END_LOOP;

//  if(changedone) roar(ar, prob, layer);

}

point_t *
remove_edge_and_find_new_join(autorouter_t *ar, problem_t *prob, int layer, 
    point_t *p1, point_t *p2, point_t *op) {

  /* init clusters */
  
  int count = 0;

  TAR_PROB_POINTS_LOOP(prob);
  {
    if(point->netlist == p1->netlist)
      point->cluster = count++;
  }
  TAR_END_LOOP;

  /* merge clusters */

  TAR_EDGES_LOOP(prob);
  {
    
    if(edge->layer == layer) 
      if(!(edge->flags & EDGEFLAG_NOTROUTED) && !(edge->flags & EDGEFLAG_INACTIVE)) {
        if(edge->p1->netlist == p1->netlist) {

          TAR_PROB_POINTS_LOOP(prob);
          {
            if(point->netlist == p1->netlist) 
              if(point->cluster == edge->p1->cluster)
                point->cluster = edge->p2->cluster;
          }
          TAR_END_LOOP;
                  

        }

      }
  }
  TAR_EDGES_ENDLOOP;

  if(op->cluster == p2->cluster) return p1;
  if(op->cluster == p1->cluster) return p2;

  return NULL;

}

void
roar2(autorouter_t *ar, problem_t *prob, int layer) {
  
  TAR_EDGES_LOOP(prob);
  {
    if(edge->layer == layer) 
      if(!(edge->flags & EDGEFLAG_NOTROUTED) && !(edge->flags & EDGEFLAG_INACTIVE)) {
        
        point_t *minp1 = ((edge_t *) edge->segments->data)->p1;
        point_t *minp2 = NULL;
        double minscore = wiring_estimate(edge->segments);

        linklist_t *possible_points = NULL;
  
        linklist_t *curll = edge->segments;
        while(curll != NULL) {
          edge_t *e2 = (edge_t *)curll->data;
          
          minscore += length(e2->p1->x, e2->p1->y, e2->p2->x, e2->p2->y);
          
          if(e2->p2->type == TCSPOINT) {

            if(e2->p2->edge->p1->netlist == minp1->netlist) {
              point_t *temppoint = e2->p2->edge->p1;
              if(temppoint->type == PIN || temppoint->type == PAD || temppoint->type == VIA)
                linklist_tailinsert(&possible_points, temppoint);
            }else if(e2->p2->edge->p2->netlist == minp1->netlist) {
              point_t *temppoint = e2->p2->edge->p2;
              if(temppoint->type == PIN || temppoint->type == PAD || temppoint->type == VIA)
                linklist_tailinsert(&possible_points, temppoint);
            }

          }

          minp2 = e2->p2;
          curll = curll->next;
        }
        
        bool touched = false;

        point_t *op1 = minp1, *op2 = minp2;
//        point_t *cp1 = minp1, *cp2 = minp2;
         
        curll = possible_points;
        while(curll != NULL) {
          point_t *p = (point_t *)curll->data;
          touched = true;

          printf("point %d,%d\n", p->x, p->y);

          ripup(ar, prob, layer, edge);
          
          edge->p1 = remove_edge_and_find_new_join(ar,prob,layer, op1, op2, p);
          edge->p2 = p;
          if(edge->p1 == NULL) {
            printf("ROAR2: edge->p1 == NULL\n");
            curll = curll->next;
            continue;
          }

          reroute(ar, prob, layer, edge);
          
          spring_embedder_tcs(ar, prob, layer);
          
          double curscore = wiring_estimate(edge->segments);
          if(curscore < minscore) {
            minp1 = edge->p1; minp2 = edge->p2;
            minscore = curscore;
          }

          printf("point %d,%d new score = %f, minscore = %f\n", p->x, p->y, curscore, minscore);

          curll = curll->next;
        }
        
        if(touched) {
          ripup(ar, prob, layer, edge);
          edge->p1 = minp1;
          edge->p2 = minp2;
          reroute(ar, prob, layer, edge);
          spring_embedder_tcs(ar, prob, layer);
        }

        linklist_destroy(possible_points);

      }
  }
  TAR_EDGES_ENDLOOP;

}

void
export_pcb_data(autorouter_t *ar, problem_t *prob, int layer) {

  TAR_EDGES_LOOP(prob);
  {
    if(edge->layer == layer)
      if(!(edge->flags & EDGEFLAG_NOTROUTED) && !(edge->flags & EDGEFLAG_INACTIVE)) {
        /* get first segment in solution */
        point_t *next_arc_or_term = get_next_arc_or_term(edge->segments);
        if(next_arc_or_term != NULL) {
          if(next_arc_or_term->type == TCSPOINT) {
            rbs_export_term_to_arc(ar, prob, layer, edge->segments);
          }else{
            rbs_export_term_to_term(ar, prob, layer, edge->segments);
          }
        }
      }
  }
  TAR_EDGES_ENDLOOP;

}

void
foreach_leaf(autorouter_t *ar, problem_t *prob, 
    void (*func)(autorouter_t *, problem_t *) ) {

  if(prob->left != NULL && prob->right != NULL) {
    foreach_leaf(ar, prob->left, func);
    foreach_leaf(ar, prob->right, func);
  }else{
    func(ar, prob);
  }
}

void
foreach_leaf_layer(autorouter_t *ar, problem_t *prob, 
    void (*func)(autorouter_t *, problem_t *, int) ) {

  if(prob->left != NULL && prob->right != NULL) {
    foreach_leaf_layer(ar, prob->left, func);
    foreach_leaf_layer(ar, prob->right, func);
  }else{
    int i;
    for(i=0;i<ar->board_layers;i++) {
      func(ar, prob, i);
    }
  }
}




void
autorouter_run(autorouter_t *ar) {
  int i;

  printf("**********************************\n"); 
  printf("topological autorouter starting...\n");
  printf("board width = %d height = %d layers = %d\n", ar->board_width, ar->board_height, ar->board_layers);
  printf("netlists = %d\n", ar->netlists_n);
  printf("**********************************\n"); 

  ar->root_problem->x1 = 0; 
  ar->root_problem->y1 = 0; 
  ar->root_problem->x2 = ar->board_width; 
  ar->root_problem->y2 = ar->board_height; 

  ar->root_problem->width = ar->board_width;
  ar->root_problem->height = ar->board_height;


  draw_caption(ar, "Topological Autorouter");
  //draw_caption(ar, "Division by Flow Density");
  
  //divide(ar, ar->root_problem); trace_pause(ar);

  draw_caption(ar, "Triangulation & MST");
  foreach_leaf(ar, ar->root_problem, laa_triangulate); trace_pause(ar);
  foreach_leaf(ar, ar->root_problem, laa_mst); trace_pause(ar);
  foreach_leaf(ar, ar->root_problem, laa_sort_edges);

  draw_caption(ar, "2-Net Layer Assignment");
  foreach_leaf(ar, ar->root_problem, laa); trace_pause(ar);
  
  foreach_leaf(ar, ar->root_problem, spread_vias); 
  
  draw_caption(ar, "Topological Net Ordering");
  foreach_leaf_layer(ar, ar->root_problem, nop);
  

  for(i=0;i<ar->board_layers;i++) {
    trace(ar, ar->root_problem, "nop", DRAW_EDGES|DRAW_POINTS|DRAW_ORDERS, i); trace_pause(ar);
  }
  
 // exit(0);
  
  draw_caption(ar, "Triangulation Crossing Sketch");
  foreach_leaf(ar, ar->root_problem, local_router);
//  for(i=0;i<ar->board_layers;i++) {
    trace(ar, ar->root_problem, "rbscomp", DRAW_POINTS | DRAW_TRIAGS | DRAW_SOLN_EDGES, -1);
//  }
//  draw_caption(ar, "Rubberband Sketch");
  foreach_leaf_layer(ar, ar->root_problem, roar);
//    trace(ar, ar->root_problem, "roar1", DRAW_POINTS | DRAW_TRIAGS | DRAW_PROB | DRAW_SOLN_EDGES, -1);
  foreach_leaf_layer(ar, ar->root_problem, roar2);
//      trace(ar, ar->root_problem, "roar2", DRAW_POINTS | DRAW_TRIAGS | DRAW_PROB | DRAW_SOLN_EDGES, -1);
  foreach_leaf_layer(ar, ar->root_problem, export_pcb_data);

  printf("\n%d routing errors\n", ar->bad_routes);

  printf("**********************************\n"); 
  printf("topological autorouter finished...\n");
  printf("**********************************\n"); 
}



#ifdef TEST

int
main(int argc, char **argv) {

  if(argc==0) {
    printf("Filename required\n");
    return 0;
  }

  autorouter_t *r = autorouter_create();

  autorouter_import_text_netlist(r, argv[1]);

  autorouter_run(r);

  autorouter_destroy(r);
  return 0;
}


#else
  static int
toporouter (int argc, char **argv, int x, int y)
{
  autorouter_t *r = autorouter_create();

  autorouter_import_pcb_data(r);
  
  autorouter_run(r);

  autorouter_destroy(r);
  
  SaveUndoSerialNumber ();

  /* optimize rats, we've changed connectivity a lot. */
  DeleteRats (False /*all rats */ );
  RestoreUndoSerialNumber ();
  AddAllRats (False /*all rats */ , NULL);
  RestoreUndoSerialNumber ();

  IncrementUndoSerialNumber ();

  ClearAndRedrawOutput ();
  
  return 0;
}

static HID_Action toporouter_action_list[] = {
  {"Toporouter", NULL, toporouter,
    "Topological autorouter", "None.."}
};

REGISTER_ACTIONS (toporouter_action_list)

  void
hid_toporouter_init()
{
  register_toporouter_action_list();
}
#endif /* TEST */


