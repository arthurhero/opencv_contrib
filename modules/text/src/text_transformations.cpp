#include <math.h>
#include <cmath>
#include <algorithm>
#include <stdlib.h>
#include <pango/pangocairo.h>
#include "opencv2/text/text_transformations.hpp"

using namespace std;
// SEE text_transformations.hpp FOR ALL DOCUMENTATION

//////////////////////////////////////////////////////// from behdad's cairotwisted.c (required functions)

double
TextTransformations::two_points_distance (cairo_path_data_t *a, cairo_path_data_t *b)
{
  double dx, dy;

  dx = b->point.x - a->point.x;
  dy = b->point.y - a->point.y;

  return sqrt (dx * dx + dy * dy);
}


double
TextTransformations::curve_length (double x0, double y0,
				   double x1, double y1,
				   double x2, double y2,
				   double x3, double y3)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_path_t *path;
  cairo_path_data_t *data, current_point;
  int i;
  double length;

  surface = cairo_image_surface_create (CAIRO_FORMAT_A8, 0, 0);
  cr = cairo_create (surface);
  cairo_surface_destroy (surface);

  cairo_move_to (cr, x0, y0);
  cairo_curve_to (cr, x1, y1, x2, y2, x3, y3);

  length = 0;
  path = cairo_copy_path_flat (cr);
  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {

    case CAIRO_PATH_MOVE_TO:
      current_point = data[1];
      break;

    case CAIRO_PATH_LINE_TO:
      length += two_points_distance (&current_point, &data[1]);
      current_point = data[1];
      break;

    default:
    case CAIRO_PATH_CURVE_TO:
    case CAIRO_PATH_CLOSE_PATH:
      g_assert_not_reached ();
    }
  }
  cairo_path_destroy (path);

  cairo_destroy (cr);

  return length;
}


parametrization_t *
TextTransformations::parametrize_path (cairo_path_t *path)
{
  int i;
  cairo_path_data_t *data, last_move_to, current_point;
  parametrization_t *parametrization;

  parametrization = static_cast<parametrization_t*>(g_malloc (path->num_data * sizeof (parametrization_t)));

  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    parametrization[i] = 0.0;
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
      last_move_to = data[1];
      current_point = data[1];
      break;
    case CAIRO_PATH_CLOSE_PATH:
      /* Make it look like it's a line_to to last_move_to */
      data = (&last_move_to) - 1;
      /* fall through */
    case CAIRO_PATH_LINE_TO:
      parametrization[i] = two_points_distance (&current_point, &data[1]);
      current_point = data[1];
      break;
    case CAIRO_PATH_CURVE_TO:
      /*//naive curve-length, treating bezier as three line segments:
	parametrization[i] = two_points_distance (&current_point, &data[1])
	+ two_points_distance (&data[1], &data[2])
	+ two_points_distance (&data[2], &data[3]);
      */ 
      parametrization[i] = curve_length (current_point.point.x, current_point.point.y,
					 data[1].point.x, data[1].point.y,
					 data[2].point.x, data[2].point.y,
					 data[3].point.x, data[3].point.y);
	 
      current_point = data[3];
      break;
    default:
      g_assert_not_reached ();
    }
  }

  return parametrization;
}


void
TextTransformations::transform_path (cairo_path_t *path, transform_point_func_t f, void *closure)
{
  int i;
  cairo_path_data_t *data;

  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_CURVE_TO:
      f (closure, &data[3].point.x, &data[3].point.y);
      f (closure, &data[2].point.x, &data[2].point.y);
    case CAIRO_PATH_MOVE_TO:
    case CAIRO_PATH_LINE_TO:
      f (closure, &data[1].point.x, &data[1].point.y);
      break;
    case CAIRO_PATH_CLOSE_PATH:
      break;
    default:
      g_assert_not_reached ();
    }
  }
}


