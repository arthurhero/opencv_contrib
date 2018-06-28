////////////////////////////// main ////////////////////////////////
#include "../include/opencv2/text/path_curve.hpp"
#include "../include/opencv2/text/flow_lines.hpp"
#include <pango/pangocairo.h>
#include <math.h>
#include <iostream>

// GET RID OF THIS MAIN LATER

int main() {

  cairo_status_t status;
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_path_t *path;

  double width = 600, height = 300;

  bool boundry = true; 
  bool hatched = true; 
  bool dashed = true; 
  bool curved = false;
  bool doubleline = true; //broken does nothing
  bool horizontal = false;
  int num_lines = 5; 
  int seed = 23346487;
  
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
