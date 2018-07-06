#include <pango/pangocairo.h>
#include <math.h>
#include <algorithm>
#include <stdlib.h>
#include <vector>
#include "opencv2/text/mts_bghelper.hpp"

using namespace std;

namespace cv{
    namespace text{

        // SEE mts_bghelper.hpp FOR ALL DOCUMENTATION


        void
            MTS_BackgroundHelper::make_dash_pattern(double * pattern, int len) {
                double dash;

                //set the pattern
                for(int i = 0; i < len; i++) {
                    dash = (rng() % 10000) / 1000.0;
                    pattern[i] = dash;
                }
            }

        /*
           possibly optimize by using cairo_transform instead of manual_translate?
           keep everything the same, but change draw_parallel for cairo_translate 
           (may need if statements to check which way to translate)
         */
        void
            MTS_BackgroundHelper::draw_boundary(cairo_t *cr, double linewidth, 
                    double og_col, bool horizontal) {

                // get original dash code
                int dash_len = cairo_get_dash_count(cr);
                double dash[dash_len], *offset;
                cairo_get_dash(cr, dash, offset);

                // calculate a distance between lines (range linewidth - 6*linewidth)
                double distance = (1 + rng() % 6) * linewidth;

                // set boundary line characteristics
                cairo_set_line_width(cr, 6*linewidth);
                cairo_set_dash(cr, dash, 0,0); //set dash pattern to none

                // set boundary line gray-scale color (lighter than original)
                double color = og_col + .35;
                cairo_set_source_rgb(cr, color, color, color);

                // stroke the boundary line
                cairo_stroke_preserve(cr);

                // translate distance so that main line is drawn off center of boundary
                draw_parallel(cr, horizontal, distance, false); //don't stroke new path

                // reset to color and line width of original line
                cairo_set_line_width(cr, linewidth);
                cairo_set_source_rgb(cr, og_col, og_col, og_col);
                cairo_set_dash(cr, dash, dash_len, 0);
            }


        void
            MTS_BackgroundHelper::draw_hatched(cairo_t *cr, double linewidth) {
                //set width of hatches (2-10 times as thick as original line)
                double wide = (2 + (rng() % 9)) * linewidth;
                cairo_set_line_width(cr, wide);

                //set dash pattern to be used
                double on_len = 3 * linewidth / (1 + (rng() % 5)); 
                double off_len = 3 + (rng() % (int) ceil(15 * on_len)); 
                const double pattern[] = { on_len, off_len};
                cairo_set_dash(cr, pattern, 2, 0);
                cairo_stroke_preserve(cr);

                //return to original settings
                cairo_set_line_width(cr, linewidth);
                cairo_set_dash(cr, pattern, 0, 0); 
            }



        void
            MTS_BackgroundHelper::draw_parallel(cairo_t *cr, bool horizontal, 
                    double distance, bool stroke) {

                cairo_path_t *path;
                cairo_path_data_t *data;
                double xtrans = 0, ytrans = 0;

                //copy current path
                path = cairo_copy_path(cr);

                if(horizontal) { // line horizontal, translate new path in y direction
                    ytrans = distance;
                    manual_translate(cr, path, data, xtrans, ytrans); 

                } else { // line vertical, translate new path in x direction
                    xtrans = distance; 
                    manual_translate(cr, path, data, xtrans, ytrans); 
                }

                //stroke new parallel line if stroke flag is true
                if(stroke) cairo_stroke(cr);

                //destroy used up path
                cairo_path_destroy(path);
            }

        void
            MTS_BackgroundHelper::set_dash_pattern(cairo_t *cr) {
                //set length of pattern (1 - 6)
                int pattern_len = 1 + (rng() % 6);
                double dash_pattern[pattern_len];

                //make and set pattern
                make_dash_pattern(dash_pattern, pattern_len);
                cairo_set_dash(cr, dash_pattern, pattern_len, 0);
            }