void
TextTransformations::point_on_path (parametrized_path_t *param,
				    double *x, double *y)
{
  int i;
  double ratio, the_y = *y, the_x = *x, dx, dy;
  cairo_path_data_t *data, last_move_to, current_point;
  cairo_path_t *path = param->path;
  parametrization_t *parametrization = param->parametrization;

  for (i=0; i + path->data[i].header.length < path->num_data &&
	 (the_x > parametrization[i] || path->data[i].header.type == CAIRO_PATH_MOVE_TO);
       i += path->data[i].header.length) {

    the_x -= parametrization[i];
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
      current_point = data[1];
      last_move_to = data[1];
      break;
    case CAIRO_PATH_LINE_TO:
      current_point = data[1];
      break;
    case CAIRO_PATH_CURVE_TO:
      current_point = data[3];
      break;
    case CAIRO_PATH_CLOSE_PATH:
      break;
    default:
      g_assert_not_reached ();
    }
  }
  data = &path->data[i];

  switch (data->header.type) {

  case CAIRO_PATH_MOVE_TO:
    break;
  case CAIRO_PATH_CLOSE_PATH:
    /* Make it look like it's a line_to to last_move_to */
    data = (&last_move_to) - 1;
    /* fall through */
  case CAIRO_PATH_LINE_TO:
    {
      ratio = the_x / parametrization[i];
      /* Line polynomial */
      *x = current_point.point.x * (1 - ratio) + data[1].point.x * ratio;
      *y = current_point.point.y * (1 - ratio) + data[1].point.y * ratio;

      /* Line gradient */
      dx = -(current_point.point.x - data[1].point.x);
      dy = -(current_point.point.y - data[1].point.y);

      /*optimization for: ratio = the_y / sqrt (dx * dx + dy * dy);*/
      ratio = the_y / parametrization[i];
      *x += -dy * ratio;
      *y +=  dx * ratio;
    }
    break;
  case CAIRO_PATH_CURVE_TO:
    {
      /* FIXME the formulas here are not exactly what we want, because the
       * Bezier parametrization is not uniform.  But I don't know how to do
       * better.  The caller can do slightly better though, by flattening the
       * Bezier and avoiding this branch completely.  That has its own cost
       * though, as large y values magnify the flattening error drastically.
       *///interpolation?

      double ratio_1_0, ratio_0_1;
      double ratio_2_0, ratio_0_2;
      double ratio_3_0, ratio_2_1, ratio_1_2, ratio_0_3;
      double _1__4ratio_1_0_3ratio_2_0, _2ratio_1_0_3ratio_2_0;

      ratio = the_x / parametrization[i];

      ratio_1_0 = ratio;
      ratio_0_1 = 1 - ratio;

      ratio_2_0 = ratio_1_0 * ratio_1_0; /*      ratio  *      ratio  */
      ratio_0_2 = ratio_0_1 * ratio_0_1; /* (1 - ratio) * (1 - ratio) */

      ratio_3_0 = ratio_2_0 * ratio_1_0; /*      ratio  *      ratio  *      ratio  */
      ratio_2_1 = ratio_2_0 * ratio_0_1; /*      ratio  *      ratio  * (1 - ratio) */
      ratio_1_2 = ratio_1_0 * ratio_0_2; /*      ratio  * (1 - ratio) * (1 - ratio) */
      ratio_0_3 = ratio_0_1 * ratio_0_2; /* (1 - ratio) * (1 - ratio) * (1 - ratio) */

      _1__4ratio_1_0_3ratio_2_0 = 1 - 4 * ratio_1_0 + 3 * ratio_2_0;
      _2ratio_1_0_3ratio_2_0    =     2 * ratio_1_0 - 3 * ratio_2_0;

      /* Bezier polynomial */
      *x = current_point.point.x * ratio_0_3
	+ 3 *   data[1].point.x * ratio_1_2
	+ 3 *   data[2].point.x * ratio_2_1
	+       data[3].point.x * ratio_3_0;
      *y = current_point.point.y * ratio_0_3
	+ 3 *   data[1].point.y * ratio_1_2
	+ 3 *   data[2].point.y * ratio_2_1
	+       data[3].point.y * ratio_3_0;

      /* Bezier gradient */
      dx =-3 * current_point.point.x * ratio_0_2
	+ 3 *       data[1].point.x * _1__4ratio_1_0_3ratio_2_0
	+ 3 *       data[2].point.x * _2ratio_1_0_3ratio_2_0
	+ 3 *       data[3].point.x * ratio_2_0;
      dy =-3 * current_point.point.y * ratio_0_2
	+ 3 *       data[1].point.y * _1__4ratio_1_0_3ratio_2_0
	+ 3 *       data[2].point.y * _2ratio_1_0_3ratio_2_0
	+ 3 *       data[3].point.y * ratio_2_0;

      ratio = the_y / sqrt (dx * dx + dy * dy);
      *x += -dy * ratio;
      *y +=  dx * ratio;
    }
    break;
  default:
    g_assert_not_reached ();
  }
}

