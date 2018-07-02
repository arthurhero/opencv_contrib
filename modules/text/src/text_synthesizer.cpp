#include "precomp.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/calib3d.hpp"

#include "opencv2/text/text_synthesizer.hpp"
#include "opencv2/text/text_transformations.hpp"
#include "opencv2/text/flow_lines.hpp"


#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <map>
#include <limits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <glib.h>
#include <pango/pangocairo.h>


using namespace std;

namespace cv{
    namespace text{

        namespace {
            //Unnamed namespace with auxiliary classes and functions used for quick computation
            static void MatToCairo(Mat &MC3,cairo_surface_t *surface)
            {
                Mat MC4 = Mat(cairo_image_surface_get_height(surface),cairo_image_surface_get_width(surface),CV_8UC4,cairo_image_surface_get_data(surface));
                vector<Mat> Imgs1;
                vector<Mat> Imgs2;
                cv::split(MC3,Imgs2);
                cv::split(MC4,Imgs1);
                for(int i=0;i<3;i++)
                {   
                    Imgs1[i]=Imgs2[i];
                }
                // Alpha - прозрачность
                Imgs1[3]=255;
                cv::merge(Imgs1,MC4);
            }

            static void CairoToMat(cairo_surface_t *surface,Mat &MC3)
            {
                Mat MC4 = Mat(cairo_image_surface_get_height(surface),cairo_image_surface_get_width(surface),CV_8UC4,cairo_image_surface_get_data(surface));
                vector<Mat> Imgs1;
                vector<Mat> Imgs2;
                cv::split(MC3,Imgs2);
                cv::split(MC4,Imgs1);
                for(int i=0;i<3;i++)
                {
                    Imgs2[i]=Imgs1[i];
                }
                cv::merge(Imgs2,MC3);
            }

            static void CairoToMat(cairo_surface_t *surface,Mat &MC3, Mat &MC1)
            {
                Mat MC4 = Mat(cairo_image_surface_get_height(surface),cairo_image_surface_get_width(surface),CV_8UC4,cairo_image_surface_get_data(surface));
                vector<Mat> Imgs1;
                vector<Mat> Imgs2;
                cv::split(MC3,Imgs2);
                cv::split(MC4,Imgs1);
                for(int i=0;i<3;i++)
                {
                    Imgs2[i]=Imgs1[i];
                }
                MC1 = Imgs1[3];
                cv::merge(Imgs2,MC3);
            }

            void addSpots (cairo_surface_t *text, int degree, bool trans, int color){
                RNG rng_;//Randon number generator used for all distributions
                int height = cairo_image_surface_get_height(text);
                int width = cairo_image_surface_get_width(text);

                double prob; //0~100
                unsigned char *data, *data_t;
                int stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, width);
                data = (unsigned char *)malloc(stride * height);
                int pnum = 1+2*degree;
                int xs[pnum];
                int ys[pnum];
                double rads[pnum];

                for (int i=0;i<pnum;i++) {
                    xs[i]=rng_.next()%stride;
                    ys[i]=rng_.next()%height;
                    rads[i]=rng_.next()%(height/(4+2*degree));
                }

                for (int r=0;r<height;r++) {
                    for (int c=0;c<stride;c++) {
                        data[r*stride+c]=0;
                    }
                }

                for (int i=0;i<pnum;i++) {
                    int x = xs[i];
                    int y = ys[i];
                    double rad = rads[i];
                    for (int r=0;r<height;r++) {
                        for (int c=0;c<stride;c++) {
                            double dis = pow(pow((double)(r-y),2)+pow((double)(c-x),2),0.5);
                            prob=100-(100/(1+100*exp(-(dis-rad))));
                            if (rng_.next()%100 < prob) {
                                data[r*stride+c]=255;
                            }
                        }
                    }
                }

                if (trans) {
                    //cairo_surface_flush(text);
                    data_t = cairo_image_surface_get_data(text);
                    int stride_t = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
                    for (int r=0;r<height;r++) {
                        for (int c=0;c<stride;c++) {
                            if (data[r*stride+c] == 255) {
                                if (c*4+3<stride_t) {
                                    data_t[r*stride_t+c*4+3]=0; 
                                }
                            }
                        }
                    }
                    cairo_surface_mark_dirty(text);
                    cairo_surface_flush(text);
                    free(data);
                } else {
                    cairo_surface_t *mask;
                    mask = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_A8, width, height,stride);
                    cairo_t *cr;
                    cr = cairo_create (text);
                    cout << "color " << color << endl;
                    cairo_set_source_rgb(cr,color/255.0,color/255.0,color/255.0);
                    cairo_mask_surface(cr, mask, 0, 0);
                    cout << "ref count " << cairo_get_reference_count(cr) << endl;
                    cairo_destroy (cr);
                    cout << "ref count " << cairo_surface_get_reference_count(mask) << endl;
                    cairo_surface_destroy (mask);
                }

            }