        coords
            MTS_BackgroundHelper::orient_path(cairo_t *cr, bool horizontal, bool curved, 
                    int width, int height) {
                double x,y,angle;
                int translation_x, translation_y;

                // randomly set translation and rotation based on orientation 
                if(horizontal) { //horizontal line orientation
                    //make a starting point for straight lines
                    x = 0;
                    y = height - (rng() % height); // y variation of 0-height along left side

                    //make a starting translation and angle
                    angle = ((rng() % 7854) - (rng() % 7854))/10000.0; //get angle +- PI/4
                    translation_x = (rng() % width) - (rng() % width); // +- width
                    if(curved) { translation_y = -fabs(y); } // translate points up from bottom
                    else { translation_y = 0; }

                    cairo_translate(cr, translation_x, translation_y);
                    cairo_rotate(cr, angle); 

                } else { //vertical line orientation
                    //make a starting point for straight lines
                    y = 0;
                    x = width - (rng() % width); // x variation of 0-width along top side

                    //make a starting translation and angle
                    angle = -((3 * 7854) - 2 * (rng() % 7854))/10000.0; // from PI/4 to 3PI/4

                    /*translate to approximate curved line center point, rotate around it, 
                      translate back */
                    if(curved) {
                        // try to keep xy translations in bounds of surface. finiky
                        translation_y = -height/2.0 + (rng() % (int) ceil(height/2.0)) 
                            - (rng() % (int) ceil(height/2.0));
                        translation_x = (rng() % (int) ceil(width/2.0)); 

                        cairo_translate(cr, width/2.0, height);
                        cairo_rotate(cr, angle); 
                        cairo_translate(cr, translation_x, translation_y);

                    } else { //if line is straight, just translate 
                        translation_x = 0;
                        translation_y = (rng() % (int) ceil(height)) 
                            - (rng() % (int) ceil(height)); // +- height

                        cairo_translate(cr, translation_x, translation_y);
                    }
                }
                //set and return starting coordinates
                coords start(x,y);
                return start;
            }

        void
            MTS_BackgroundHelper::generate_curve(cairo_t *cr, bool horizontal, int width, 
                    int height) {

                std::vector<coords> points;
                PangoLayout *layout;
                cairo_path_t *path;
                PangoLayoutLine *line;
                int num_points = 3 + (rng() % 12); // make from 3 to 15 points 

                //get correct point vector based on line orientation
                if(horizontal) {
                    points = MTS_BaseHelper::make_points_wave(width, height, num_points);

                } else {
                    points = MTS_BaseHelper::make_points_wave(height, height, num_points);
                } 

                //curve the path and give extra optional parameter to stroke the path
                MTS_BaseHelper::create_curved_path(cr,path,line,layout,width,
                        height,0,0,points,true);
            }


        void
            MTS_BackgroundHelper::addLines(cairo_t *cr, bool boundary, bool hatched, 
                    bool dashed, bool curved, bool doubleline, 
                    bool horizontal, int width, int height, 
                    double color){

                double magic_line_ratio, line_width;
                coords start_point;

                //set line color and width
                cairo_set_source_rgb(cr, color, color, color); // gray-scale
                magic_line_ratio = 1.0/(40.0 + (rng() % 40)); // ratio to keep line scaled
                line_width = std::min(width, height) * magic_line_ratio;
                cairo_set_line_width(cr, line_width);

                //move to origin of surface
                cairo_move_to(cr, 0, 0);

                //orient the path for the line correctly
                start_point = orient_path(cr, horizontal, curved, width, height);

                // set path shape 
                if(curved) { 
                    // draw a wiggly line
                    generate_curve(cr, horizontal, width, height);

                } else { // draw a straight line
                    // move to starting point
                    cairo_move_to(cr, start_point.first, start_point.second); 
                    if(horizontal) {
                        // make a line to width and a random height
                        cairo_line_to(cr, width, rng() % height); 
                    } else { //vertical 
                        // make a line to height and a random width
                        cairo_line_to(cr, rng() % width, height); 
                    }
                } 

                // set line style to dashed or not (default solid)
                if(dashed) { set_dash_pattern(cr); } 

                // set boundry or not
                if(boundary) { draw_boundary(cr, line_width, color, horizontal); }

                // set hatching or not
                if(hatched) { draw_hatched(cr, line_width); } 

                // draw parallel or not
                if(doubleline) { draw_parallel(cr, horizontal, 3*line_width); }

                //stroke
                cairo_stroke(cr);

                //set rotations and translations back to normal
                cairo_identity_matrix(cr); 
            }


