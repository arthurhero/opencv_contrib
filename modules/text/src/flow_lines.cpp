//#include "opencv2/text/path_curve.hpp"
#include "../include/opencv2/text/path_curve.hpp"
#include <pango/pangocairo.h>
#include <math.h>
#include <algorithm>
#include <stdlib.h>

#include <iostream>


/////////////////// header//////////////////////////////

/*
 * A class that holds methods to handle all synthetic flow line generation.
 * Inherits from PathCurve class for access to curving functions and protected
 * make_points_wave function
 */
class
FlowLines : PathCurve {
private: //----------------------- PRIVATE METHODS --------------------------
  

public: //----------------------- PUBLIC METHODS --------------------------
  /*
   * Draws lines with arbitrary characteristics
   *
   * cr - cairo context
   * boundry - if true, add a colored line that runs next to the original
   * hatched - if true, add short perpendicular lines through the original
   * dashed - if true, make line dashed
   * curved - if true, add curvature with create_curved_path from pc
   * doubleline - if true, add another line parallel to the original
   * horizontal - if true, lines and transformations go left to right, else top to bottom
   * num_lines - the number of lines to draw (the number of times this function loops)
   * seed - the seed for the random number generator used in this method
   * width - the width of the layout in pixels
   * height - the height of the layout in pixels
   */
  static void
  addLines(cairo_t *cr, bool boundry, bool hatched, bool dashed, bool curved, bool doubleline, bool horizontal, int num_lines, int seed, int width, int height);



};

///////////////////////////// source ///////////////////////////////

// SEE flow_lines.hpp FOR ALL DOCUMENTATION

void
FlowLines::addLines(cairo_t *cr, bool boundry, bool hatched, bool dashed, bool curved, bool doubleline, bool horizontal, int num_lines, int seed, int width, int height){
  //init variables
  std::vector<coords> points;
  double x,y,angle,color, magic_line_ratio;
  int num_points, pattern_len, translation, parallels;
  double * dash_pattern;
 
  PangoLayout *layout;
  cairo_path_t *path;
  PangoLayoutLine *line;
      
  srand(seed);
  
  layout = pango_cairo_create_layout (cr);

  //set line color and width
  color = (rand() % 20) / 100.0; //keep color fairly dark
  cairo_set_source_rgb(cr, color, color, color); // gray-scale
  magic_line_ratio = 1.0/(80.0 + (rand() % 40));
  cairo_set_line_width(cr, std::min(width, height) * magic_line_ratio);


  // draws one line per iteration of the loop
  for(int i = 0; i < num_lines; i++) { //move loop outside for randomness of params per run???? ================
    //move to origin of surface
    cairo_move_to(cr, 0, 0);

    // randomly set starting positions and rotation based on orientation 
    if(horizontal) { //horizontal line orientation
      //make a starting point and angle
      translation = (rand() % width) - (rand() % width); // +- width
      //starting point xy
      x = 0;
      y = height - (rand() % height); // range 0 - height
      angle = 0;//((rand() % 7854) - (rand() % 7854))/10000.0; //get angle +- PI/4
      
      cairo_translate(cr, translation, 0); // translate some x 
      cairo_rotate(cr, angle); // set plausible rotation angle for horizontal line

    } else { //vertical line orientation
      //make a starting point and angle
      translation = (rand() % height) - (rand() % height); // +- height
      //starting point xy
      x = width - (rand() % width);   // range 0 - width
      y = 0;
      angle = 0;//(3 * (rand() % 7854) - (rand() % 7854))/10000.0; //get angle from PI/4 to 3PI/4;

      cairo_translate(cr, 0, translation); // translate some y
      cairo_rotate(cr, angle); // set plausible rotation angle for vertical line
    }

    // set path shape 
    if(curved) { // draw a wiggly line with many points
      num_points = 3 + (rand() % 12); // make from 3 to 15 points 

      //get correct point vector based on line orientation
      if(horizontal) {
	points = PathCurve::make_points_wave(width, height, num_points, seed);
      } else {
	points = PathCurve::make_points_wave(height, width, num_points, seed);
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

    /*
    // set line style
    if(dashed) {
      pattern_len = set_dash_pattern(dash_pattern); // TODO ===================================
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
	draw_boundry(); //TODO ========================================
      } // else do nothing

      // set hatching or not
      if(hatched) {
	draw_hatched(cr); //TODO =============================================
      } // else do nothing

      // translate a little so that next line drawn in parallel
      translate_parallel(); //TODO =========================================
    }
    */
    // stroke to surface ======================================== done already in parallel loop???
    cairo_stroke(cr);
    
    //set rotations and translations back to normal  
    cairo_identity_matrix(cr); 
  }
}


////////////////////////////// main ////////////////////////////////


// GET RID OF THIS MAIN LATER

int main() {

  cairo_status_t status;
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_path_t *path;

  double width = 600, height = 300;

  bool boundry = false;
  bool hatched = false;
  bool dashed = false;
  bool curved = true;
  bool doubleline = false;
  bool horizontal = true;
  int num_lines = 1; 
  int seed = 34552;
  
  //initialize canvas vars
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 
					width, height);
  cr = cairo_create(surface);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); //set bg to white
  cairo_paint(cr);

  //set drawing source color
  cairo_set_source_rgb(cr, 0,0,0);
  cairo_move_to(cr, 0,0);

  /*use that good stuff here*/
  FlowLines::addLines(cr, boundry, hatched, dashed, curved, doubleline, horizontal, num_lines,seed, width, height);

  //clean up
  cairo_destroy(cr);
  status = cairo_surface_write_to_png (surface, "image.png");
  cairo_surface_destroy (surface);
  
  if (status != CAIRO_STATUS_SUCCESS) {
    g_printerr("Error saving png.");
    return 1;
  }
  return 0;
}
