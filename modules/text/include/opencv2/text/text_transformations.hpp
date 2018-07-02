#include <vector>
#include <pango/pangocairo.h>
#include <math.h>

#ifndef TEXT_TRANS_HPP
#define TEXT_TRANS_HPP

// rename pair of doubles for readability as coordinates (x,y)
typedef std::pair<double, double> coords;

typedef double parametrization_t;  

/* Simple struct to hold a path and its parametrization */
typedef struct {
  cairo_path_t *path;
  parametrization_t *parametrization;
} parametrized_path_t;

// path transforming function pointer
typedef void (*transform_point_func_t) (void *closure, double *x, double *y);

class TextTransformations {
private:// -------------------- PRIVATE METHODS ---------------------------


  ////////////// from Behdad's cairotwisted.c (required functions) /////////////

 
  /* Returns Euclidean distance between two points */
  static double
  two_points_distance (cairo_path_data_t *a, 
		       cairo_path_data_t *b);

  /* Returns length of a Bezier curve.
   * Seems like computing that analytically is not easy.  The
   * code just flattens the curve using cairo and adds the length
   * of segments.
   */
  static double
  curve_length (double x0, double y0,
		double x1, double y1,
		double x2, double y2,
		double x3, double y3);

  /* Compute parametrization info.  That is, for each part of the
   * cairo path, tags it with its length.
   *
   * Free returned value with g_free().
   */
  static parametrization_t *
  parametrize_path (cairo_path_t *path);

  /* Project a path using a function.  Each point of the path (including
   * Bezier control points) is passed to the function for transformation.
   */
  static void
  transform_path (cairo_path_t *path, transform_point_func_t f, void *closure);

  /* Project a point X,Y onto a parameterized path.  The final point is
   * where you get if you walk on the path forward from the beginning for X
   * units, then stop there and walk another Y units perpendicular to the
   * path at that point.  In more detail:
   *
   * There's three pieces of math involved:
   *
   *   - The parametric form of the Line equation
   *     http://en.wikipedia.org/wiki/Line
   *
   *   - The parametric form of the Cubic Bézier curve equation
   *     http://en.wikipedia.org/wiki/B%C3%A9zier_curve
   *
   *   - The Gradient (aka multi-dimensional derivative) of the above
   *     http://en.wikipedia.org/wiki/Gradient
   *
   * The parametric forms are used to answer the question of "where will I be
   * if I walk a distance of X on this path".  The Gradient is used to answer
   * the question of "where will I be if then I stop, rotate left for 90
   * degrees and walk straight for a distance of Y".
   */
  static void
  point_on_path (parametrized_path_t *param, double *x, double *y);


  /* Projects the current path of cr onto the provided path. */
  static void
  map_path_onto (cairo_t *cr, cairo_path_t *path);

  ///////////https://github.com/phuang/pango/blob/master/examples/cairotwisted.c

  /*
   * Takes four x,y coordinate points and turns them into a curved path
   * 
   * start - the start point
   * f1 - point to curve around
   * f2 - point to curve around
   * end - end point
   * cp1 - a point to save the results into
   * cp2 - a point to save the results into
   */
  static void 
  four_point_to_cp(coords start,
		   coords f1,
		   coords f2,
		   coords end,
		   coords *cp1,
		   coords *cp2);

  /* 
   * Draws a path shape from points using cubic interpolation.  
   * uses coords from points vec to determine where to draw curves to and from.
   * draws curves point by point from vector.
   *
   * cr - cairo context
   * points - a vector of x,y coordinate pairs 
   *          (precondition: must contain at least 2 elements)
   */
  static void 
  point_to_path(cairo_t *cr, std::vector<coords> points);

  /*
   * Make an arc that follows the path of points vector
   *
   * cr - cairo context
   * points - a vector of x,y coordinate pairs (precondition: size == 2)
   * radius - the radius of the curvature
   * width - width of the surface
   * height - height of the surface
   * direction - a bit that tells whether the returned points will be along the 
   *             top or bottom of the circle. 1 : top, 0 : bottom 
   *             (non-one input defaults to bottom)
   */
  static void 
  points_to_arc_path(cairo_t *cr, std::vector<coords> points, double radius, 
		     double width, double height, short direction);

protected: //----------------- PROTECTED METHODS ----------------------------
  /*
   * Makes a vector of 2 x,y coordinates that follow the arc of the input radius.
   * For noticable results with minimal distortion, radius should be greater than
   * (1/2 * width) but less than (5 * width)
   *
   * width - surface width in pixels
   * height - surface height in pixels
   * radius - the curve radius of the arc (precondition: radius >= 1/2 * width)
   * direction - a bit that tells whether the returned points will be along the 
   *             top or bottom of the circle. 1 : top, 0 : bottom 
   *             (non-one input defaults to bottom)
   */
  static std::vector<coords>
  make_points_arc(double width, double height, double radius, short direction);

  /*
   * Makes and returns a vector of x,y coordinate points for
   * a wave path to be drawn along. Coordinate point variation
   * is determined with rng within certain bounds to prevent
   * distorted results.
   *
   * width - surface width in pixels
   * height - surface height in pixels
   * num_points - the number of points to push onto the vector 
   *              (minimum 3) (range 3-5 for least text distortion)
   * seed - the random number generator seed
   */
  static std::vector<coords>
  make_points_wave(double width, double height, int num_points, int seed);


public:// -------------------- PUBLIC METHODS --------------------------------

