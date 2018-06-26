#include "precomp.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/calib3d.hpp"

#include "opencv2/text/text_synthesizer.hpp"
#include "opencv2/text/text_transformations.hpp"

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


//TODO FIND apropriate
#define CV_IMWRITE_JPEG_QUALITY 1
#define CV_LOAD_IMAGE_COLOR 1

using namespace std;

namespace cv{
    namespace text{

        namespace {
            //Unnamed namespace with auxiliary classes and functions used for quick computation
            template <typename T> T min_ (T v1, T v2) {
                return (v1 < v2) * v1 + (v1 >= v2) * v2;
            }

            template <typename T> T max_(T v1, T v2) {
                return (v1 > v2)* v1 + (v1 <= v2) * v2;
            }

            /*
               template <typename P,typename BL_A,typename BL> void blendRGBA(Mat& out, const Mat &in1, const Mat& in2){
               CV_Assert (out.cols == in1.cols && out.cols == in2.cols);
               CV_Assert (out.rows == in1.rows && out.rows == in2.rows);
               CV_Assert (out.channels() == 4 && in1.channels() == 4 && in2.channels() == 4);
               int lineWidth=out.cols * 4;
               BL blend;
               BL_A blendA;
               for(int y = 0; y < out.rows; y++){
               const P* in1B = in1.ptr<P> (y) ;
               const P* in1G = in1.ptr<P> (y) + 1;
               const P* in1R = in1.ptr<P> (y) + 2;
               const P* in1A = in1.ptr<P> (y) + 3;

               const P* in2B = in2.ptr<P> (y);
               const P* in2G = in2.ptr<P> (y) + 1;
               const P* in2R = in2.ptr<P> (y) + 2;
               const P* in2A = in2.ptr<P> (y) + 3;

               P* outB = out.ptr<P> (y);
               P* outG = out.ptr<P> (y) + 1;
               P* outR = out.ptr<P> (y) + 2;
               P* outA = out.ptr<P> (y) + 3;

               for(int x = 0; x < lineWidth; x += 4){
               outB[x] = blend(in1B + x, in1A + x, in2B + x, in2A + x);
               outG[x] = blend(in1G + x, in1A + x, in2G + x, in2A + x);
               outR[x] = blend(in1R + x, in1A + x, in2R + x, in2A + x);
               outA[x] = blendA(in1A[x], in2A[x]);
               }
               }
               }
             */
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

            /*
             * Make a stretching filter over the active mask.
             * Be sure to deactivate it with cairo_identity_matrix 
             * when you no longer wish to stretch all drawing.
             * 
             * cr - cairo context
             * xfactor - the stretch factor in x direction
             * yfactor - the stretch factor in y direction
             */
            static void activate_stretch_filter (cairo_t *cr, double xfactor, double yfactor) {

                //set transformation matrix
                cairo_matrix_t matrix;
                cairo_matrix_init(&matrix, xfactor, 0, 0, yfactor, 0, 0);
                //  stretching horizontal/vertical matrix (or both if desired)
                //  x 0
                //  0 y
                cairo_transform(cr, &matrix);
            }

            void addNoise (cairo_surface_t *text, int degree){
                RNG rng_;//Randon number generator used for all distributions
                int height = cairo_image_surface_get_height(text);
                int width = cairo_image_surface_get_width(text);
                int prob = degree * 5; //0~50
                unsigned char *data;
                data = cairo_image_surface_get_data(text);

                cairo_surface_flush(text);
                unsigned char tmp;
                int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
                for (int r=0;r<height;r++) {
                    for (int c=0;c<stride;c+=4) {
                        if (rng_.next()%100<prob && data[(r*stride+c)+3]!=0){
                            tmp = rng_.next()%56; 
                            data[r*stride+c]+=tmp;
                            data[r*stride+c+1]+=tmp;
                            data[r*stride+c+2]+=tmp;
                        }
                    }
                }
                cairo_surface_mark_dirty(text);
                cairo_surface_flush(text);
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
                            prob=100-(100/(1+100*pow(M_E,-(dis-rad))));
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


        }//unnamed namespace

        void blendWeighted(Mat& out,Mat& top,Mat& bottom,float topMask,float bottomMask);
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

        void blendWeighted(Mat& out,Mat& top,Mat& bottom,Mat& topMask_,Mat& bottomMask_);
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

        void blendOverlay(Mat& out,Mat& top,Mat& bottom,Mat& topMask);
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