void
TextTransformations::map_path_onto (cairo_t *cr, cairo_path_t *path)
{
  cairo_path_t *current_path;
  parametrized_path_t param;

  param.path = path;
  param.parametrization = parametrize_path (path);

  current_path = cairo_copy_path (cr);
  cairo_new_path (cr);

  transform_path (current_path,
		  (transform_point_func_t) point_on_path, &param);

  cairo_append_path (cr, current_path);

  cairo_path_destroy (current_path);
  g_free (param.parametrization);
}

//////////////////////////////////////https://github.com/phuang/pango/blob/master/examples/cairotwisted.c


void 
TextTransformations::four_point_to_cp(coords start,
				      coords f1,
				      coords f2,
				      coords end,
				      coords *cp1,
				      coords *cp2) {
 
  double AB_DENOM = 27.0;   
  double AB_NUMER1 = 1.0;  
  double AB_NUMER2 = 8.0;  
  double XY_DENOM1 = 9.0;  
  double XY_DENOM2 = 27.0; 
  double XY_DENOM3 = 6.0;  
  double XY_NUMER1 = 27.0; 
  double XY_NUMER2 = 12.0; 
  double XY_MULTI = .5;    

  /*
    f1=(1-t)^3 start + 3(1-t)^2 t cp1 + 3(1-t) t^2 cp2 + t^3 end
    f1=8/27 start + 12/27 cp1 + 6/27 cp2 + 1/27 end
    f2=1/27 start + 6/27 cp1 + 12/27 cp2 + 8/27 end


    f1.x - 1/27 end.x - 8/27 start.x = 12/27 cp1.x + 6/27 cp2.x
    f1.y - 1/27 end.y - 8/27 start.y = 12/27 cp1.y + 6/27 cp2.y
    f2.x - 8/27 end.x - 1/27 start.x = 6/27 cp1.x + 12/27 cp2.x
    f2.y - 8/27 end.y - 1/27 start.y = 6/27 cp1.y + 12/27 cp2.y

    a1 = 12/27 x1 + 6/27 x2
    a2 = 6/27 x1 + 12/27 x2
    0.5 a1 = 6/27 x1 + 3/27 x2

    a2 - 0.5a1 = 9/27 x2

    b2 - 0.5b1 = 9/27 y2
  */
  
  double a1 = f1.first - (AB_NUMER1/AB_DENOM)*end.first - (AB_NUMER2/AB_DENOM)*start.first;
  double a2 = f2.first - (AB_NUMER2/AB_DENOM)*end.first - (AB_NUMER1/AB_DENOM)*start.first;

  double x2 = (a2 - XY_MULTI*a1) * (XY_NUMER1/XY_DENOM1);
  double x1 = (a2 - (XY_NUMER2/XY_DENOM2)*x2) * (XY_NUMER1/XY_DENOM3);

  double b1 = f1.second - (AB_NUMER1/AB_DENOM)*end.second - (AB_NUMER2/AB_DENOM)*start.second;
  double b2 = f2.second - (AB_NUMER2/AB_DENOM)*end.second - (AB_NUMER1/AB_DENOM)*start.second;

  double y2 = (b2 - XY_MULTI*b1) * (XY_NUMER1/XY_DENOM1);
  double y1 = (b2 - (XY_NUMER2/XY_DENOM2)*y2) * (XY_NUMER1/XY_DENOM3);

  cp1->first = x1;
  cp1->second = y1;
  cp2->first = x2;
  cp2->second = y2;
}


