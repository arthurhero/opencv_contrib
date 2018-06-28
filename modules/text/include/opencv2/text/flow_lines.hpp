#ifndef FLOW_LINES_HPP
#define FLOW_LINES_HPP

#include <pango/pangocairo.h>


/*
 * A class that holds methods to handle all synthetic flow line generation.
 * Inherits from PathCurve class for access to curving functions and protected
 * make_points_wave function
 */
class FlowLines : PathCurve {
private: //----------------------- PRIVATE METHODS --------------------------
  /*
   * Sets len number of elements in array, pattern.
   * For arbitrarily setting the dash pattern of a line
   *
   * pattern - an array 
   * len - the length of pattern
   */
  static void
  make_dash_pattern(double *pattern, int len);

  /*
   * Makes a thicker line behind the original that is a different gray-scale hue
   *
   * cr - cairo context
   * horizontal - the orientation of the line (false = vertical)
   * linewidth - the width of the original line (used in scaling for new line)
   * og_col - the color of the original line
   */
  static void
  draw_boundry(cairo_t *cr, bool horizontal, double linewidth, double og_col);

  /*
   * Draws the main line (thin) and then another line (thick) with a specific
   * dash patterj over it such that it looks as if the 2nd line is actually 
   * small perpendicular lines
   *
   * cr - cairo context
   * linewidth - the width of the original line (used in scaling for new line)
   */
  static void
  draw_hatched(cairo_t *cr, double linewidth);

  /*
   * Copies the current path and then moves it distance in direction
   * specified by horizontal, finally strokes new parallel line to surface
   *
   * cr - cairo context
   * horizontal - the orientation of the line (false = vertical)
   * distance - the distance between the original line and the new parallel line
   */
  static void
  draw_parallel(cairo_t *cr, bool horizontal, double distance);

  /*
   * Takes in a new path and manually translates each point by the input
   * xtrans and ytrans
   *
   * cr - cairo context
   * path - a new cairo path
   * data - an enum that describes each portion of path
   */
  static void
  manual_translate(cairo_t *cr, cairo_path_t *path, cairo_path_data_t *data, 
		   double xtrans, double ytrans);

  /*
   * Sets an arbitrary dash pattern to the path stored by cr
   *
   * cr - cairo context
   */
  static void
  set_dash_pattern(cairo_t *cr);


  /*
   * Generates a wiggly curved line path (but doesn't stroke it to surface)
   * using methods from the PathCurve class
   *
   * cr - cairo context
   * horizontal - gives the orientation of the line (false = vertical)
   * width - surface width
   * height - surface height
   */
  static void
  generate_curve(cairo_t *cr, bool horizontal, int width, int height);

public: //----------------------- PUBLIC METHODS --------------------------

  /* IMPERFECT, CURVED VERTICAL LINES TEND TO APPEAR ON RIGHT
   * Draws a line with random placement that has the characteristics
   * specified by the boolean parameters
   *
   * cr - cairo context
   * boundry - if true, add a colored line that runs next to the original
   * hatched - if true, add short perpendicular lines through the original
   * dashed - if true, make line dashed with arbitrary pattern
   * curved - if true, add curvature with create_curved_path from pc
   * doubleline - if true, add another line parallel next to the original
   * horizontal - if true, lines and transformations go left to right, 
   *              else top to bottom
   * seed - the seed for the random number generator used in this method
   * width - the width of the layout in pixels
   * height - the height of the layout in pixels
   */
  static void
  addLines(cairo_t *cr, bool boundry, bool hatched, bool dashed, bool curved,
	   bool doubleline, bool horizontal, int seed, int width, int height);

};


#endif