        void blendOverlay(Mat& out,Scalar topCol,Scalar bottomCol,Mat& topMask);
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
            bgProbability_{0.33,0.66},
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
            colordiffProb_{0.5,0.5,0,0.2},
            distracttextProb_{0,0,0.5,0},
            //distracttextProb_{1,1,1,1},
            boundryProb_{0.2,0,0.3,0},
            //colorblobProb_{0.1,0,0,0.1},
            colorblobProb_{1,1,1,1},
            gridProb_{0.1,0,0.3,0},
            straightlineProb_{0.2,0.1,0.9,0.2},
            citypointProb_{0,0,0,0.2},
            parallelProb_{0,0.4,0,0},
            vparallelProb_{0,0.6,0,0},
            mountainProb_{0.1,0,0.2,0.1},
            railroadProb_{0.2,0,0.3,0.1},
            riverlineProb_{0.95,0,0.5,0.1}
        {
            resWidth_=1000;


            //independent properties
            missingProbability_=0.2;
            noiseProbability_=0.3;
            rotatedProbability_=0.3;


            finalBlendAlpha_=0.9;
            finalBlendProb_=0;

            //max num of bg features each bg4 type can have
            flow_n=3;
            water_n=2;
            bigland_n=5;
            smallland_n=2;

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
                    CV_Assert(this->availableFonts_.size());
                    CV_Assert(this->blockyFonts_.size());
                    CV_Assert(this->regularFonts_.size());
                    CV_Assert(this->cursiveFonts_.size());
                    double *fontProb;
                    int font_prob = this->rng_.next()%10000;

                    if (this->bgType_==Water) {
                        fontProb=this->fontProbability_[0];
                    } else if (this->bgType_==Bigland) {
                        fontProb=this->fontProbability_[1];
                    } else {
                        fontProb=this->fontProbability_[2];
                    }

                    if(font_prob < 10000*fontProb[0]){
                        const char *fnt = this->blockyFonts_[rng_.next() % this->blockyFonts_.size()].c_str();
                        strcpy(ret,fnt);
                    } else if(font_prob < 10000*fontProb[1]){
                        const char *fnt = this->regularFonts_[rng_.next() % this->regularFonts_.size()].c_str();
                        strcpy(ret,fnt);
                    } else {
                        const char *fnt = this->cursiveFonts_[rng_.next() % this->cursiveFonts_.size()].c_str();
                        strcpy(ret,fnt);
                    }

                    //Italic
                    int italic_prob = this->rng_.next()%10000;
                    if (this->bgType_==Water) {
                        if(italic_prob < 10000*this->italicProbability_[0]){
                            strcat(ret," Italic");
                        }
                    } else if (this->bgType_==Bigland) {
                        if(italic_prob < 10000*this->italicProbability_[1]){
                            strcat(ret," Italic");
                        }
                    } else {
                        if(italic_prob < 10000*this->italicProbability_[2]){
                            strcat(ret," Italic");
                        }
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


                    int spacing_prob = this->rng_.next()%10000;
                    int stretch_prob = this->rng_.next()%10000;

                    double *spacingProb;
                    double *stretchProb;
                    double curvingProb;
                    if (this->bgType_==Water) {
                        spacingProb=this->spacingProbability_[0];
                        stretchProb=this->stretchProbability_[0];
                        curvingProb=this->curvingProbability_[0];
                    }
                    else if (this->bgType_==Bigland) {
                        spacingProb=this->spacingProbability_[1];
                        stretchProb=this->stretchProbability_[1];
                        curvingProb=this->curvingProbability_[1];
                    } else {
                        spacingProb=this->spacingProbability_[2];
                        stretchProb=this->stretchProbability_[2];
                        curvingProb=this->curvingProbability_[2];
                    } 


                    bool curved = false;
                    if(this->rndProbUnder(curvingProb)){
                        curved = true;
                    }


                    //get spacing degree
                    double spacing_deg;
                    if (spacing_prob<10000*spacingProb[0]){
                        spacing_deg=-0.5;
                    } else if (spacing_prob<10000*spacingProb[1]){
                        spacing_deg=-0.8;
                    } else if (spacing_prob<10000*spacingProb[2]){
                        spacing_deg=0;
                    } else if (spacing_prob<10000*spacingProb[3]){
                        spacing_deg=1;
                    } else {
                        spacing_deg=2;
                    }
                    cout << "spacing deg " << spacing_deg << endl;

                    //get stretch degree
                    double stretch_deg;
                    if (stretch_prob<10000*stretchProb[0]){
                        stretch_deg=0.7;
                    } else if (stretch_prob<10000*stretchProb[1]){
                        stretch_deg=0.8;
                    } else if (stretch_prob<10000*stretchProb[2]){
                        stretch_deg=1;
                    } else if (stretch_prob<10000*stretchProb[3]){
                        stretch_deg=1.2;
                    } else{
                        stretch_deg=1.5;
                    }
                    cout << "stretch deg " << stretch_deg << endl;

                    double fontsize = (double)this->resHeight_/4*3;
                    double spacing = fontsize/20*spacing_deg;

                    //Start to draw a pango img
                    cairo_surface_t *surface;
                    cairo_t *cr;

                    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1000, this->resHeight_);
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
                    double *weightProb;
                    int weight_prob = this->rng_.next()%10000;
                    if (this->bgType_==Water) {
                        weightProb=this->weightProbability_[0];
                    } else if (this->bgType_==Bigland) {
                        weightProb=this->weightProbability_[1];
                    } else {
                        weightProb=this->weightProbability_[2];
                    }

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
                    int size = (int)((double)cur_size/ink_h*this->resHeight_/10*9);
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

                    double textWidth, textHeight;
                    ink_w=stretch_deg*(ink_w);
                    int patchWidth = (int)ink_w;
                    cout << "patch width " << patchWidth << endl;

                    activate_stretch_filter(cr, stretch_deg, 1);
                    cout << "after stretch" << endl;
                    if (this->rotatedAngle_!=0) {
                        cout << "rotated angle" << this->rotatedAngle_ << endl;
                        cairo_rotate(cr, this->rotatedAngle_);

                        double sine = abs(sin(this->rotatedAngle_));
                        double cosine = abs(cos(this->rotatedAngle_));
                        double tangent = abs(tan(this->rotatedAngle_));
                        double cotan = 1/tangent;

                        double ratio = ink_h/(double)ink_w;
                        textWidth=this->resHeight_/(cosine*ratio+sine);
                        cout << "new width " << textWidth << endl;
                        textHeight = ratio*textWidth;
                        cout << "new height " << textHeight << endl;
                        patchWidth = (int)(cosine*textWidth+sine*textHeight)/3*4;

                        cur_size = pango_font_description_get_size(desc);
                        size = (int)((double)cur_size/ink_h*textHeight);
                        cout << "rotate size " << size << endl;
                        pango_font_description_set_size(desc, size);
                        pango_layout_set_font_description (layout, desc);

                        int x_off=0, y_off=0;
                        if (this->rotatedAngle_<0) {
                            x_off=-(int)(sine*sine*textWidth);
                            y_off=(int)(cosine*sine*textWidth);
                        } else {
                            x_off=(int)(cosine*sine*textHeight);
                            y_off=-(int)(sine*sine*textHeight);
                        }   
                        cairo_translate (cr, x_off, y_off);

                        double height_ratio=textHeight/ink_h;
                        ink_y=ink_y*height_ratio+1;
                        ink_x=ink_x*height_ratio+1;
                        cairo_translate (cr, -ink_x, -ink_y);
                        pango_cairo_show_layout (cr, layout);
                    } else if (curved) {
                        cairo_path_t *path;
                        PangoLayoutLine *line;
                        cur_size = pango_font_description_get_size(desc);
                        size = (int)((double)cur_size/10*8);
                        cout << "curved size " << size << endl;
                        pango_font_description_set_size(desc, size);
                        pango_layout_set_font_description (layout, desc);
                        if (this->rng_.next()%2==0) {
                            cout << "before tt" << endl;
                            tt.create_curved_path(cr,path,line,layout,(double)patchWidth,(double)this->resHeight_,-ink_x/10.0*8+this->resHeight_/5.0,-ink_y/10.0*8,4,time(NULL));
                        } else {
                            tt.create_curved_path(cr,path,line,layout,(double)patchWidth,(double)this->resHeight_,-ink_x/10.0*8+this->resHeight_/5.0,-ink_y/10.0*8,3,time(NULL));
                        }
                        cout << "after tt" << endl;
                        cairo_fill_preserve (cr);
                    } else {
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

                    //draw distracting text
                    if (distract) {
                        char font2[50];
                        int shrink = this->rng_.next()%3+2;
                        this->generateFont(font2,size/1024/shrink);
                        tt.distractText(cr, patchWidth, this->resHeight_, font2, time(NULL));
                    }

                    cout << "destroy cr" << endl;
                    cout << "ref count " << cairo_get_reference_count(cr) << endl;
                    cairo_destroy (cr);

                    cairo_surface_t *surface_n;
                    cairo_t *cr_n;

                    surface_n = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, patchWidth, this->resHeight_);
                    cr_n = cairo_create (surface_n);

                    cairo_set_source_surface(cr_n, surface, 0, 0);
                    cairo_rectangle(cr_n, 0, 0, patchWidth, this->resHeight_);
                    cairo_fill(cr_n);

                    cout << "destroy cr_n" << endl;
                    cout << "ref count " << cairo_get_reference_count(cr_n) << endl;
                    cairo_destroy (cr_n);
                    cout << "destroy surface" << endl;
                    cout << "ref count " << cairo_surface_get_reference_count(surface) << endl;
                    cairo_surface_destroy (surface);

                    cout << "adding noise" << endl;
                    if(this->rndProbUnder(this->noiseProbability_)){
                        addNoise(surface_n,1);
                    }

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

                    Mat res=cv::Mat(this->resHeight_,width,CV_8UC3,Scalar_<uchar>(0,0,0));
                    CairoToMat(surface,res);
                    res.copyTo(sample);
                    cout << "ref count " << cairo_get_reference_count(cr) << endl;
                    cairo_destroy (cr);
                    cout << "ref count " << cairo_surface_get_reference_count(surface) << endl;
                    cairo_surface_destroy (surface);
                }

