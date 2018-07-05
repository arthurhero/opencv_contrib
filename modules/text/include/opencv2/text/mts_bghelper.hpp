#ifndef MTS_BACKGROUND_HELPER_HPP
#define MTS_BACKGROUND_HELPER_HPP

#include <pango/pangocairo.h>
#include <vector>
#include "mts_basehelper.hpp"

/*
 * A class to handle the synthetic generation of all background features
 */
class MTS_BackgroundHelper : MTS_BaseHelper {
private://----------------------- PRIVATE METHODS --------------------------

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
   * linewidth - the width of the original line (used in scaling for new line)
   * og_col - the color of the original line
   * horizontal - the orientation of the line (false = vertical)
   */
  static void
  draw_boundary(cairo_t *cr, double linewidth, double og_col, bool horizontal);

  /*
   * Draws the main line (thin) and then another line (thick) with a specific
   * dash pattern over it such that it looks as if the 2nd line is actually 
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
   * stroke - flag that dictates whether or not to stroke the parallel line.
   *          (optional parameter, defaults to true)
   */
  static void
  draw_parallel(cairo_t *cr, bool horizontal, double distance, 
		bool stroke=true);

  /*
   * Sets an arbitrary dash pattern to the path stored by cr
   *
   * cr - cairo context
   */
  static void
  set_dash_pattern(cairo_t *cr);


  /*
   * Generates a wiggly curved line path (but doesn't stroke it to surface)
   * using methods from the TextTransformations class
   *
   * cr - cairo context
   * horizontal - gives the orientation of the line (false = vertical)
   * width - surface width
   * height - surface height
   */
  static void
  generate_curve(cairo_t *cr, bool horizontal, int width, int height);


  /*
   * Sets path orientation through rotation and translation
   *
   * cr - cairo context
   * horizontal - gives the orientation of the line (false = vertical)
   * curved - informs whether the line will be curved (false = straight)
   * width - surface width
   * height - surface height
   */
  static coords
  orient_path(cairo_t *cr, bool horizontal, bool curved, int width, int height);


  /* 
   * Draws a line with random placement that has the characteristics
   * specified by the boolean parameters
   *
   * cr - cairo context
   * boundary - if true, add a colored line that runs next to the original
   * hatched - if true, add short perpendicular lines through the original
   * dashed - if true, make line dashed with arbitrary pattern
   * curved - if true, add curvature with create_curved_path from pc
   * doubleline - if true, add another line parallel next to the original
   * horizontal - if true, lines and transformations go left to right, 
   *              else top to bottom
   * width - the width of the layout in pixels
   * height - the height of the layout in pixels
   */
  static void
  addLines(cairo_t *cr, bool boundary, bool hatched, bool dashed, 
	   bool curved, bool doubleline, bool horizontal, int width, 
	   int height, double color);


  /* Add bg patters to the cr like even-spaced straight line, uneven-spaced
   * straight line, grid, etc.
   *
   *
   *
   *
   */
  static void
  addBgPattern (cairo_t *cr, int width, int height, 
		bool even, bool grid, bool curved);

  /*
   *
   */
  static void
  colorDiff (cairo_t *cr, int width, int height, double color1, double color2); 
 

  /*
   * Draws a circle along an arbitrary edge of the surface
   * 
   * cr - cairo context
   * width - surface width
   * height - surface height
   */
  static void
  city_point(cairo_t *cr, int width, int height);


public://------------------------ PUBLIC METHODS ---------------------------

  /*
   * Generates a map-like background 
   *
   * sample - the image surface
   * features - the list of features to be added
   * width - surface width in pixels
   * bg_color - the grayscale color value for the background
   * contrast - the contrast level
   */
  static void
  generateBgSample(CV_OUT Mat& sample, std::vector<BGFeature> &features, 
		   int width, int bg_color, int contrast);
};


#endif
