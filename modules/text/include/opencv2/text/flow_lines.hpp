#ifndef FLOW_LINES_HPP
#define FLOW_LINES_HPP

#include <pango/pangocairo.h>
#include <opencv2/text/text_transformations.hpp>


/*
 * A class that holds methods to handle all synthetic flow line generation.
 * Inherits from PathCurve class for access to curving functions and protected
 * make_points_wave function
 */
class FlowLines : TextTransformations {
private: //----------------------- PRIVATE METHODS --------------------------
  /*
   * Takes array size, returns randomly assigned array of specified len
   */
  static void
  set_dash_pattern(double *pattern, int len);

  /*
    makes a thicker line behind the original that is a different gray-scale hue
  */
  static void
  draw_boundry(cairo_t *cr, bool horizontal, double linewidth, double og_col);

  /*
    draws the main line (thin) and then another line (thick) with a specific dash pattern
    over it such that it looks as if the 2nd line is actually small perpendicular lines
  */
  static void
  draw_hatched(cairo_t *cr, double linewidth);

  /*
    translates distance either up or down (based on the input line orientation)
    to give space for a second line to be drawn in parallel
  */
  static void
  translate_parallel(cairo_t *cr, bool horizontal, double distance);

public: //----------------------- PUBLIC METHODS --------------------------

  static void
      manual_translate(cairo_t *cr, cairo_path_t *path, cairo_path_data_t *data, double xtrans, double ytrans);

  /* IMPERFECT, CURVED VERTICAL LINES TEND TO APPEAR ON RIGHT
   * Draws num_lines lines with random placement that have the characteristics
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
  addLines(cairo_t *cr, bool boundry, bool hatched, bool dashed, bool curved, bool doubleline, bool horizontal, int num_lines, int seed, int width, int height, double color);

};


#endif
