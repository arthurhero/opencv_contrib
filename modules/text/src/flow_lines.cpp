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
  /*
   * Takes and arbitrarily sets an array pattern for cairo_set_dash, returns array size
   */
  int
  set_dash_pattern(double * pattern);

  /*
translates linewidth and then strokes a line of arbitrary new color and then translates back
   */
void
draw_boundry(double linewidth);

  /*
draws the main line (thin) and then another line (thick) with a specific dash pattern
over it such that it looks as if the 2nd line is actually small perpendicular lines
   */
void
draw_hatched(cairo_t *cr, double linewidth);

  /*
translates a just enough either up or down (based on the input line orientation)
to give space for a second line to be drawn in parallel
   */
void
translate_parallel(bool horizontal, double linewidth);

public: //----------------------- PUBLIC METHODS --------------------------
  /* IMPERFECT, CURVED VERTICAL LINES TEND TO APPEAR ON RIGHT
   * Draws num_lines lines with arbitrary placement that have the characteristics
   * specified by the parameters
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
int
FlowLines::set_dash_pattern(double * pattern) {
  double dash;  

  //set how long the pattern array will be
  int len = 1 + rand() % 5;

  //set the pattern
  for(int i = 0; i < len; i++) {
    dash = (rand() % 10000) / 1000.0;
    pattern[i] = dash;
  }

  return len;
}


void
FlowLines::draw_boundry(double linewidth) {

}


void
FlowLines::draw_hatched(cairo_t *cr, double linewidth) {

}

void
FlowLines::translate_parallel(bool horizontal, double linewidth) {

}


void
FlowLines::addLines(cairo_t *cr, bool boundry, bool hatched, bool dashed, bool curved, bool doubleline, bool horizontal, int num_lines, int seed, int width, int height){
  //init variables
  std::vector<coords> points;
  double x,y,angle,color, magic_line_ratio, line_width;
  int num_points, pattern_len, translation_x, translation_y, parallels;
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
  line_width = std::min(width, height) * magic_line_ratio;
  cairo_set_line_width(cr, line_width);


  // draws one line per iteration of the loop
  for(int i = 0; i < num_lines; i++) { //move loop outside for randomness of params per run???? ================
    //move to origin of surface
    cairo_move_to(cr, 0, 0);
    

    // randomly set translation and rotation based on orientation 
    if(horizontal) { //horizontal line orientation
      //make a starting translation and angle
      angle = ((rand() % 7854) - (rand() % 7854))/10000.0; //get angle +- PI/4
      translation_x = (rand() % width) - (rand() % width); // +- width
      if(curved) { translation_y = -fabs(y); } // translate points up from bottom
      else { translation_y = 0; }

      cairo_translate(cr, translation_x, translation_y);
      cairo_rotate(cr, angle); 

    } else { //vertical line orientation
      //make a starting translation and angle
      angle = -((3 * 7854) - 2 * (rand() % 7854))/10000.0; //get angle from PI/4 to 3PI/4
      
      translation_y = -height/2.0 + (rand() % (int)(height/2.0)) - (rand() % (int)(height/2.0));// =================================================
      translation_x = (rand() % (int) (width/2.0)); //- (rand() % height); // +- height
      printf("rot = %f tran_x = %d tran_y = %d\n", angle, translation_x, translation_y); //===============================================
      
      //translate to curved line start point, rotate around it, translate back
      if(curved) {
	cairo_translate(cr, width/2.0, height);
	cairo_rotate(cr, angle); 
	cairo_translate(cr, translation_x, translation_y);
      } else { //if line is straight, just translate and rotate
	cairo_translate(cr, translation_x, translation_y);
	cairo_rotate(cr, angle); 
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
      pattern_len = set_dash_pattern(dash_pattern); 
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
      translate_parallel(horizontal); //TODO =========================================
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
  bool horizontal = false;
  int num_lines = 10; 
  int seed = 10698;
  
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