        void
            MTS_BackgroundHelper::addBgBias(cairo_t *cr, int width, int height, int color){

                cairo_pattern_t *pat = cairo_pattern_create_linear(rng_.next()%width,0,rng_.next()%width,height);
                cairo_pattern_t *pat2 = cairo_pattern_create_linear(0,rng_.next()%height,width,rng_.next()%height);
                int num = rng_.next()%5+3;
                int num2 = rng_.next()%30+5; 
                double offset = 1.0/(num-1);
                for (int i=0;i<num;i++){
                    int color1 = color + rng_.next()%200-100;
                    color1 = min(color1, 255);
                    double dcolor = color1/255.0;
                    cairo_pattern_add_color_stop_rgb(pat, i*offset, dcolor,dcolor,dcolor);
                }
                for (int i=0;i<num2;i++){
                    int color2 = color + rng_.next()%200-100;
                    color2 = min(color2, 255);
                    double dcolor = color2/255.0;
                    cairo_pattern_add_color_stop_rgb(pat2, i*offset, dcolor,dcolor,dcolor);
                }

                cairo_set_source(cr, pat);
                cairo_paint_with_alpha(cr,0.3);
                cairo_set_source(cr, pat2);
                cairo_paint_with_alpha(cr,0.3);

            }


        void
            MTS_BackgroundHelper::addBgPattern (cairo_t *cr, int width, int height, 
                    bool even, bool grid, bool curved) {

                //randomly choose number of lines from 2 to 10
                int num = rng()%9+2;

                //length of lines
                int length = std::max(width, height)*1.414;

                //average spacing
                int spacing = length / num;

                //randomly choose rotation degree
                int deg = rng()%360;
                double rad = deg/180.0*3.14;

                cairo_translate(cr, width/2.0, height/2.0);
                cairo_rotate(cr, rad);
                cairo_translate(cr, -width/2.0, -height/2.0);

                //initialize the vector of lines
                std::vector<std::vector<coords> > lines;

                int top_y = -(length-height)/2;
                int bottom_y = height+(length-height)/2;

                //get curving params
                int rand_num = rng()%3+1;
                std::vector<int> xs;
                std::vector<int> ys;
                for (int k=0;k<rand_num;k++) {
                    int x_off = rng()%(width/2)-(width/4);
                    xs.push_back(x_off);
                    int y_off = rng()%length;
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
                int b = rng()%spacing;
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
                    cairo_translate(cr, width/2.0, height/2.0);
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
            MTS_BackgroundHelper::colorDiff (cairo_t *cr, int width, int height, 
                    double color1, double color2) {

                int num = rng()%2+1; //1~2

                //get random points on top and bottom border
                for (int i=0;i<num;i++) {
                    if(i==0)cairo_set_source_rgb(cr,color1,color1,color1);
                    else cairo_set_source_rgb(cr,color2,color2,color2);
                    bool left = (bool)rng()%2;
                    int x_top=rng()%width;
                    if (left) {
                        cairo_move_to (cr, 0, 0);
                    } else {
                        cairo_move_to (cr, width, 0);
                    }
                    cairo_line_to (cr, x_top, 0);
                    int x_bottom=rng()%width;
                    cairo_line_to (cr, x_bottom, height);
                    if (left) {
                        cairo_line_to (cr, 0, height);
                        cairo_line_to (cr, 0, 0);
                    } else {
                        cairo_line_to (cr, width, height);
                        cairo_line_to (cr, width, 0);
                    }
                    cairo_fill(cr);
                }

                //return to origin
                cairo_move_to (cr, 0, 0);
            }

        void
            MTS_BackgroundHelper::city_point(cairo_t *cr, int width, int height) {

                //options for side of the surface the point origin appears on
                enum Side { left, right, top, bottom }; // (top from user perspective)

                int option = rng() % 4; // choose a Side for circle origin
                int x,y; // circle origin coordinates

                int radius = (rng() % ((height/2)+1)) + 5; // range 5-(h/2)

                // set circle origin coords based on random choice of side
                switch(option) {
                    case left:   // 0
                        x = -(rng() % radius);
                        y = rng() % height; 
                        break;
                    case right:  // 1
                        x = (rng() % radius) + width;
                        y = rng() % height;
                        break;
                    case top:    // 2
                        x = rng() % width;
                        y = -(rng() % radius);
                        break; 
                    case bottom: // 3
                        x = rng() % width;
                        y = (rng() % radius) + height;
                        break;
                }

                //draw and fill the circle arc
                cairo_arc(cr, x, y, radius, 0, 2*M_PI);
                cairo_fill(cr);
            }


        void
            MTS_BackgroundHelper::generateBgFeatures(std::vector<BGFeature> &bgFeatures){
                int index=static_cast<int>(bgType_);
                int maxnum=maxnum_[index];

                std::vector<BGFeature> allFeatures={Colordiff, Distracttext, Boundry, Colorblob, Straight, Grid, Citypoint, Parallel, Vparallel, Mountain, Railroad, Riverline};
                for (int i=0;i<maxnum;i++){
                    bool flag = true;
                    while (flag) { 
                        int j = rng()%allFeatures.size();
                        BGFeature cur = allFeatures[j];
                        allFeatures.erase(allFeatures.begin()+j);
                        if (cur==Vparallel && find(bgFeatures.begin(), bgFeatures.end(), Parallel)!= bgFeatures.end()) continue;
                        if (cur==Parallel && find(bgFeatures.begin(), bgFeatures.end(), Vparallel)!= bgFeatures.end()) continue;
                        if (cur==Colordiff && find(bgFeatures.begin(), bgFeatures.end(), Colorblob)!= bgFeatures.end()) continue;
                        if (cur==Colorblob && find(bgFeatures.begin(), bgFeatures.end(), Colordiff)!= bgFeatures.end()) continue;

                        flag=false;

                        int curIndex=static_cast<int>(cur);
                        if(rndProbUnder(bgProbs_[curIndex][index])){
                            bgFeatures.push_back(cur);
                        }
                    }
                }
            }


        void 
            MTS_BackgroundHelper::generateBgSample(cairo_surface_t *&bgSurface, std::vector<BGFeature> &features, int height, int width, int bg_color, int contrast){

                cairo_surface_t *surface;
                cairo_t *cr;
                surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
                cr = cairo_create (surface);

                cairo_set_source_rgb(cr, bg_color/255.0,bg_color/255.0,bg_color/255.0);
                cairo_paint (cr);

                if (find(features.begin(), features.end(), Colordiff)!= features.end()) {
                    double color1 = (bg_color-rng()%(contrast/2))/255.0;
                    double color2 = (bg_color-rng()%(contrast/2))/255.0;
                    colorDiff(cr, width, height, color1, color2);
                }
                if (find(features.begin(), features.end(), Colorblob)!= features.end()) {
                    addSpots(surface,0,false,bg_color-rng()%contrast);
                }

                //add bg bias field
                addBgBias(cr, width, height, bg_color);

                if (find(features.begin(), features.end(), Parallel)!= features.end()) {
                    double color = (bg_color-rng()%contrast)/255.0;
                    cairo_set_source_rgb(cr,color,color,color);
                    addBgPattern(cr, width, height, true, false, true);
                }
                if (find(features.begin(), features.end(), Vparallel)!= features.end()) {
                    double color = (bg_color-rng()%contrast)/255.0;
                    cairo_set_source_rgb(cr,color,color,color);
                    addBgPattern(cr, width, height, false, false, true);
                }
                if (find(features.begin(), features.end(), Grid)!= features.end()) {
                    double color = (bg_color-rng()%contrast)/255.0;
                    cairo_set_source_rgb(cr,color,color,color);
                    addBgPattern(cr, width, height, true, true, false);
                }
                if (find(features.begin(), features.end(), Railroad)!= features.end()) {
                    double color = (bg_color-rng()%contrast)/255.0;
                    int line_num = rng()%2+1;
                    for (int i=0;i<line_num;i++) {
                        addLines(cr, false, true, false, true, false, (bool)rng()%2, width, height, color);
                    }
                }
                if (find(features.begin(), features.end(), Boundry)!= features.end()) {
                    double color = (bg_color-contrast)/255.0;
                    int line_num = rng()%2+1;
                    for (int i=0;i<line_num;i++) {
                        addLines(cr, true, false, (bool)rng()%2, true, false, (bool)rng()%2,  width, height, color);
                    }
                }
                if (find(features.begin(), features.end(), Straight)!= features.end()) {
                    double color = (bg_color-rng()%contrast)/255.0;
                    int line_num = rng()%5+1;
                    for (int i=0;i<line_num;i++) {
                        addLines(cr, false, false, false, false, false, (bool)rng()%2, width, height, color);
                    }
                }
                if (find(features.begin(), features.end(), Riverline)!= features.end()) {
                    double color = (bg_color-rng()%contrast)/255.0;
                    int line_num = rng()%5+1;
                    for (int i=0;i<line_num;i++) {
                        addLines(cr, false, false, false, true, (bool)rng()%2, (bool)rng()%2, width, height, color);
                    }
                }
                if (find(features.begin(), features.end(), Citypoint)!= features.end()) {
                    double color = (bg_color-rng()%contrast)/255.0;
                    cairo_set_source_rgb(cr,color,color,color);
                    city_point(cr, width, height);
                }

                bgSurface = surface;
            }

    }
}