void 
TextTransformations::point_to_path(cairo_t *cr, std::vector<coords> points) {

  unsigned int count = points.size();

  if (count < 2) return; //verify preconditions
  
  coords start = points.back(); 
  points.pop_back();
  coords end = points.back();
  points.pop_back();
  count -= 2;
  
  //if there are only 2 coords in points, draw line between the 2
  if (count == 0) {
    cairo_move_to(cr, start.first, start.second);
    cairo_line_to(cr, end.first, end.second);
    return;
  }

  /* calculations
     y = a + bx + cx^2 + dx^3
     w = a + bu + cu^2 + du^3

     y - dx^3 - cx^2 = a + bx
     w - du^3 - cu^2 = a + bu

     (x/u)(w - du^3 - cu^2) = (x/u)a + (x/u)ub
     y - dx^3 - cx^2 - (x/u)(w - du^3 - cu^2) = (1 - x/u)a

     a = (y - dx^3 - cx^2 - (x/u)(w - du^3 - cu^2)) / (1 - x/u)
     b = (y - dx^3 - cx^2 - a) / x
  */

  double x = start.first / 100, y = start.second, u = end.first / 100, w = end.second;
  double a, b, c, d;

  c = rand()%11-5, d = rand()%11-5;

  if (x == u) return;
  else if (x == 0) {
    a = y;
    b = (w - y - d*pow(u,3) - c*pow(u,2)) / u;
  } else if (u == 0) {
    a = w;
    b = (y - w - d*pow(x,3) - c*pow(x,2)) / x;
  } else {
    a = (y - d*pow(x,3) - c*pow(x,2) - (x/u)*(w - d*pow(u,3) - c*pow(u,2))) / (1 - x/u);
    b = (y - d*pow(x,3) - c*pow(x,2) - a) / x;
  }
  
  double coeff[4] = {a,b,c,d};

  cout << "a b c d " << a << " " << b << " " << c << " " << d << endl;

  double x1 = (2.0/3)*x + (1.0/3)*u;
  double x2 = (1.0/3)*x + (2.0/3)*u;
  double y1 = coeff[0] + coeff[1]*x1 + coeff[2]*pow(x1,2) + coeff[3]*pow(x1,3);
  double y2 = coeff[-1] + coeff[1]*x2 + coeff[2]*pow(x2,2) + coeff[3]*pow(x2,3);

  coords f1 = std::make_pair(x1*100,y1); 
  coords f2 = std::make_pair(x2*100,y2);

  coords cp1;
  coords cp2;

  four_point_to_cp(start, f1, f2, end, &cp1, &cp2);

  //draw a 1st curve using cp (curve points)
  cairo_move_to(cr, start.first, start.second);
  cairo_curve_to(cr, cp1.first, cp1.second, cp2.first, cp2.second, end.first, end.second);

  double fdd = coeff[1] + 2*coeff[2]*u + 3*coeff[3]*pow(u,2);
  
  //while still points left in vector, advance start to prev end and end to next point and draw curve
  while (count > 0) {
    start = end;
    end = points.back(); //pop next point from points vec
    points.pop_back();

    x = start.first / 100, y = start.second, u = end.first / 100, w = end.second;
    if (x == u) return;
    else {

      /*
	y = a + bx + cx^2 + dx^3
	w = a + bu + cu^2 + du^3

	fdd = b + 2cx + 3dx^2
	//sdd = 2c + 6dx

	(y+w)/2+100 = a + b(x+u)/2 + c((x+u)/2)^2 + d((x+u)/2)^3
	m = a + bn + cn^2 + dn^3

	solve the matrix
      */
      /*
	d = ((sdd/2)*(pow(x,2)-pow(u,2)-2*x*(x-u)))/
	((pow(x,3)-pow(u,3))-3*pow(x,3)*(x-u)
	-3*x*(pow(x,2)-pow(u,2)-2*x*(x-u)));
	c = (y-w-fdd*(x-u)-d*((pow(x,3)-pow(u,3))-3*pow(x,3)*(x-u)))/
	(pow(x,2)-pow(u,2)-2*x*(x-u));
	b = (y-w-d*(pow(x,3)-pow(u,3))-c*(pow(x,2)-pow(u,2)))/(x-u);
	a = y-d*pow(x,3)-c*pow(x,2)-b*x;
      */

      double m = (y+w)/2;
      double n = (x+u)/2;
      double k = (x-u)/(x-n);
      double x_2 = pow(x,2), u_2 = pow(u,2), n_2 = pow(n,2);
      double x_3 = pow(x,3), u_3 = pow(u,3), n_3 = pow(n,3);

      double j = (x_2-u_2-(x_2-n_2)*k)/(x_2-u_2-2*x*(x-u));
      d = (y-w-(y-m)*k-j*(y-w-fdd*(x-u)))/(x_3-u_3-(x_3-n_3)*k-j*(x_3-u_3-3*x_2*(x-u)));
      c = (y-w-(y-m)*k-d*(x_3-u_3-(x_3-n_3)*k))/(x_2-u_2-(x_2-n_2)*k);
      b = (y-w-d*(x_3-u_3)-c*(x_2-u_2))/(x-u);
      a = (y-d*x_3-c*x_2-b*x);
    }

    coeff[0] = a, coeff[1] = b, coeff[2] = c, coeff[3] = d;

    x1 = (2.0/3)*x + (1.0/3)*u;
    x2 = (1.0/3)*x + (2.0/3)*u;
    y1 = coeff[0] + coeff[1]*x1 + coeff[2]*pow(x1,2) + coeff[3]*pow(x1,3);
    y2 = coeff[0] + coeff[1]*x2 + coeff[2]*pow(x2,2) + coeff[3]*pow(x2,3);

    f1.first = x1 * 100, f1.second = y1,
      f2.first = x2 * 100, f2.second = y2;

    four_point_to_cp(start, f1, f2, end, &cp1, &cp2);
    
    //draw next curve of the path
    cairo_curve_to(cr, cp1.first, cp1.second, cp2.first, cp2.second, end.first, end.second);

    fdd = coeff[1] + 2*coeff[2]*u + 3*coeff[3]*pow(u,2);

    count -= 1;
  }

  return;
}


