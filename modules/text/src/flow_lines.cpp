//#include "opencv2/text/path_curve.hpp"
#include "../include/opencv2/text/path_curve.hpp"
#include <pango/pangocairo.h>

/*
 * A class that holds methods to handle all synthetic flow line generation.
 * Inherits from PathCurve class for access to curving functions and protected
 * make_points_wave function
 */
class
FlowLines : PathCurve {
private:
  

public:
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
   */
  static void
  addLines(cairo_t *cr, bool boundry, bool hatched, bool dashed, bool curved, bool doubleline, bool horizontal, int num_lines) {
    for(int i = 0; i < num_lines; i++) {
      
    }
  }

  void func() { this->make_points_arc(100,50,100,1); }

};



// GET RID OF THIS MAIN LATER

int main() {

  cairo_status_t status;
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_path_t *path;

  double width = 600, height = 300;
  
  //initialize canvas vars
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 
					width, height);
  cr = cairo_create(surface);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); //set bg to white
  cairo_paint(cr);

  //set drawing source color
  cairo_set_source_rgb(cr, 0,0,0);
  cairo_move_to(cr, 0,0);
  //instantiate class
  FlowLines fl;

  /*use that good stuff here*/
  fl.func();


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
