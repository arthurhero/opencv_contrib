#ifndef MTS_BASE_HELPER_HPP
#define MTS_BASE_HELPER_HPP

#include "precomp.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/calib3d.hpp"

#include <pango/pangocairo.h>
#include <vector>

// The different categories for text features
enum TextType {Water, BigLand, SmallLand};

// The 4 general types of backgrounds that can be generated
enum BGType {Flow, Waterbody, Big, Small};

// All possible features that can be incorporated into a background
enum BGFeature {Colordiff, Distracttext, Boundry, Colorblob, 
    Straight, Grid, Citypoint, Parallel, 
    Vparallel, Mountain, Railroad, Riverline};


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


/*
 * A general class to hold methods and fields that are used in many 
 * functions for different purposes.
 */
class MTS_BaseHelper {
    private://----------------------- PRIVATE METHODS --------------------------

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


    protected://-------------------- PROTECTED METHODS -------------------------

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
         * Makes a vector of 2 x,y coordinates that follow the arc of the input 
         * radius. For noticable results with minimal distortion, radius 
         * should be greater than (1/2 * width) but less than (5 * width)
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
         * is determined by a rng within certain bounds to prevent
         * distorted results.
         *
         * width - surface width in pixels
         * height - surface height in pixels
         * num_points - the number of points to push onto the vector 
         *              (minimum 3) (range 3-5 for least text distortion)
         */
        static std::vector<coords>
            make_points_wave(double width, double height, int num_points);


        /*
         * Makes a mask that has holes in it to project over background or text
         */
        static void
            addSpots (cairo_surface_t *text, int degree, bool trans, int color);


        /*
         * Iterativly translates each cairo movement stored in path and 
         * data by xtrans, ytrans. 
         *
         * cr - cairo context
         * path - the cairo path to be translated
         * data - the data corresponding to path (initialized in method)
         * xtrans - the translation distance in pixels in the x direction
         * ytrans - the translation distance in pixels in the y direction
         */
        static void
            manual_translate(cairo_t *cr, cairo_path_t *path, cairo_path_data_t *data, 
                    double xtrans, double ytrans);


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
         */
        static void
            create_curved_path (cairo_t *cr, cairo_path_t *path, PangoLayoutLine *line, 
                    PangoLayout *layout, double width, double height, 
                    double x, double y, int num_points);

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

  static cv::RNG rng_;


    public://----------------------- PUBLIC METHODS --------------------------

  /*
   * A basic constructor for MTS_BaseHelper
   */
        MTS_BaseHelper();


        /*
         * Returns true or false based on a randomly generated probability under
         * the input value v
         *
         * v - the probability to be calculated under
         */
        static bool
            rndProbUnder(double v);


  /*
   * A setter function for setting the seed used in the rng field
   *
   * rndState - the seed set for the rng
   */
        static void
            setSeed(uint64 rndState);


  /*
   * A wrapper function for the rng_ field in this class. 
   * Generates a positive random number
   */
        static int
            rand();


  /*
   * Randomly assigns the background and text types of the image
   * (options from BGType and TextType enums)
   */
        static void
            setTypes();


        //public static fields
         double bgProbability_[2];
         TextType textType_;
         BGType bgType_;

        //text features
        static double stretchProbability_[3][4];
        static double spacingProbability_[3][4];

        static double curvingProbability_[3];

        static double italicProbability_[3];
        static double weightProbability_[3][2];

        static double fontProbability_[3][2];

        //independent text properties
        static double missingProbability_;
        static double rotatedProbability_;
        static double rotatedAngle_;

        static double finalBlendAlpha_;
        static double finalBlendProb_;

        //bg features
        static double bgProbs_[12][4];

        //max num of bg features each bg4 type can have
        static int maxnum_[4];
};

#endif
