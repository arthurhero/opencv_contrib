#ifndef MTS_IMPLEMENTATION_HPP
#define MTS_IMPLEMENTATION_HPP

#include "opencv2/text/map_text_synthesizer.hpp"
#include "opencv2/text/mts_basehelper.hpp"
#include "opencv2/text/mts_texthelper.hpp"
#include "opencv2/text/mts_bghelper.hpp"

#include <glib.h>
#include <pango/pangocairo.h>


using namespace std;

namespace cv{
    namespace text{

        class MTSImplementation: public MapTextSynthesizer{
            protected:
                static void CairoToMat(cairo_surface_t *surface,Mat &MC3);

                static void CairoToMat(cairo_surface_t *surface,Mat &MC3, Mat &MC1);

                static void blendWeighted(Mat& out,Mat& top,Mat& bottom,float topMask,float bottomMask);

                static void blendWeighted(Mat& out,Mat& top,Mat& bottom,Mat& topMask_,Mat& bottomMask_);

                static void blendOverlay(Mat& out,Mat& top,Mat& bottom,Mat& topMask);

                static void blendOverlay(Mat& out,Scalar topCol,Scalar bottomCol,Mat& topMask);

                static void addGaussianNoise(Mat& out);

                static void addGaussianBlur(Mat& out);

                void updateFontNameList(std::vector<String>& fntList);

                std::vector<String> blockyFonts_;
                std::vector<String> regularFonts_;
                std::vector<String> cursiveFonts_;
                std::vector<String> availableFonts_;

                std::shared_ptr<std::vector<String> > fonts_[3];
                std::vector<String> sampleCaptions_;

                MTSBaseHelper helper;
                MTSTextHelper th;
                MTSBgHelper bh;

            public:
                MTSImplementation(int sampleHeight = 50, uint64 rndState = 0);

                void setBlockyFonts(std::vector<String>& fntList);

                void setRegularFonts(std::vector<String>& fntList);

                void setCursiveFonts(std::vector<String>& fntList);

                void setSampleCaptions (std::vector<String>& words);

                void generateSample(CV_OUT String &caption, CV_OUT Mat & sample);
        };

    }  //namespace text
}  //namespace cv

#endif