void 
TextTransformations::points_to_arc_path(cairo_t *cr, std::vector<coords> points, 
					double radius, double width, double height, short direction) {
  
  coords origin;
  coords start = points.back(); 
  coords end = points.front();
 
  double start_angle;
  double end_angle;

  if(direction == 1) {
    // top of circle, so arc origin is below canvas
    double delta_y = sqrt(radius * radius - (width/2) * (width/2)); //pythagorean theorum
    origin = std::make_pair(width/2, height + delta_y); 

    end_angle = -asin(fabs(end.second - origin.second)/radius);
    start_angle = -(M_PI - asin(fabs(start.second - origin.second)/radius));
    cairo_arc(cr, origin.first, origin.second, radius, start_angle, end_angle);

  } else { // direction == 0 (default)
    // bottom of circle, so arc origin is above canvas
    origin = std::make_pair(width/2, height - radius); 
 
    end_angle = asin(fabs(end.second - origin.second)/radius); 
    start_angle = M_PI - asin(fabs(start.second - origin.second)/radius);
    //swithced to arc in negative direction to keep text right-side up
    cairo_arc_negative(cr, origin.first, origin.second, radius, start_angle, end_angle);
  }
}


std::vector<coords>
TextTransformations::make_points_arc(double width, double height, double radius, short direction) {

  if (radius < .5 * width) radius = .5 * width; // verify preconditions
  
  double next_angle;
  std::vector<coords> points;
  double x, y;
  double theta = asin(width / (2 * radius)); //find the angle between the center point of 
                                             //the canvas and the point where the circle 
                                             //intersects the edge of the canvas

  if (direction == 1) { //top of circle
    double delta_y = sqrt(radius * radius - (width/2) * (width/2)); //pythagorean theorum
    //find first edge of the arc using parametric equation of a circle  
    next_angle = M_PI/2 - theta;
    x = (.5 * width) + (radius * cos(next_angle));
    y = (height + delta_y) + (radius * sin(next_angle));
    points.push_back(std::make_pair(x,y));
    
    //find second edge of the arc using parametric equation of a circle
    next_angle = next_angle + 2 * theta;
    x = (.5 * width) + (radius * cos(next_angle));
    y = (height + delta_y) + (radius * sin(next_angle));
    points.push_back(std::make_pair(x,y));

  } else /* direction == 0 */ { //bottom of circle   
    // first edge of arc in top right
    x = width;
    y = 0;
    points.push_back(std::make_pair(x,y));
    
    // second edge of arc in top left
    x = 0;
    y = 0;
    points.push_back(std::make_pair(x,y));
  }
  return points;
}


