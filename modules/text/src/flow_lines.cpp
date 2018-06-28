//#include "opencv2/text/path_curve.hpp"
//#include "opencv2/text/flow_lines.hpp"
#include "../include/opencv2/text/path_curve.hpp"
#include "../include/opencv2/text/flow_lines.hpp"
#include <pango/pangocairo.h>
#include <math.h>
#include <algorithm>
#include <stdlib.h>

#include <iostream>

// SEE flow_lines.hpp FOR ALL DOCUMENTATION

void
FlowLines::set_dash_pattern(double * pattern, int len) {
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
  double on_len = linewidth / (1 + (rand() % 5)); 
  double off_len = 1 + (rand() % (int) (15 * on_len)); 
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
FlowLines::addLines(cairo_t *cr, bool boundry, bool hatched, bool dashed,
		    bool curved, bool doubleline, bool horizontal, 
		    int num_lines, int seed, int width, int height){
  //init variables
  std::vector<coords> points;
  double x,y,angle,color, magic_line_ratio, line_width;
  int num_points, pattern_len, translation_x, translation_y, parallels;
 
  PangoLayout *layout;
  cairo_path_t *path;
  PangoLayoutLine *line;
      
  srand(seed);
  
  layout = pango_cairo_create_layout (cr);

  //set line color and width
  color = (rand() % 20) / 100.0; //keep color fairly dark
  cairo_set_source_rgb(cr, color, color, color); // gray-scale
  magic_line_ratio = 1.0/(80.0 + (rand() % 40));
  line_width = std::min(width, height) * magic_line_ratio;
  cairo_set_line_width(cr, line_width);


  // draws one line per iteration of the loop
  for(int i = 0; i < num_lines; i++) { //move loop outside for randomness of params per run???? ================
    //move to origin of surface
    cairo_move_to(cr, 0, 0);
    

    // randomly set translation and rotation based on orientation 
    if(horizontal) { //horizontal line orientation
      //make a starting point for straight lines
      x = 0, y = height - (rand() % height); // y variation of 0-height along left side

      //make a starting translation and angle
      angle = ((rand() % 7854) - (rand() % 7854))/10000.0; //get angle +- PI/4
      translation_x = (rand() % width) - (rand() % width); // +- width
      if(curved) { translation_y = -fabs(y); } // translate points up from bottom
      else { translation_y = 0; }

      cairo_translate(cr, translation_x, translation_y);
      cairo_rotate(cr, angle); 

    } else { //vertical line orientation
      //make a starting point for straight lines
      y = 0, x = width - (rand() % width); // x variation of 0-width along top side

      //make a starting translation and angle
      angle = -((3 * 7854) - 2 * (rand() % 7854))/10000.0; //get angle from PI/4 to 3PI/4
      
      
      //translate to approximate curved line center point, rotate around it, translate back
      if(curved) {
	// try to keep xy translations in bounds of surface. finiky
	translation_y = -height/2.0 + (rand() % (int)(height/2.0)) - (rand() % (int)(height/2.0));
	translation_x = (rand() % (int) (width/2.0)); 

	cairo_translate(cr, width/2.0, height);
	cairo_rotate(cr, angle); 
	cairo_translate(cr, translation_x, translation_y);

      } else { //if line is straight, just translate 
	translation_x = 0;
	translation_y = (rand() % (int) (height)) - (rand() % height); // +- height

	cairo_translate(cr, translation_x, translation_y);
      }
    }

    // set path shape 
    if(curved) { // draw a wiggly line with many points
      num_points = 3 + (rand() % 12); // make from 3 to 15 points 

      //get correct point vector based on line orientation
      if(horizontal) {
	points = PathCurve::make_points_wave(width, height, num_points, rand());
      } else {
	points = PathCurve::make_points_wave(height, height, num_points, rand());
      } 
      //curve the path and give extra optional parameter to stroke the path
      PathCurve::create_curved_path(cr,path,line,layout,width,height,0,0,points,true); // ==========================call boundry here???

    } else { // draw a straight line
      cairo_move_to(cr, x, y); // move to starting point
      if(horizontal) {
	cairo_line_to(cr, width, rand() % height); // make a line to width and a random height
      } else { //vertical 
	cairo_line_to(cr, rand() % width, height); // make a line to height and a random width
      }
    } 

    // set line style
    if(dashed) {
      printf("drawing dashed\n");
      pattern_len = rand() % 6;
      double dash_pattern[pattern_len];

      set_dash_pattern(dash_pattern, pattern_len);
      cairo_set_dash(cr, dash_pattern, pattern_len, 0);
    } // else do nothing, line is already solid

    // set number of parallel lines to make
    if(doubleline) {
      parallels = 2;
    } else {
      parallels = 1;
    }
    // draw a line in parallel for each run of loop
    for (int i = 0; i < parallels; i++) {
      // set boundry or not
      if(boundry) {
	draw_boundry(cr, horizontal, line_width, color); 
      } // else do nothing

      // set hatching or not
      if(hatched) {
	draw_hatched(cr, line_width); 
      } // else do nothing
    
      //stroke
      /*cairo_set_source_rgb(cr, color,color,color);
	cairo_set_line_width(cr, line_width);*/
      cairo_stroke(cr);

      // translate a little so that next line drawn in parallel
      translate_parallel(cr, horizontal, line_width); 
    
    }


    //set rotations and translations back to normal  
    cairo_identity_matrix(cr); 
  }
}