            TextTransformations tt;
            FlowLines fl;

        }//unnamed namespace

        void blendWeighted(Mat& out,Mat& top,Mat& bottom,float topMask,float bottomMask){
            if(out.channels( )==3 && top.channels( )==3 && bottom.channels( )==3 ){
                for(int y=0;y<out.rows;y++){
                    float* outR=out.ptr<float>(y);
                    float* outG=out.ptr<float>(y)+1;
                    float* outB=out.ptr<float>(y)+2;

                    float* topR=top.ptr<float>(y);
                    float* topG=top.ptr<float>(y)+1;
                    float* topB=top.ptr<float>(y)+2;

                    float* bottomR=bottom.ptr<float>(y);
                    float* bottomG=bottom.ptr<float>(y)+1;
                    float* bottomB=bottom.ptr<float>(y)+2;

                    for(int x=0;x<out.cols;x++){
                        int x3=x*3;
                        outR[x3]=topR[x3]*topMask+bottomR[x3]*bottomMask;
                        outG[x3]=topG[x3]*topMask+bottomG[x3]*bottomMask;
                        outB[x3]=topB[x3]*topMask+bottomB[x3]*bottomMask;
                    }
                }
                return;
            }
            if(out.channels( )==1 && top.channels( )==1 && bottom.channels( )==1 ){
                for(int y=0;y<out.rows;y++){
                    float* outG=out.ptr<float>(y);
                    float* topG=top.ptr<float>(y);
                    float* bottomG=bottom.ptr<float>(y);
                    for(int x=0;x<out.cols;x++){
                        outG[x]=topG[x]*topMask+bottomG[x]*bottomMask;
                    }
                }
                return;
            }
            CV_Error(Error::StsError,"Images must all be either CV_32FC1 CV_32FC32");
        }

        void blendWeighted(Mat& out,Mat& top,Mat& bottom,Mat& topMask_,Mat& bottomMask_){
            for(int y=0;y<out.rows;y++){
                float* outR=out.ptr<float>(y);
                float* outG=out.ptr<float>(y)+1;
                float* outB=out.ptr<float>(y)+2;

                float* topR=top.ptr<float>(y);
                float* topG=top.ptr<float>(y)+1;
                float* topB=top.ptr<float>(y)+2;

                float* bottomR=bottom.ptr<float>(y);
                float* bottomG=bottom.ptr<float>(y)+1;
                float* bottomB=bottom.ptr<float>(y)+2;

                float* topMask=topMask_.ptr<float>(y);
                float* bottomMask=bottomMask_.ptr<float>(y);

                for(int x=0;x<out.cols;x++){
                    int x3=x*3;
                    outR[x3]=topR[x3]*topMask[x]+bottomR[x3]*bottomMask[x];
                    outG[x3]=topG[x3]*topMask[x]+bottomG[x3]*bottomMask[x];
                    outB[x3]=topB[x3]*topMask[x]+bottomB[x3]*bottomMask[x];
                }
            }
        }

        void blendOverlay(Mat& out,Mat& top,Mat& bottom,Mat& topMask){
            for(int y=0;y<out.rows;y++){
                float* outR=out.ptr<float>(y);
                float* outG=out.ptr<float>(y)+1;
                float* outB=out.ptr<float>(y)+2;

                float* topR=top.ptr<float>(y);
                float* topG=top.ptr<float>(y)+1;
                float* topB=top.ptr<float>(y)+2;

                float* bottomR=bottom.ptr<float>(y);
                float* bottomG=bottom.ptr<float>(y)+1;
                float* bottomB=bottom.ptr<float>(y)+2;

                float* mask=topMask.ptr<float>(y);

                for(int x=0;x<out.cols;x++){
                    int x3=x*3;
                    outR[x3]=topR[x3]*mask[x]+bottomR[x3]*(1-mask[x]);
                    outG[x3]=topG[x3]*mask[x]+bottomG[x3]*(1-mask[x]);
                    outB[x3]=topB[x3]*mask[x]+bottomB[x3]*(1-mask[x]);
                }
            }
        }

        void blendOverlay(Mat& out,Scalar topCol,Scalar bottomCol,Mat& topMask){
            float topR=float(topCol[0]);
            float topG=float(topCol[1]);
            float topB=float(topCol[2]);

            float bottomR=float(bottomCol[0]);
            float bottomG=float(bottomCol[1]);
            float bottomB=float(bottomCol[2]);

            for(int y=0;y<out.rows;y++){
                float* outR=out.ptr<float>(y);
                float* outG=out.ptr<float>(y)+1;
                float* outB=out.ptr<float>(y)+2;

                float* mask=topMask.ptr<float>(y);

                for(int x=0;x<out.cols;x++){
                    int x3=x*3;
                    outR[x3]=topR*mask[x]+bottomR*(1-mask[x]);
                    outG[x3]=topG*mask[x]+bottomG*(1-mask[x]);
                    outB[x3]=topB*mask[x]+bottomB*(1-mask[x]);
                }
            }
        }

        //probabilities are all accumulative
        TextSynthesizer::TextSynthesizer(int sampleHeight):
            resHeight_(sampleHeight),
            bgProbability_{0.25,0.8},
            stretchProbability_{
                {0.05,0.1,0.4,0.7},
                {0.05,0.1,0.4,0.7},
                {0.2,0.5,0.7,0.9}},
            spacingProbability_{
                {0.05,0.1,0.2,0.6},
                {0.05,0.1,0.4,0.9},
                {0.2,0.7,0.9,0.95}},

            //for three bg types
            curvingProbability_{0.9,0.1,0.2},
            //curvingProbability_{1,1,1},
            italicProbability_{0.8,0.1,0.5},

            weightProbability_{
                {0.1,0.7},
                {0.1,0.5},
                {0.5,0.9}},

            //Blocky, Regular, Cursive
            fontProbability_{
                {0.2,0.9},
                {0.4,0.9},
                {0.05,0.9}},

            //bg features
            bgProbs_{
                //colordiffProb_
                {0.5,0.5,0,0.2},
                //distracttextProb_
                {0.2,0.5,0.8,0.5},
                //boundryProb_
                {0.2,0,0.3,0},
                //colorblobProb_
                {1,1,1,1},
                //straightlineProb_
                {0.2,0.1,0.9,0.2},
                //gridProb_
                {0.1,0,0.3,0},
                //citypointProb_
                {0,0,0,0.2},
                //parallelProb_
                {0,0.4,0,0},
                //vparallelProb_
                {0,0.6,0,0},
                //mountainProb_
                {0.1,0,0.2,0.1},
                //railroadProb_
                {0.2,0,0.3,0.1},
                //riverlineProb_
                {0.95,0,0.5,0.1}
            },
            maxnum_{3,2,5,2}
        {

            //independent properties
            missingProbability_=0.2;
            rotatedProbability_=0.3;


            finalBlendAlpha_=0.9;
            finalBlendProb_=0;


        }

        class TextSynthesizerImpl: public TextSynthesizer{
            protected:
                bool rndProbUnder(double v){
                    return (this->rng_.next()%10000)<(10000*v);
                }

                void updateFontNameList(std::vector<String>& fntList){
                    fntList.clear();

                    PangoFontFamily ** families;
                    int n_families;
                    PangoFontMap * fontmap;

                    fontmap = pango_cairo_font_map_get_default();
                    pango_font_map_list_families (fontmap, & families, & n_families);

                    for (int k = 0; k < n_families; k++) {
                        PangoFontFamily * family = families[k];
                        const char * family_name;
                        family_name = pango_font_family_get_name (family);
                        fntList.push_back(String(family_name));
                    }   
                    g_free (families);
                }

                void setAvailableFonts(std::vector<String>& fntList){
                    std::vector<String> dbList;
                    this->updateFontNameList(dbList);
                    for(size_t k=0;k<fntList.size();k++){
                        if(std::find(dbList.begin(), dbList.end(), fntList[k]) == dbList.end()){
                            CV_Error(Error::StsError,"The font name list must only contain fonts in your system");
                        }
                    }
                    this->availableFonts_=fntList;
                }

                void setBlockyFonts(std::vector<String>& fntList){
                    std::vector<String> dbList=this->availableFonts_;
                    for(size_t k=0;k<fntList.size();k++){
                        if(std::find(dbList.begin(), dbList.end(), fntList[k]) == dbList.end()){
                            CV_Error(Error::StsError,"The font name list must only contain fonts in your system");
                        }
                    }
                    this->blockyFonts_=fntList;
                }

                void setRegularFonts(std::vector<String>& fntList){
                    std::vector<String> dbList=this->availableFonts_;
                    for(size_t k=0;k<fntList.size();k++){
                        if(std::find(dbList.begin(), dbList.end(), fntList[k]) == dbList.end()){
                            CV_Error(Error::StsError,"The font name list must only contain fonts in your system");
                        }
                    }
                    this->regularFonts_=fntList;
                }

                void setCursiveFonts(std::vector<String>& fntList){
                    std::vector<String> dbList=this->availableFonts_;
                    for(size_t k=0;k<fntList.size();k++){
                        if(std::find(dbList.begin(), dbList.end(), fntList[k]) == dbList.end()){
                            CV_Error(Error::StsError,"The font name list must only contain fonts in your system");
                        }
                    }
                    this->cursiveFonts_=fntList;
                }

                void generateFont(char *ret, int fontsize){
                    cout << "before for" << endl;
                    CV_Assert(this->availableFonts_.size());
                    CV_Assert(this->blockyFonts_.size());
                    CV_Assert(this->regularFonts_.size());
                    CV_Assert(this->cursiveFonts_.size());
                    double *fontProb;
                    int font_prob = this->rng_.next()%10000;

                    int bgIndex=static_cast<int>(this->bgType_);
                    fontProb=this->fontProbability_[bgIndex];


                    for (int i=0;i<3;i++) {
                        if(font_prob < 10000*fontProb[i]){
                            cout << "font index " << i << endl;
                            int listsize = this->fonts_[i]->size();
                            cout << "list size " << listsize << endl;
                            CV_Assert(listsize);
                            const char *fnt = this->fonts_[i]->at(rng_.next() % listsize).c_str();
                            strcpy(ret,fnt);
                            break;
                        }

                    }

                    cout << "after for" << endl;

                    //Italic
                    int italic_prob = this->rng_.next()%10000;
                    if(italic_prob < 10000*this->italicProbability_[bgIndex]){
                        strcat(ret," Italic");
                    }
                    strcat(ret," ");
                    std::ostringstream stm;
                    stm << fontsize;
                    strcat(ret,stm.str().c_str());
                }

                void generateTxtPatch(Mat& output,Mat& outputMask,String caption, int text_color, bool distract){
                    size_t len = caption.length();
                    if (this->rndProbUnder(this->rotatedProbability_)){
                        int degree = this->rng_.next()%21-10;
                        cout << "degree " << degree << endl;
                        this->rotatedAngle_=((double)degree/180)*3.14;
                    } else {
                        this->rotatedAngle_=0;
                    }


                    int bgIndex=static_cast<int>(this->bgType_);

                    double curvingProb=this->curvingProbability_[bgIndex];
                    double *spacingProb=this->spacingProbability_[bgIndex];
                    double *stretchProb=this->stretchProbability_[bgIndex];

                    bool curved = false;
                    if(this->rndProbUnder(curvingProb)){
                        curved = true;
                    }

                    //get spacing degree
                    int spacing_prob = this->rng_.next()%10000;
                    double spacing_deg=-0.6+exp(4)/5;
                    for (int i=0;i<4;i++) {
                        if (spacing_prob<spacingProb[i]*10000) {
                            spacing_deg=-0.6+exp((double)i)/5;
                            break;
                        }
                    }

                    cout << "spacing deg " << spacing_deg << endl;

                    //get stretch degree
                    int stretch_prob = this->rng_.next()%10000;
                    double stretch_deg=-0.5+exp(4/4.0);
                    for (int i=0;i<4;i++) {
                        if (stretch_prob<stretchProb[i]*10000) {
                            stretch_deg=-0.5+exp(i/4.0);
                            break;
                        }
                    }

                    cout << "stretch deg " << stretch_deg << endl;

                    double fontsize = (double)this->resHeight_/4*3;
                    double spacing = fontsize/20*spacing_deg;

                    int maxpad=this->resHeight_/10;
                    int x_pad = this->rng_.next()%maxpad-maxpad/2;
                    int y_pad = this->rng_.next()%maxpad-maxpad/2;
                    cout << "pad " << x_pad << " " << y_pad << endl;

                    double scale = (this->rng_.next()%5)/20.0+0.9;
                    cout << "scale " << scale << endl;

                    //Start to draw a pango img
                    cairo_surface_t *surface;
                    cairo_t *cr;

                    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 2000, this->resHeight_);
                    cr = cairo_create (surface);


                    char font[50];
                    cout << "generate font" << endl;
                    this->generateFont(font,(int)fontsize);
                    cout << font << endl;
                    //cout << caption << endl;
                    cairo_set_source_rgb(cr, text_color/255.0,text_color/255.0,text_color/255.0);

                    PangoLayout *layout;
                    PangoFontDescription *desc;

                    layout = pango_cairo_create_layout (cr);

                    //set font destcription
                    desc = pango_font_description_from_string(font);

                    //Weight
                    double *weightProb=this->weightProbability_[bgIndex];
                    int weight_prob = this->rng_.next()%10000;

                    if(weight_prob < 10000*weightProb[0]){
                        pango_font_description_set_weight(desc, PANGO_WEIGHT_LIGHT);
                    } else if(weight_prob < 10000*weightProb[1]){
                        pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
                    } else {
                        pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
                    }

                    pango_layout_set_font_description (layout, desc);

                    int spacing_ = (int)(1024*spacing);
		    
		    std::ostringstream stm;
                    stm << spacing_;
                    string mark = "<span letter_spacing='"+stm.str()+"'>"+caption+"</span>";
                    cout << "mark " << mark << endl;

                    pango_layout_set_markup(layout, mark.c_str(), -1);

                    PangoRectangle *ink_rect = new PangoRectangle;
                    PangoRectangle *logical_rect = new PangoRectangle;
                    pango_layout_get_extents(layout, ink_rect, logical_rect);

                    int ink_x=ink_rect->x/1024;
                    int ink_y=ink_rect->y/1024;
                    int ink_w=ink_rect->width/1024;
                    int ink_h=ink_rect->height/1024;

                    cout << "x " << ink_x << endl;
                    cout << "y " << ink_y << endl;
                    cout << "width " << ink_w << endl;
                    cout << "height " << ink_h << endl;

                    int cur_size = pango_font_description_get_size(desc);
                    cout << "cur size " << cur_size << endl;
                    int size = (int)((double)cur_size/ink_h*this->resHeight_);
                    cout << "size " << size << endl;
                    pango_font_description_set_size(desc, size);
                    pango_layout_set_font_description (layout, desc);

                    pango_layout_get_extents(layout, ink_rect, logical_rect);
                    ink_x=ink_rect->x/1024;
                    ink_y=ink_rect->y/1024;
                    ink_w=ink_rect->width/1024;
                    ink_h=ink_rect->height/1024;

                    cout << "new x " << ink_x << endl;
                    cout << "new y " << ink_y << endl;
                    cout << "new width " << ink_w << endl;
                    cout << "new height " << ink_h << endl;

                    cur_size = pango_font_description_get_size(desc);
                    cout << "new cur size " << cur_size << endl;

                    ink_w=stretch_deg*(ink_w);
                    int patchWidth = (int)ink_w;
                    cout << "patch width " << patchWidth << endl;

                    cout << "after stretch" << endl;
                    if (this->rotatedAngle_!=0) {
                        cout << "rotated angle" << this->rotatedAngle_ << endl;
                        cairo_rotate(cr, this->rotatedAngle_);

                        double sine = abs(sin(this->rotatedAngle_));
                        double cosine = abs(cos(this->rotatedAngle_));

                        double ratio = ink_h/(double)ink_w;
                        double textWidth, textHeight;

                        textWidth=(this->resHeight_/(cosine*ratio+sine));
                        cout << "new width " << textWidth << endl;
                        textHeight = (ratio*textWidth);
                        cout << "new height " << textHeight << endl;
                        patchWidth = (int)ceil(cosine*textWidth+sine*textHeight)+1;
                        cout << "real h " << sine*textWidth+cosine*textHeight << endl;

                        cur_size = pango_font_description_get_size(desc);
                        size = (int)((double)cur_size/ink_h*textHeight);
                        cout << "rotate size " << size << endl;
                        pango_font_description_set_size(desc, size);
                        pango_layout_set_font_description (layout, desc);

                        spacing_ = (int)floor((double)spacing_/ink_h*textHeight);

                        std::ostringstream stm;
                        stm << spacing_;
                        string mark = "<span letter_spacing='"+stm.str()+"'>"+caption+"</span>";
                        cout << "mark " << mark << endl;

                        pango_layout_set_markup(layout, mark.c_str(), -1);

                        double x_off=0, y_off=0;
                        if (this->rotatedAngle_<0) {
                            x_off=-sine*sine*textWidth;
                            y_off=cosine*sine*textWidth;
                        } else {
                            x_off=cosine*sine*textHeight;
                            y_off=-sine*sine*textHeight;
                        }   
                        cairo_translate (cr, x_off, y_off);

                        cairo_scale(cr, stretch_deg, 1);

                        double height_ratio=textHeight/ink_h;
                        y_off=(ink_y*height_ratio);
                        x_off=(ink_x*height_ratio);
                        cairo_translate (cr, -x_off, -y_off);
                        pango_cairo_show_layout (cr, layout);
                    } else if (curved && patchWidth > 10*this->resHeight_ && spacing_deg>=10) {
                        cairo_scale(cr, stretch_deg, 1);
                        cairo_path_t *path;
                        PangoLayoutLine *line;
                        cout << "before tt" << endl;
                        tt.create_curved_path(cr,path,line,layout,(double)patchWidth,(double)this->resHeight_,0,0,this->rng_.next()%3+3,this->rng_.next());

                        cout << "after tt" << endl;
                        double x1,x2,y1,y2;
                        cairo_path_extents(cr,&x1,&y1,&x2,&y2);
                        cout << "x1 " << x1 << endl;
                        cout << "y1 " << y1 << endl;
                        cout << "x2 " << x2 << endl;
                        cout << "y2 " << y2 << endl;

                        cairo_path_t *path_n=cairo_copy_path(cr);
                        cairo_new_path(cr);
                        cairo_path_data_t *path_data_n;
                        fl.manual_translate(cr, path_n, path_data_n, -x1, -y1);

                        cairo_path_extents(cr,&x1,&y1,&x2,&y2);
                        cout << "x1 " << x1 << endl;
                        cout << "y1 " << y1 << endl;
                        cout << "x2 " << x2 << endl;
                        cout << "y2 " << y2 << endl;

                        path_n=cairo_copy_path(cr);
                        cairo_new_path(cr);

                        cairo_surface_t *surface_c;
                        cairo_t *cr_c;

                        surface_c = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, (int)ceil(x2-x1), (int)ceil(y2-y1));
                        cr_c = cairo_create (surface_c);
                        cairo_new_path(cr_c);
                        cairo_append_path(cr_c,path_n);
                        cairo_fill(cr_c);

                        double ratio = this->resHeight_/(y2-y1);
                        cout << "ratio " << ratio << endl;
                        cairo_scale(cr,ratio,ratio);
                        patchWidth=(int)(ceil((x2-x1)*ratio)*stretch_deg);
                        cairo_set_source_surface(cr, surface_c, 0, 0);
                        cairo_rectangle(cr, 0, 0, x2-x1, y2-y1);
                        cairo_fill(cr);
                        cairo_destroy (cr_c);
                        cairo_surface_destroy (surface_c);
                    } else {
                        cairo_scale(cr, stretch_deg, 1);
                        cairo_translate (cr, -ink_x, -ink_y);
                        pango_cairo_show_layout (cr, layout);
                    }

                    //free layout
                    cout << "freeing" << endl;
                    g_object_unref(layout);
                    pango_font_description_free (desc);
                    free(logical_rect);
                    free(ink_rect);

                    cairo_identity_matrix(cr);

                    cout << "destroy cr" << endl;
                    cout << "ref count " << cairo_get_reference_count(cr) << endl;
                    cairo_destroy (cr);

                    cairo_surface_t *surface_n;
                    cairo_t *cr_n;

                    surface_n = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, patchWidth, this->resHeight_);
                    cr_n = cairo_create (surface_n);

                    //apply arbitrary padding and scaling
                    cairo_translate (cr_n, x_pad, y_pad);

                    cairo_translate (cr_n, patchWidth/2, this->resHeight_/2);
                    cairo_scale(cr_n, scale, scale);
                    cairo_translate (cr_n, -patchWidth/2, -this->resHeight_/2);

                    cairo_set_source_surface(cr_n, surface, 0, 0);
                    cairo_rectangle(cr_n, 0, 0, patchWidth, this->resHeight_);
                    cairo_fill(cr_n);

                    cairo_identity_matrix(cr_n);

                    cairo_set_source_rgb(cr_n, text_color/255.0,text_color/255.0,text_color/255.0);
                    //draw distracting text
                    if (distract) {
                        char font2[50];
                        int shrink = this->rng_.next()%3+2;
                        this->generateFont(font2,size/1024/shrink);
                        tt.distractText(cr_n, patchWidth, this->resHeight_, font2, time(NULL));
                    }

                    cout << "destroy cr_n" << endl;
                    cout << "ref count " << cairo_get_reference_count(cr_n) << endl;
                    cairo_destroy (cr_n);
                    cout << "destroy surface" << endl;
                    cout << "ref count " << cairo_surface_get_reference_count(surface) << endl;
                    cairo_surface_destroy (surface);

                    cout << "add spots" << endl;
                    if(this->rndProbUnder(this->missingProbability_)){
                        addSpots(surface_n,2,true,0);
                    }

                    cout << "after spots" << endl;
                    //cairo_surface_write_to_png (surface, "/home/chenziwe/aaaaa.png");

                    Mat textImg, maskImg;
                    cout << "create mats" << endl;
                    textImg =cv::Mat(this->resHeight_,patchWidth,CV_8UC3,Scalar_<uchar>(0,0,0));
                    maskImg =cv::Mat(this->resHeight_,patchWidth,CV_8UC1,Scalar(0));

                    cout << "converting to mat" << endl;
                    CairoToMat(surface_n,textImg,maskImg);

                    cout << "destroy surface_n" << endl;
                    cout << "ref count " << cairo_surface_get_reference_count(surface_n) << endl;
                    cairo_surface_destroy (surface_n);

                    cout << "after destroy stuff" << endl;

                    textImg.copyTo(output);
                    maskImg.copyTo(outputMask);

                }

                void addGaussianNoise(Mat& out){
                    double sigma = (rng_.next()%10+1)/100.0;

                    Mat noise = Mat(out.rows, out.cols, CV_32F);
                    randn(noise, 0, sigma);
                    vector<Mat> chs;
                    cv::split(out,chs);

                    for (int i=0;i<3;i++) {
                        chs[i]+=noise;
                        threshold(chs[i],chs[i],1.0,1.0,THRESH_TRUNC);
                        threshold(chs[i],chs[i],0,1.0,THRESH_TOZERO);
                    }

                    merge(chs,out);
                }

                void addGaussianBlur(Mat& out){
                    int ker_size = rng_.next()%2*2+1;
                    GaussianBlur(out,out,Size(ker_size,ker_size),0,0,BORDER_REFLECT_101);
                }

                void addBgBias(cairo_t *cr, int width, int height, int color){

                    cairo_pattern_t *pat = cairo_pattern_create_linear(width/2,0,width/2,height);
                    cairo_pattern_t *pat2 = cairo_pattern_create_linear(0,height/2,width,height/2);
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

                void generateBgSample(CV_OUT Mat& sample, std::vector<BGFeature> &features, int width, int bg_color, int contrast){

                    cairo_surface_t *surface;
                    cairo_t *cr;
                    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, this->resHeight_);
                    cr = cairo_create (surface);

                    cairo_set_source_rgb(cr, bg_color/255.0,bg_color/255.0,bg_color/255.0);
                    cairo_paint (cr);

                    if (find(features.begin(), features.end(), Colordiff)!= features.end()) {
                        double color1 = (bg_color-this->rng_.next()%(contrast/2))/255.0;
                        double color2 = (bg_color-this->rng_.next()%(contrast/2))/255.0;
                        tt.colorDiff(cr, width, this->resHeight_, time(NULL), color1, color2);
                    }
                    if (find(features.begin(), features.end(), Colorblob)!= features.end()) {
                        addSpots(surface,0,false,bg_color-this->rng_.next()%contrast);
                    }

                    //add bg bias field
                    addBgBias(cr, width, this->resHeight_, bg_color);

                    if (find(features.begin(), features.end(), Parallel)!= features.end()) {
                        double color = (bg_color-this->rng_.next()%contrast)/255.0;
                        cairo_set_source_rgb(cr,color,color,color);
                        tt.addBgPattern(cr, width, this->resHeight_, true, false, true, time(NULL));
                    }
                    if (find(features.begin(), features.end(), Vparallel)!= features.end()) {
                        double color = (bg_color-this->rng_.next()%contrast)/255.0;
                        cairo_set_source_rgb(cr,color,color,color);
                        tt.addBgPattern(cr, width, this->resHeight_, false, false, true, time(NULL));
                    }
                    if (find(features.begin(), features.end(), Grid)!= features.end()) {
                        double color = (bg_color-this->rng_.next()%contrast)/255.0;
                        cairo_set_source_rgb(cr,color,color,color);
                        tt.addBgPattern(cr, width, this->resHeight_, true, true, false, time(NULL));
                    }
                    if (find(features.begin(), features.end(), Railroad)!= features.end()) {
                        double color = (bg_color-this->rng_.next()%contrast)/255.0;
                        int line_num = this->rng_.next()%2+1;
                        for (int i=0;i<line_num;i++) {
                            fl.addLines(cr, false, true, false, true, false, (bool)this->rng_.next()%2, time(NULL), width, this->resHeight_, color);
                        }
                    }
                    if (find(features.begin(), features.end(), Boundry)!= features.end()) {
                        double color = (bg_color-this->rng_.next()%contrast)/255.0;
                        int line_num = this->rng_.next()%2+1;
                        for (int i=0;i<line_num;i++) {
                            fl.addLines(cr, true, false, (bool)this->rng_.next()%2, true, false, (bool)this->rng_.next()%2, time(NULL), width, this->resHeight_, color);
                        }
                    }
                    if (find(features.begin(), features.end(), Straight)!= features.end()) {
                        double color = (bg_color-this->rng_.next()%contrast)/255.0;
                        int line_num = this->rng_.next()%2+1;
                        for (int i=0;i<line_num;i++) {
                            fl.addLines(cr, false, false, false, false, false, (bool)this->rng_.next()%2, time(NULL), width, this->resHeight_, color);
                        }
                    }
                    if (find(features.begin(), features.end(), Riverline)!= features.end()) {
                        double color = (bg_color-this->rng_.next()%contrast)/255.0;
                        int line_num = this->rng_.next()%2+1;
                        for (int i=0;i<line_num;i++) {
                            fl.addLines(cr, false, false, false, true, (bool)this->rng_.next()%2, (bool)this->rng_.next()%2, time(NULL), width, this->resHeight_, color);
                        }
                    }

                    Mat res=cv::Mat(this->resHeight_,width,CV_8UC3,Scalar_<uchar>(0,0,0));
                    CairoToMat(surface,res);
                    res.copyTo(sample);
                    cout << "ref count " << cairo_get_reference_count(cr) << endl;
                    cairo_destroy (cr);
                    cout << "ref count " << cairo_surface_get_reference_count(surface) << endl;
                    cairo_surface_destroy (surface);
                }

                RNG rng_;//Randon number generator used for all distributions
                std::vector<String> blockyFonts_;
                std::vector<String> regularFonts_;
                std::vector<String> cursiveFonts_;
                std::vector<String> availableFonts_;

                std::shared_ptr<std::vector<String> > fonts_[3];
                std::vector<String> sampleCaptions_;

            public:
                TextSynthesizerImpl(
                        int sampleHeight = 50,
                        uint64 rndState = 0)
                    : TextSynthesizer(sampleHeight),
                    rng_(rndState != 0 ? rndState:std::time(NULL)),
                    fonts_{std::shared_ptr<std::vector<String> >(&(this->blockyFonts_)),
                          std::shared_ptr<std::vector<String> >(&(this->regularFonts_)),
                          std::shared_ptr<std::vector<String> >(&(this->cursiveFonts_))}{
                              namedWindow("__w");
                              waitKey(1);
                              destroyWindow("__w");
                              this->updateFontNameList(this->availableFonts_);
                          }

                void generateTxtSample(CV_OUT String &caption, CV_OUT Mat& sample,CV_OUT Mat& sampleMask, int text_color, bool distract){
                    if(sampleCaptions_.size()!=0){
                        caption = sampleCaptions_[this->rng_.next()%sampleCaptions_.size()];
                        cout << "generating text patch" << endl;
                        generateTxtPatch(sample,sampleMask,caption,text_color,distract);
                    } else {
                        int len = rand()%10+1;
                        char text[len+1];
                        for (int i=0;i<len;i++) {
                            text[i]=tt.randomChar(rand());
                        }   
                        text[len]='\0';
                        caption=String(text);
                        generateTxtPatch(sample,sampleMask,caption,text_color,distract);
                    }

                }

                void generateBgFeatures(vector<BGFeature> &bgFeatures){
                    int index=static_cast<int>(this->bgType4_);
                    int maxnum=this->maxnum_[index];

                    std::vector<BGFeature> allFeatures={Colordiff, Distracttext, Boundry, Colorblob, Straight, Grid, Citypoint, Parallel, Vparallel, Mountain, Railroad, Riverline};
                    for (int i=0;i<maxnum;i++){
                        bool flag = true;
                        while (flag) {
                            int j = this->rng_.next()%allFeatures.size();
                            BGFeature cur = allFeatures[j];
                            allFeatures.erase(allFeatures.begin()+j);
                            if (cur==Vparallel && find(bgFeatures.begin(), bgFeatures.end(), Parallel)!= bgFeatures.end()) continue;
                            if (cur==Parallel && find(bgFeatures.begin(), bgFeatures.end(), Vparallel)!= bgFeatures.end()) continue;
                            if (cur==Colordiff && find(bgFeatures.begin(), bgFeatures.end(), Colorblob)!= bgFeatures.end()) continue;
                            if (cur==Colorblob && find(bgFeatures.begin(), bgFeatures.end(), Colordiff)!= bgFeatures.end()) continue;

                            flag=false;

                            int curIndex=static_cast<int>(cur);
                            if(this->rndProbUnder(this->bgProbs_[curIndex][index])){
                                bgFeatures.push_back(cur);
                            }
                        }
                    }
                }

                void generateSample(CV_OUT String &caption, CV_OUT Mat & sample){
                    int bg_prob = this->rng_.next()%10000;
                    if (bg_prob<10000*this->bgProbability_[0]){
                        this->bgType_=Water;
                        this->bgType4_=Flow;
                    } else if (bg_prob<10000*this->bgProbability_[1]){
                        this->bgType_=Bigland;
                        if (this->rng_.next()%2==0){
                            this->bgType4_=Waterbody;
                        } else {
                            this->bgType4_=Small;
                        }
                    } else {
                        this->bgType_=Smallland;
                        this->bgType4_=Small;
                    }

                    std::vector<BGFeature> bgFeatures;
                    generateBgFeatures(bgFeatures);

                    int bg_brightness = 255-this->rng_.next()%100;
                    int text_color = this->rng_.next()%50;
                    int contrast = bg_brightness - text_color;

                    Mat txtSample;
                    Mat bgSample;
                    Mat bgResized;
                    Mat txtMask;
                    Mat txtMerged;
                    Mat floatBg;
                    Mat floatTxt;
                    Mat floatMask;

                    std::vector<Mat> txtChannels;
                    //cout << "generating text sample" << endl;
                    if (find(bgFeatures.begin(), bgFeatures.end(), Distracttext)!= bgFeatures.end()) {
                        generateTxtSample(caption, txtSample,txtMask,text_color, true);
                    } else {
                        generateTxtSample(caption, txtSample,txtMask,text_color, false);
                    }
                    //cout << "finished generating text sample" << endl;

                    //cout << "generating bg sample" << endl;
                    cout << "bg feature num " << bgFeatures.size() << endl; 

                    generateBgSample(bgSample, bgFeatures, txtSample.cols, bg_brightness, contrast);
                    //cout << "finished generating bg sample" << endl;

                    bgSample.convertTo(floatBg, CV_32FC3, 1.0/255.0);
                    txtSample.convertTo(floatTxt, CV_32FC3, 1.0/255.0);
                    txtMask.convertTo(floatMask, CV_32FC1, 1.0/255.0);
                    bgResized=floatBg;

                    sample=Mat(txtSample.rows,txtSample.cols,CV_32FC3);

                    blendOverlay(sample,floatTxt,bgResized,floatMask);

                    float blendAlpha=float(this->finalBlendAlpha_*(this->rng_.next()%1000)/1000.0);
                    if(this->rndProbUnder(this->finalBlendProb_)){
                        blendWeighted(sample,sample,bgResized,1-blendAlpha,blendAlpha);
                    }

                    addGaussianNoise(sample);
                    addGaussianBlur(sample);
                }

                std::vector<String> listAvailableFonts() const {
                    std::vector<String> res;
                    res=this->availableFonts_;
                    return res;
                }

                void setSampleCaptions (std::vector<String>& words) {
                    this->sampleCaptions_ = words;
                }

                //TOOODOOOO
                void addFontFiles(const std::vector<cv::String>& fntList){
                    this->updateFontNameList(this->availableFonts_);
                }

        };

        Ptr<TextSynthesizer> TextSynthesizer::create(int sampleHeight){
            Ptr<TextSynthesizer> res(new TextSynthesizerImpl(sampleHeight));
            return res;
        }

    }  //namespace text
}  //namespace cv