std::vector<coords>
TextTransformations::make_points_wave(double width, double height, int num_points, int seed) {

  std::vector<coords> points;

  if (num_points < 3) return points; //verify preconditions

  int x_variance, y_variance;
  double x, y;    

  //initialize rng.with seed
  srand (seed);

  //cout << "before for" << endl;
  //created num_points x,y coords
  for(int i = num_points - 1; i >= 0; i--) {
    //y variance of + 0 to 1/8 height
    //cout << "yv" << endl;
    y_variance = (rand() % (int) ((1.0/8.0) * height));

    //x variance of +- 1/8 (width/num_points - 1)
    //cout << "xv" << endl;
    x_variance = (rand() % (int) ceil((1.0/8.0) * (width / (num_points - 1))))
      - (rand() % (int) ceil((1.0/8.0) * (width / (num_points - 1))));

    //cout << "x" << endl;
    x = x_variance + ((width / (num_points - 1)) * i);
    //x = (width / (num_points - 1)) * i;
    //cout << "y" << endl;
    y = height - y_variance; //ensure points stay above the bottom of the canvas
    //y=height;

    coords new_point(x,y);
    points.push_back(new_point);
  }
  //cout << "after for" << endl;

  return points;
}


void 
TextTransformations::create_arc_path (cairo_t *cr, cairo_path_t *path, PangoLayoutLine *line, 
				      PangoLayout *layout, double x, double y, double radius, 
				      double width, double height, short direction) {
  if (radius < .5*width) radius = .5*width; //verify preconditions

  //set the points for the path
  std::vector<coords> points = make_points_arc(width, height, radius, direction);

  //draw path shape
  points_to_arc_path(cr, points, radius, width, height, direction); 
  
  // Decrease tolerance, since the text going to be magnified 
  cairo_set_tolerance (cr, .01);

  path = cairo_copy_path_flat (cr);


  cairo_new_path (cr);

  line = pango_layout_get_line_readonly (layout, 0);

  cairo_move_to (cr, x,y); //establish how far from/along path the text is
  pango_cairo_layout_line_path (cr, line);

  map_path_onto (cr, path);

  //clean up
  cairo_path_destroy (path);

}


