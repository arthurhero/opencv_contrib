#include "opencv2/text/flow_lines.hpp"
#include "opencv2/text/text_transformations.hpp"

#include <pango/pangocairo.h>
#include <math.h>
#include <algorithm>
#include <stdlib.h>


// SEE flow_lines.hpp FOR ALL DOCUMENTATION

void
FlowLines::make_dash_pattern(double * pattern, int len) {
  double dash;

  //set the pattern
  for(int i = 0; i < len; i++) {
    dash = (rand() % 10000) / 1000.0;
    pattern[i] = dash;
  }
}


void
FlowLines::draw_boundry(cairo_t *cr, bool horizontal, double linewidth, double og_col) {

  //set border line gray-scale color (keep it lighter than original)
  double color = .3 + ((rand() % 50) / 100.0); 
  cairo_set_source_rgb(cr, color, color, color);
  cairo_set_line_width(cr, 3*linewidth);
  cairo_stroke_preserve(cr); // preserve the path

  // reset color and line width
  cairo_set_source_rgb(cr, og_col, og_col, og_col);
  cairo_set_line_width(cr, linewidth);
}


void
FlowLines::draw_hatched(cairo_t *cr, double linewidth) {

  //set width of hatches (2-10 times as thick as original line)
  double wide = (2 + (rand() % 9)) * linewidth;
  cairo_set_line_width(cr, wide);

  //set dash pattern to be used
  double on_len = 3 * linewidth / (1 + (rand() % 5)); 
  double off_len = 3 + (rand() % (int) ceil(15 * on_len)); 
  const double pattern[] = { on_len, off_len};
  cairo_set_dash(cr, pattern, 2, 0);
  cairo_stroke_preserve(cr);

  //return to original settings
  cairo_set_line_width(cr, linewidth);
  cairo_set_dash(cr, pattern, 0, 0); 
}


void
FlowLines::translate_parallel(cairo_t *cr, bool horizontal, double distance) {
  double xtrans = 0, ytrans = 0;

  if(horizontal) { // line horizontal, translate in y direction
    ytrans = distance;
    cairo_translate(cr, xtrans, ytrans);

  } else { // line vertical, translate in x direction
    xtrans = distance; 
    cairo_translate(cr, xtrans, ytrans);
  }
}


void
FlowLines::draw_parallel(cairo_t *cr, bool horizontal, double distance) {
  
  cairo_path_t *path;
  cairo_path_data_t *data;
  double xtrans = 0, ytrans = 0;

  //copy current path
  path = cairo_copy_path(cr);

  if(horizontal) { // line horizontal, translate new path in y direction
    ytrans = distance;
    manual_translate(cr, path, data, xtrans, ytrans); 

  } else { // line vertical, translate new path in x direction
    xtrans = distance; 
    manual_translate(cr, path, data, xtrans, ytrans); 
  }

  //stroke new parallel line
  cairo_stroke(cr);

  //destroy used up path
  cairo_path_destroy(path);
}


void
FlowLines::manual_translate(cairo_t *cr, cairo_path_t *path, cairo_path_data_t *data, double xtrans, double ytrans) {
  //loop over data in the path and alter each part by specified translation
  for (int i = 0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
      cairo_move_to(cr, data[1].point.x + xtrans, data[1].point.y + ytrans);
      break;

    case CAIRO_PATH_LINE_TO:
      cairo_line_to (cr, data[1].point.x + xtrans, data[1].point.y + ytrans);
      break;

    case CAIRO_PATH_CURVE_TO:
      cairo_curve_to (cr, data[1].point.x + xtrans, data[1].point.y + ytrans,
		      data[2].point.x + xtrans, data[2].point.y + ytrans,
		      data[3].point.x + xtrans, data[3].point.y + ytrans);
      break;

    case CAIRO_PATH_CLOSE_PATH:
      cairo_close_path(cr);
      break;
    }
  }
}


void
FlowLines::set_dash_pattern(cairo_t *cr) {
  //set length of pattern (1 - 6)
  int pattern_len = 1 + (rand() % 6);
  double dash_pattern[pattern_len];

  //make and set pattern
  make_dash_pattern(dash_pattern, pattern_len);
  cairo_set_dash(cr, dash_pattern, pattern_len, 0);
}