  /*
   * Creates an arc path that allows for text to be drawn along
   * it. For minimal distortion and visible results, radius should 
   * be greater than (1/2 * width) but less than (5 * width)
   *
   * cr - cairo context 
   * path - a non-local cairo_path_t variable for this function to use 
   *        (is assigned in function)
   * line - a non-local PangoLayoutLine variable for this function to use 
   *        (is assigned in function)
   * layout - the PangoLayout used for the desired text
   * x - the horizontal distance in pixels the text is positioned along the path
   * y - the vertical distance in pixels the letters are removed from the path 
   *     (y > 20 can become distorted)
   * radius - the radius of the curvature to be created 
   *          (precondition: radius >= .5*width)
   * width - the width of the surface
   * height - the height of the surface
   * direction - a bit that tells whether the returned points will be along the 
   *             top or bottom of the circle. 1 : top, 0 : bottom 
   *             (non-one input defaults to bottom)
   */
  static void 
  create_arc_path (cairo_t *cr, cairo_path_t *path, PangoLayoutLine *line, 
		   PangoLayout *layout, double x, double y, double radius, 
		   double width, double height, short direction);



  /*
   * Creates a curved path from points using cubic interpolation, and allows 
   * for text to be drawn along it
   *
   * cr - cairo context 
   * path - a non-local cairo_path_t variable for this function 
   *        to use (is assigned in function)
   * line - a non-local PangoLayoutLine variable for this function 
   *        to use (is assigned in function)
   * layout - the PangoLayout used for the desired text
   * width - the width of the surface
   * height - the height of the surface
   * x - the horizontal distance in pixels the text is positioned along the path
   * y - the vertical distance in pixels the letters are removed from the path 
   *     (y > 20 can cause distortion)
   * num_points - the number of points that form the path. 
   *              This number determines how many curves are generated.
   *              (minimum 3) (range 3-5 for least distortion)
   * seed - the random number generator seed that determines point 
   *        placement variance
   */
  static void
  create_curved_path (cairo_t *cr, cairo_path_t *path, PangoLayoutLine *line, 
		      PangoLayout *layout, double width, double height, 
		      double x, double y, int num_points, int seed);

  /*
   * An overload for create_curved_path that allows for the points vector
   * to be set outside the function.   
   *
   * points - vector of x,y coordinate pairs that are used to make the
   *          shape of the path
   * stroke - a flag to tell function whether or not to stroke the line or
   *          simply leave it as a path. (optional parameter, default false)
   */
  static void
  create_curved_path (cairo_t *cr, cairo_path_t *path, PangoLayoutLine *line,
		      PangoLayout *layout, double width, double height,
		      double x, double y, std::vector<coords> points,
		      bool stroke=false);



  /* Add bg patters to the cr like even-spaced straight line, uneven-spaced
   * straight line, grid, etc.
   *
   *
   *
   *
   */
  static void
  addBgPattern (cairo_t *cr, int width, int height, 
		bool even, bool grid, bool curved, int seed);

  /*
   *
   */
  static void
  colorDiff (cairo_t *cr, int width, int height, 
	     int seed, double color1, double color2); 
  
  /*
   *
   */
  static char
  randomChar(int seed);
  
  /*
   *
   */
  static void
  distractText (cairo_t *cr, int width, int height, char *font, 
		int seed); 
  
  /*
   * Draws a circle along an arbitrary edge of the surface
   * 
   * cr - cairo context
   * width - surface width
   * height - surface height
   */
  static void
  city_point(cairo_t *cr, int width, int height);

};

#endif

/******* skeleton main for using TextTransformations class ***********
#include <pango/pangocairo.h>
#include <math.h>
#include "text_transformations.hpp"

int main() {

  cairo_status_t status;
  cairo_surface_t *surface;
  cairo_t *cr;
  PangoLayout *layout;
  cairo_path_t *path;
  PangoLayoutLine *line;
  PangoFontDescription *desc;

  //initialize canvas vars
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 
					WIDTH, HEIGHT);
  cr = cairo_create(surface);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); //set bg to white
  cairo_paint(cr);

  layout = pango_cairo_create_layout (cr);
  desc = pango_font_description_from_string ("Sans");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  //set drawing source color
  cairo_set_source_rgb(cr, 0,0,0);

  //go to initial starting point
  cairo_translate (cr, X_POS, Y_POS);

  //set desired text
  pango_layout_set_markup (layout, "<span font='12'>TEXT</span>", -1);

  //instantiate class
  TextTransformations pc;

  //DRAW TEXT CURVE-------------------------------------------

  pc.create_curved_path(cr,path,line,layout,x,y,num_points,seed);
  OR
    pc.create_arc_path(cr,path,line,layout,x,y,radius,width,height,direction);

  //----------------------------------------------------------

  //paste text to surface
  cairo_set_source_rgba (cr, .6, 0, 0, 1);
  cairo_fill_preserve (cr); 

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
****************************************************************/