void
TextTransformations::create_curved_path (cairo_t *cr, cairo_path_t *path, PangoLayoutLine *line, 
					 PangoLayout *layout, double width, double height,
					 double x, double y, int num_points, int seed) {

  if (num_points < 3) num_points = 3; //verify preconditions

  //set the points for the path
  //cout << "make points" << endl;
  std::vector<coords> points = make_points_wave(width, height, num_points, seed);

  //cout << "to path" << endl;
  point_to_path(cr, points); //draw path shape

  // Decrease tolerance, since the text going to be magnified 
  cairo_set_tolerance (cr, 0.01);


  //cout << "to flat" << endl;
  path = cairo_copy_path_flat (cr);

  cairo_new_path (cr);

  //cout << "get line" << endl;
  line = pango_layout_get_line_readonly (layout, 0);

  cairo_move_to (cr, x,y);//establish how far from/along path the text is
  pango_cairo_layout_line_path (cr, line);

  //cout << "map onto" << endl;
  map_path_onto (cr, path);

  //clean up
  cairo_path_destroy (path);
}

void
TextTransformations::create_curved_path (cairo_t *cr, cairo_path_t *path,
        PangoLayoutLine *line, PangoLayout *layout,
        double width, double height, double x, double y,
        std::vector<coords> points, bool stroke) {

    point_to_path(cr, points); //draw path shape

    //if stroking path, don't execute path destroying functions
    if(!stroke) {
        // Decrease tolerance, since the text going to be magnified 
        cairo_set_tolerance (cr, 0.01);


        path = cairo_copy_path_flat (cr);

        cairo_new_path (cr);

        line = pango_layout_get_line_readonly (layout, 0);

        cairo_move_to (cr, x,y);//establish how far from/along path the text is
        pango_cairo_layout_line_path (cr, line);

        map_path_onto (cr, path);

        //clean up
        cairo_path_destroy (path);
    }
}



void
TextTransformations::addBgPattern (cairo_t *cr, int width, int height, bool even, bool grid, bool curved, int seed) {

    //initialize rng.with seed
    srand (seed);

    //randomly choose number of lines from 2 to 10
    int num = rand()%9+2;

    //length of lines
    int length = std::max(width, height)*1.414;

    //average spacing
    int spacing = length / num;

    //randomly choose rotation degree
    int deg = rand()%360;
    double rad = deg/180.0*3.14;

  cairo_translate(cr, width/2.0, height/2.0);
  cairo_rotate(cr, rad);
  cairo_translate(cr, -width/2.0, -height/2.0);

  //initialize the vector of lines
  std::vector<std::vector<coords> > lines;

  int top_y = -(length-height)/2;
  int bottom_y = height+(length-height)/2;

  //get curving params
  int rand_num = rand()%3+1;
  std::vector<int> xs;
  std::vector<int> ys;
  for (int k=0;k<rand_num;k++) {
      int x_off = rand()%(width/2)-(width/4);
      xs.push_back(x_off);
      int y_off = rand()%length;
      if (k==0) {
          ys.push_back(y_off);
      } else if (y_off<ys[0]){
          ys.insert(ys.begin(),y_off);
      } else if (k==1 && y_off>ys[0]){
          ys.push_back(y_off);
      } else if (k==2 && y_off>ys[1]){
          ys.push_back(y_off);
      } else if (k==2 && y_off<ys[1]){
          ys.insert(ys.begin()+1,y_off);
      }
  }

  //get a random initial spacing
  int b = rand()%spacing;
  if(even) b=spacing;
  int cur_x = -(length-width)/2+b;
  for (int i=0; i<num;i++) {
      std::vector<coords> points;
      points.push_back(std::make_pair(cur_x,top_y));
      if (curved) {
          for (int k=0;k<rand_num;k++) {
              points.push_back(std::make_pair(cur_x+xs[k],top_y+ys[k]));
          }
      }
      points.push_back(std::make_pair(cur_x,bottom_y));
      lines.push_back(points);
      cur_x+=(spacing-b)*2/num*(i+1)+b;
  }

  //draw the lines
  for (int i=0;i<num;i++){
      std::vector<coords> points=lines[i];
      point_to_path(cr, points); //draw path shape
      cairo_stroke(cr);
  }

  if (grid) {
      //deg+=90;
      //rad = deg/180.0*3.14;
      cairo_translate(cr, width/2.0, height/2.0);
      //cairo_rotate(cr, rad);
      cairo_rotate(cr, 3.14/2);
      cairo_translate(cr, -width/2.0, -height/2.0);
      for (int i=0;i<num;i++){
          std::vector<coords> points=lines[i];
          point_to_path(cr, points); //draw path shape
          cairo_stroke(cr);
      }
  }

  cairo_rotate(cr, 0);
}

