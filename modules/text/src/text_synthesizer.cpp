#include "precomp.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/calib3d.hpp"

#include "opencv2/text/text_synthesizer.hpp"

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
            void MatToCairo(Mat &MC3,cairo_surface_t *surface)
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

            void CairoToMat(cairo_surface_t *surface,Mat &MC3, Mat &MC1)
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

            static void draw_text (cairo_t *cr, const char *font, const char *text, int x, int y){

                PangoLayout *layout;
                PangoFontDescription *desc;

                /* Center coordinates on the middle of the region we are drawing
                 */
                cairo_translate (cr, x, y);

                /* Create a PangoLayout, set the font and text */
                layout = pango_cairo_create_layout (cr);

                pango_layout_set_text (layout, text, -1);
                desc = pango_font_description_from_string (font);
                pango_layout_set_font_description (layout, desc);
                pango_font_description_free (desc);

                cairo_save (cr);

                //Just black
                cairo_set_source_rgb (cr, 0, 0, 0);
                pango_cairo_show_layout (cr, layout);

                cairo_restore (cr);

                /* free the layout object */
                g_object_unref (layout);
            }

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


        TextSynthesizer::TextSynthesizer(int maxSampleWidth,int sampleHeight):
            resHeight_(sampleHeight),maxResWidth_(maxSampleWidth)
        {
            italicProbabillity_=.1;
            boldProbabillity_=.1;

            curvingProbabillity_=.1;
            maxHeightDistortionPercentage_=5;
            maxCurveArch_=.1;

            finalBlendAlpha_=.3;
            finalBlendProb_=.1;
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

                void generateFont(char *ret){
                    CV_Assert(this->availableFonts_.size());
                    const char *fnt = this->availableFonts_[rng_.next() % this->availableFonts_.size()].c_str();
                    strcpy(ret,fnt);
                    if(this->rndProbUnder(this->boldProbabillity_)){
                        strcat(ret," Bold");
                    }
                    if(this->rndProbUnder(this->italicProbabillity_)){
                        strcat(ret," Italic");
                    }
                    int fontsize = (this->resHeight_-2*this->txtPad_)/2;
                    strcat(ret," ");
                    std::ostringstream stm;
                    stm << fontsize;
                    strcat(ret,stm.str().c_str());
                }

                void generateTxtPatch(Mat& output,Mat& outputMask,String caption){
                    size_t len = caption.length();
                    int fontsize = (this->resHeight_-2*this->txtPad_)/2;
                    int txtWidth = len*fontsize + 2*this->txtPad_;
                    const int maxTxtWidth=
                        txtWidth>this->maxResWidth_?this->maxResWidth_:txtWidth;
                    Mat textImg, maskImg;
                    textImg =cv::Mat(this->resHeight_,maxTxtWidth,CV_8UC3,Scalar_<uchar>(0,0,0));
                    maskImg =cv::Mat(this->resHeight_,maxTxtWidth,CV_8UC1,Scalar_<uchar>(0,0,0));

                    //Start to draw a pango img
                    cairo_surface_t *surface;
                    cairo_t *cr;

                    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                            textImg.cols, textImg.rows);
                    cr = cairo_create (surface);

                    char font[50];
                    this->generateFont(font);
                    cout << font << endl;
                    cout << caption << endl;
                    draw_text(cr,font,caption.c_str(),this->txtPad_,this->txtPad_);

                    //cairo_surface_write_to_png (surface, "/home/chenziwe/aaaaa.png");

                    CairoToMat(surface,textImg,maskImg);

                    textImg.copyTo(output);
                    maskImg.copyTo(outputMask);
                    cairo_destroy (cr);
                    cairo_surface_destroy (surface);
                }


                void addCurveDeformation(const Mat& inputImg,Mat& outputImg){
                    if (this->rndProbUnder(this->curvingProbabillity_)){
                        Mat X=Mat(inputImg.rows,inputImg.cols,CV_32FC1);
                        Mat Y=Mat(inputImg.rows,inputImg.cols,CV_32FC1);
                        int xAdd=-int(this->rng_.next()%inputImg.cols);
                        float xMult=(this->rng_.next()%10000)*float(maxCurveArch_)/10000;
                        int sign=(this->rng_.next()%2)?-1:1;
                        for(int y=0;y<inputImg.rows;y++){
                            float* xRow=X.ptr<float>(y);
                            float* yRow=Y.ptr<float>(y);
                            for(int x=0;x<inputImg.cols;x++){
                                xRow[x]=float(x);
                                yRow[x] = float(y + sign * cos((x + xAdd) * xMult)
                                        * maxHeightDistortionPercentage_ - sign
                                        * maxHeightDistortionPercentage_);
                            }
                        }
                        remap(inputImg,outputImg,X,Y,INTER_LINEAR);
                    }else{
                        outputImg=inputImg;
                    }
                }


                RNG rng_;//Randon number generator used for all distributions
                int txtPad_;
                std::vector<String> availableFonts_;
                std::vector<String> sampleCaptions_;
                std::vector<String> availableBgSampleFiles_;
                std::vector<Mat> availableBgSampleImages_;

            public:
                TextSynthesizerImpl(
                        int maxSampleWidth = 400,
                        int sampleHeight = 50,
                        uint64 rndState = 0)
                    : TextSynthesizer(maxSampleWidth, sampleHeight)
                      , rng_(rndState != 0 ? rndState:std::time(NULL))
                          , txtPad_(10) {
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

                void generateBgSample(CV_OUT Mat& sample){
                    if(this->availableBgSampleImages_.size()!=0){
                        Mat& img=availableBgSampleImages_[this->rng_.next()%availableBgSampleImages_.size()];
                        int left=this->rng_.next()%(img.cols-maxResWidth_);
                        int top=this->rng_.next()%(img.rows-resHeight_);
                        img.colRange(Range(left,left+maxResWidth_)).rowRange(Range(top,top+resHeight_)).copyTo(sample);
                    }else{
                        if(this->availableBgSampleFiles_.size()==0){
                            Mat res(this->resHeight_,this->maxResWidth_,CV_8UC3);
                            this->rng_.fill(res,RNG::UNIFORM,0,256);
                            res.copyTo(sample);
                        }else{
                            Mat img;
                            img=imread(this->availableBgSampleFiles_[this->rng_.next()%availableBgSampleFiles_.size()].c_str(),IMREAD_COLOR);
                            CV_Assert(img.data != NULL);
                            CV_Assert(img.cols>maxResWidth_ && img.rows> resHeight_);
                            int left=this->rng_.next()%(img.cols-maxResWidth_);
                            int top=this->rng_.next()%(img.rows-resHeight_);
                            img.colRange(Range(left,left+maxResWidth_)).rowRange(Range(top,top+resHeight_)).copyTo(sample);
                        }
                    }
                    if(sample.channels()==4){
                        Mat rgb;
                        cvtColor(sample,rgb,COLOR_RGBA2RGB);
                        sample=rgb;
                    }
                    if(sample.channels()==1){
                        Mat rgb;
                        cvtColor(sample,rgb,COLOR_GRAY2RGB);
                        sample=rgb;
                    }
                }

                void generateTxtSample(CV_OUT Mat& sample,CV_OUT Mat& sampleMask){
                    if(sampleCaptions_.size()!=0){
                        String caption = sampleCaptions_[this->rng_.next()%sampleCaptions_.size()];
                        cout << "caption " << caption << endl;
                        generateTxtPatch(sample,sampleMask,caption);
                    } else {
                        generateTxtPatch(sample,sampleMask,"Map");
                    }

                }

                void generateSample(CV_OUT Mat & sample){
                    Mat txtSample;
                    Mat txtCurved;
                    Mat bgSample;
                    Mat bgResized;
                    Mat txtMask;
                    Mat txtMerged;
                    Mat floatBg;
                    Mat floatTxt;
                    Mat floatMask;

                    std::vector<Mat> txtChannels;
                    generateTxtSample(txtSample,txtMask);

                    split(txtSample,txtChannels);
                    txtChannels.push_back(txtMask);
                    merge(txtChannels,txtMerged);
                    cout << "adding curve" << endl;
                    addCurveDeformation(txtMerged,txtCurved);
                    cout << "split curve" << endl;
                    split(txtCurved,txtChannels);
                    cout << "get mask" << endl;
                    txtMask=txtChannels[3];
                    cout << "pop back" << endl;
                    txtChannels.pop_back();
                    cout << "merge back" << endl;
                    merge(txtChannels,txtSample);

                    cout << "generating bg sample" << endl;
                    generateBgSample(bgSample);
                    bgSample.convertTo(floatBg, CV_32FC3, 1.0/255.0);
                    txtSample.convertTo(floatTxt, CV_32FC3, 1.0/255.0);
                    txtMask.convertTo(floatMask, CV_32FC1, 1.0/255.0);
                    bgResized=floatBg.colRange(0,txtSample.cols);

                    sample=Mat(txtCurved.rows,txtCurved.cols,CV_32FC3);

                    cout << "blend overlay" << endl;
                    blendOverlay(sample,floatTxt,bgResized,floatMask);
                    cout << "width " << sample.cols << endl;
                    cout << "height " << sample.rows << endl;

                    float blendAlpha=float(this->finalBlendAlpha_*(this->rng_.next()%1000)/1000.0);
                    if(this->rndProbUnder(this->finalBlendProb_)){
                    cout << "blend weighted" << endl;
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

                virtual void addBgSampleImage(const Mat& inImg){
                    CV_Assert(inImg.cols>=maxResWidth_ && inImg.rows>= resHeight_);
                    Mat img;
                    switch(inImg.type()){
                        case CV_8UC1: {
                                          cvtColor(inImg, img, COLOR_GRAY2RGBA);
                                          break;
                                      }
                        case CV_8UC3: {
                                          cvtColor(inImg, img, COLOR_RGB2RGBA);
                                          break;
                                      }
                        case CV_8UC4: {
                                          inImg.copyTo(img);
                                          break;
                                      }
                        default:{
                                    CV_Error(Error::StsError,
                                            "Only uchar images of 1, 3, or 4 channels are accepted");
                                }
                    }
                    this->availableBgSampleImages_.push_back(img);
                }

                //TOOODOOOO
                void addFontFiles(const std::vector<cv::String>& fntList){
                    this->updateFontNameList(this->availableFonts_);
                }

                std::vector<String> listBgSampleFiles(){
                    std::vector<String> res(this->availableBgSampleFiles_.size());
                    std::copy(this->availableBgSampleFiles_.begin(),this->availableBgSampleFiles_.end(),res.begin());
                    return res;
                }
        };

        Ptr<TextSynthesizer> TextSynthesizer::create(int maxWidth,int sampleHeight){
            Ptr<TextSynthesizer> res(new TextSynthesizerImpl(maxWidth,sampleHeight));
            return res;
        }

    }  //namespace text
}  //namespace cv