coords
FlowLines::orient_path(cairo_t *cr, bool horizontal, bool curved, int width, int height) {
  double x,y,angle;
  int translation_x, translation_y;
 
  // randomly set translation and rotation based on orientation 
  if(horizontal) { //horizontal line orientation
    //make a starting point for straight lines
    x = 0;
    y = height - (rand() % height); // y variation of 0-height along left side

    //make a starting translation and angle
    angle = ((rand() % 7854) - (rand() % 7854))/10000.0; //get angle +- PI/4
    translation_x = (rand() % width) - (rand() % width); // +- width
    if(curved) { translation_y = -fabs(y); } // translate points up from bottom
    else { translation_y = 0; }

    cairo_translate(cr, translation_x, translation_y);
    cairo_rotate(cr, angle); 

  } else { //vertical line orientation
    //make a starting point for straight lines
    y = 0;
    x = width - (rand() % width); // x variation of 0-width along top side

    //make a starting translation and angle
    angle = -((3 * 7854) - 2 * (rand() % 7854))/10000.0; //get angle from PI/4 to 3PI/4
      
      
    //translate to approximate curved line center point, rotate around it, translate back
    if(curved) {
      // try to keep xy translations in bounds of surface. finiky
      translation_y = -height/2.0 + (rand() % (int) ceil(height/2.0)) 
	- (rand() % (int) ceil(height/2.0));
      translation_x = (rand() % (int) ceil(width/2.0)); 

      cairo_translate(cr, width/2.0, height);
      cairo_rotate(cr, angle); 
      cairo_translate(cr, translation_x, translation_y);

    } else { //if line is straight, just translate 
      translation_x = 0;
      translation_y = (rand() % (int) ceil(height)) 
	- (rand() % (int) ceil(height)); // +- height

      cairo_translate(cr, translation_x, translation_y);
    }
  }
  //set and return starting coordinates
  coords start(x,y);
  return start;
}

void
FlowLines::generate_curve(cairo_t *cr, bool horizontal, int width, int height) {
    
  std::vector<coords> points;
  PangoLayout *layout;
  cairo_path_t *path;
  PangoLayoutLine *line;
  int num_points = 3 + (rand() % 12); // make from 3 to 15 points 

  //get correct point vector based on line orientation
  if(horizontal) {
    points = PathCurve::make_points_wave(width, height, num_points, rand());

  } else {
    points = PathCurve::make_points_wave(height, height, num_points, rand());
  } 

  //curve the path and give extra optional parameter to stroke the path
  PathCurve::create_curved_path(cr,path,line,layout,width,height,0,0,points,true);
}


void
FlowLines::addLines(cairo_t *cr, bool boundry, bool hatched, bool dashed,
		    bool curved, bool doubleline, bool horizontal, 
		    int seed, int width, int height){

  double color, magic_line_ratio, line_width;
  int num_points, translation_x, translation_y, parallels;
  coords start_point;
      
  srand(seed);

  //set line color and width
  color = (rand() % 20) / 100.0; //keep color fairly dark
  cairo_set_source_rgb(cr, color, color, color); // gray-scale
  magic_line_ratio = 1.0/(40.0 + (rand() % 40)); // ratio to keep line scaled
  line_width = std::min(width, height) * magic_line_ratio;
  cairo_set_line_width(cr, line_width);

  //move to origin of surface
  cairo_move_to(cr, 0, 0);
    
  //orient the path for the line correctly
  start_point = orient_path(cr, horizontal, curved, width, height);

  // set path shape 
  if(curved) { 
    // draw a wiggly line
    generate_curve(cr, horizontal, width, height);

  } else { // draw a straight line
    // move to starting point
    cairo_move_to(cr, start_point.first, start_point.second); 
    if(horizontal) {
      cairo_line_to(cr, width, rand() % height); // make a line to width and a random height
    } else { //vertical 
      cairo_line_to(cr, rand() % width, height); // make a line to height and a random width
    }
  } 

  // set line style to dashed or not (default solid)
  if(dashed) { set_dash_pattern(cr); } 

  // set boundry or not
  if(boundry) { draw_boundry(cr, horizontal, line_width, color); }

  // set hatching or not
  if(hatched) { draw_hatched(cr, line_width); } 

  // draw parallel or not
  if(doubleline) { draw_parallel(cr, horizontal, 2*line_width); }
    
  //stroke
  cairo_stroke(cr);

  //set rotations and translations back to normal
  cairo_identity_matrix(cr); 
}