void
TextTransformations::colorDiff (cairo_t *cr, int width, int height, int seed, double color1, double color2) {
 
    //decide the number of color plates
    srand (seed);
    int num = rand()%2+1; //1~2

    //get random points on top and bottom border
    //std::vector<coords> tops;
    //std::vector<coords> bottoms;
    for (int i=0;i<num;i++) {
        if(i==0)cairo_set_source_rgb(cr,color1,color1,color1);
        else cairo_set_source_rgb(cr,color2,color2,color2);
        bool left = (bool)rand()%2;
        int x_top=rand()%width;
        if (left) {
            cairo_move_to (cr, 0, 0);
        } else {
            cairo_move_to (cr, width, 0);
        }
        cairo_line_to (cr, x_top, 0);
        int x_bottom=rand()%width;
        cairo_line_to (cr, x_bottom, height);
        if (left) {
            cairo_line_to (cr, 0, height);
            cairo_line_to (cr, 0, 0);
        } else {
            cairo_line_to (cr, width, height);
            cairo_line_to (cr, width, 0);
        }
        cairo_fill(cr);
        //tops.push_back(std::make_pair(x_top,0));
        //bottoms.push_back(std::make_pair(x_bottom,height));
    }
    
    cairo_move_to (cr, 0, 0);

}

char
TextTransformations::randomChar(int seed) {
    srand (seed);
    char ch;
    int number = rand()%5; // 1/5 to be number
    if (number==0){
        ch=(char)(rand()%10+48);
    } else {
        ch=(char)((rand()%26+65)+(rand()%2)*32);
    }
    return ch;
}

void
TextTransformations::distractText (cairo_t *cr, int width, int height, char *font, int seed) {

    srand (seed);
    //generate text
    int len = rand()%10+1;
    char text[len+1];
    for (int i=0;i<len;i++) {
        text[i]=randomChar(rand());
    }
    text[len]='\0';

    PangoLayout *layout;
    PangoFontDescription *desc;
    layout = pango_cairo_create_layout (cr);

    desc = pango_font_description_from_string(font);
    pango_layout_set_font_description (layout, desc);
    pango_layout_set_text(layout, text, -1);

    PangoRectangle *ink_rect = new PangoRectangle;
    PangoRectangle *logical_rect = new PangoRectangle;
    pango_layout_get_extents(layout, ink_rect, logical_rect);

    int ink_w=ink_rect->width/1024;
    int ink_h=ink_rect->height/1024;

    int x = rand()%width;
    int y = rand()%height;

    cairo_translate (cr, (double)x, (double)y);

    //randomly choose rotation degree
    int deg = rand()%360;
    double rad = deg/180.0*3.14;

    cairo_translate (cr, ink_w/2.0, ink_h/2.0);
    cairo_rotate(cr, rad);
    cairo_translate (cr, -ink_w/2.0, -ink_h/2.0);

    pango_cairo_show_layout (cr, layout);

    cairo_translate (cr, -x, -y);
    cairo_rotate(cr, 0);
    g_object_unref(layout);
    pango_font_description_free (desc);
    free(logical_rect);
    free(ink_rect);
}
