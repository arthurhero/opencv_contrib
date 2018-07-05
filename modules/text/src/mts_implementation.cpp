#include "precomp.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/calib3d.hpp"

#include "opencv2/text/mts_implementation.hpp"

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

        void MTSImplementation::CairoToMat(cairo_surface_t *surface,Mat &MC3)
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

        void MTSImplementation::CairoToMat(cairo_surface_t *surface,Mat &MC3, Mat &MC1)
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

        void MTSImplementation::blendWeighted(Mat& out,Mat& top,Mat& bottom,float topMask,float bottomMask){
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

        void MTSImplementation::blendWeighted(Mat& out,Mat& top,Mat& bottom,Mat& topMask_,Mat& bottomMask_){
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

        void MTSImplementation::blendOverlay(Mat& out,Mat& top,Mat& bottom,Mat& topMask){
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

        void MTSImplementation::blendOverlay(Mat& out,Scalar topCol,Scalar bottomCol,Mat& topMask){
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

        void MTSImplementation::addGaussianNoise(Mat& out){
            double sigma = (helper.rand()%7+2)/100.0;

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

        void MTSImplementation::addGaussianBlur(Mat& out){
            int ker_size = helper.rand()%2*2+3;
            GaussianBlur(out,out,Size(ker_size,ker_size),0,0,BORDER_REFLECT_101);
        }

        void MTSImplementation::updateFontNameList(std::vector<String>& fntList){
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

        MTSImplementation::MTSImplementation(
                int sampleHeight = 50,
                uint64 rndState = 0)
            : MapTextSynthesizer(sampleHeight),
            fonts_{std::shared_ptr<std::vector<String> >(&(this->blockyFonts_)),
                std::shared_ptr<std::vector<String> >(&(this->regularFonts_)),
                std::shared_ptr<std::vector<String> >(&(this->cursiveFonts_))}
        {
            namedWindow("__w");
            waitKey(1);
            destroyWindow("__w");
            this->updateFontNameList(this->availableFonts_);

            //initialize rng in base helper
            helper.setSeed(rndState != 0 ? rndState:time(NULL));

            //pass required fields for other helpers
            th.setFontLists(fonts_);
            th.setSampleCaptions(std::shared_ptr<std::vector<String> >(&(sampleCaptions_)));
        }

        void MTSImplementation::setBlockyFonts(std::vector<String>& fntList){
            std::vector<String> dbList=this->availableFonts_;
            for(size_t k=0;k<fntList.size();k++){
                if(std::find(dbList.begin(), dbList.end(), fntList[k]) == dbList.end()){
                    CV_Error(Error::StsError,"The font name list must only contain fonts in your system");
                }
            }
            this->blockyFonts_=fntList;
        }

        void MTSImplementation::setRegularFonts(std::vector<String>& fntList){
            std::vector<String> dbList=this->availableFonts_;
            for(size_t k=0;k<fntList.size();k++){
                if(std::find(dbList.begin(), dbList.end(), fntList[k]) == dbList.end()){
                    CV_Error(Error::StsError,"The font name list must only contain fonts in your system");
                }
            }
            this->regularFonts_=fntList;
        }

        void MTSImplementation::setCursiveFonts(std::vector<String>& fntList){
            std::vector<String> dbList=this->availableFonts_;
            for(size_t k=0;k<fntList.size();k++){
                if(std::find(dbList.begin(), dbList.end(), fntList[k]) == dbList.end()){
                    CV_Error(Error::StsError,"The font name list must only contain fonts in your system");
                }
            }
            this->cursiveFonts_=fntList;
        }

        void MTSImplementation::setSampleCaptions (std::vector<String>& words) {
            this->sampleCaptions_ = words;
        }

        void MTSImplementation::generateSample(CV_OUT String &caption, CV_OUT Mat & sample){
            int bg_prob = helper.rand()%10000;
            if (bg_prob<10000*helper.bgProbability_[0]){
                helper.bgType_=Water;
                helper.bgType4_=Flow;
            } else if (bg_prob<10000*helper.bgProbability_[1]){
                helper.bgType_=Bigland;
                if (helper.rand()%2==0){
                    helper.bgType4_=Waterbody;
                } else {
                    helper.bgType4_=Small;
                }
            } else {
                helper.bgType_=Smallland;
                helper.bgType4_=Small;
            }

            std::vector<BGFeature> bgFeatures;
            bh.generateBgFeatures(bgFeatures);

            int bg_brightness = 255-helper.rand()%100;
            int text_color = helper.rand()%50;
            int contrast = bg_brightness - text_color;

            Mat txtSample;
            Mat txtMask;
            Mat bgSample;
            Mat floatBg;
            Mat floatTxt;
            Mat floatMask;

            std::vector<Mat> txtChannels;
            
            cairo_surface_t *textSurface;
            int width;
            //cout << "generating text sample" << endl;
            if (find(bgFeatures.begin(), bgFeatures.end(), Distracttext)!= bgFeatures.end()) {
                th.generateTxtSample(caption,textSurface,height_,width,text_color,true);
            } else {
                th.generateTxtSample(caption,textSurface,height_,width,text_color,false);
            }
            txtSample=Mat(height_,width,CV_8UC3,Scalar_<uchar>(0,0,0));
            txtMask=Mat(height_,width,CV_8UC1,Scalar(0));

            cout << "converting to mat" << endl;
            CairoToMat(textSurface,txtSample,txtMask);

            cout << "destroy surface" << endl;
            cairo_surface_destroy (textSurface);

            //cout << "finished generating text sample" << endl;

            //cout << "generating bg sample" << endl;
            cout << "bg feature num " << bgFeatures.size() << endl; 

            cairo_surface_t *bgSurface;
            bh.generateBgSample(bgSurface, bgFeatures, height_, width, bg_brightness, contrast);
            bgSample=Mat(height_,width,CV_8UC3,Scalar_<uchar>(0,0,0));
            CairoToMat(bgSurface,bgSample);
            cairo_surface_destroy(bgSurface);
            //cout << "finished generating bg sample" << endl;

            bgSample.convertTo(floatBg, CV_32FC3, 1.0/255.0);
            txtSample.convertTo(floatTxt, CV_32FC3, 1.0/255.0);
            txtMask.convertTo(floatMask, CV_32FC1, 1.0/255.0);

            sample=Mat(txtSample.rows,txtSample.cols,CV_32FC3);

            blendOverlay(sample,floatTxt,floatBg,floatMask);

            float blendAlpha=float(helper.finalBlendAlpha_*(helper.rand()%1000)/1000.0);
            if(helper.rndProbUnder(helper.finalBlendProb_)){
                blendWeighted(sample,sample,floatBg,1-blendAlpha,blendAlpha);
            }

            addGaussianNoise(sample);
            addGaussianBlur(sample);
        }

    }  //namespace text
}  //namespace cv