                RNG rng_;//Randon number generator used for all distributions
                int txtPad_;
                std::vector<String> blockyFonts_;
                std::vector<String> regularFonts_;
                std::vector<String> cursiveFonts_;
                std::vector<String> availableFonts_;
                std::vector<String> sampleCaptions_;

            public:
                TextSynthesizerImpl(
                        int sampleHeight = 50,
                        uint64 rndState = 0)
                    : TextSynthesizer(sampleHeight)
                      , rng_(rndState != 0 ? rndState:std::time(NULL))
                          , txtPad_(1) {
                              namedWindow("__w");
                              waitKey(1);
                              destroyWindow("__w");

                              this->updateFontNameList(this->availableFonts_);
                          }

                void getRandomSeed (OutputArray res) const {
                    Mat tmpMat(1,8,CV_8UC1);
                    tmpMat.ptr<uint64>(0)[0] = this->rng_.state;
                    tmpMat.copyTo(res);
                }

                void setRandomSeed (Mat state) {
                    CV_Assert (state.rows == 1 && state.cols == 8);
                    CV_Assert (state.depth() == CV_8U && state.channels() == 1);
                    this->rng_.state=state.ptr<uint64>(0)[0];
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
                    int maxnum=0;
                    int index=0;
                    switch (this->bgType4_) {
                        case (Flow):
                            maxnum=this->flow_n;
                            index=0;
                            break;
                        case (Waterbody):
                            maxnum=this->water_n;
                            index=1;
                            break;
                        case (Big):
                            maxnum=this->bigland_n;
                            index=2;
                            break;
                        case (Small):
                            maxnum=this->smallland_n;
                            index=3;
                            break;
                        default:
                            break;
                    }
                    std::vector<BGFeature> allFeatures={Colordiff, Distracttext, Boundry, Colorblob, Straight, Grid, Citypoint, Parallel, Vparallel, Mountain, Railroad, Riverline};
                    for (int i=0;i<maxnum;i++){
                        bool flag = true;
                        while (flag) {
                            int j = this->rng_.next()%allFeatures.size();
                            BGFeature cur = allFeatures[j];
                            allFeatures.erase(allFeatures.begin()+j);
                            if (allFeatures[j]==Vparallel && find(bgFeatures.begin(), bgFeatures.end(), Parallel)!= bgFeatures.end()) continue;
                            if (allFeatures[j]==Parallel && find(bgFeatures.begin(), bgFeatures.end(), Vparallel)!= bgFeatures.end()) continue;
                            if (allFeatures[j]==Colordiff && find(bgFeatures.begin(), bgFeatures.end(), Colorblob)!= bgFeatures.end()) continue;
                            if (allFeatures[j]==Colorblob && find(bgFeatures.begin(), bgFeatures.end(), Colordiff)!= bgFeatures.end()) continue;

                            flag=false;
                            switch (cur) {
                                case (Colordiff):
                                    if(this->rndProbUnder(this->colordiffProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                case (Distracttext):
                                    if(this->rndProbUnder(this->distracttextProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                case (Boundry):
                                    if(this->rndProbUnder(this->boundryProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                case (Colorblob):
                                    if(this->rndProbUnder(this->colorblobProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                case (Straight):
                                    if(this->rndProbUnder(this->straightlineProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                case (Grid):
                                    if(this->rndProbUnder(this->gridProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                case (Citypoint):
                                    if(this->rndProbUnder(this->citypointProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                case (Parallel):
                                    if(this->rndProbUnder(this->parallelProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                case (Vparallel):
                                    if(this->rndProbUnder(this->vparallelProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                case (Mountain):
                                    if(this->rndProbUnder(this->mountainProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                case (Railroad):
                                    if(this->rndProbUnder(this->railroadProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                case (Riverline):
                                    if(this->rndProbUnder(this->riverlineProb_[index])){
                                        bgFeatures.push_back(cur);
                                    }
                                    break;
                                default:
                                    break;
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
