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

          /*
           * Converts a cairo_surface_t object to a openCV Mat object
           *
           * surface - the cairo surface target
           * MC3 - the openCV Mat destination (rbg 3 channels)
           */
                static void 
                CairoToMat(cairo_surface_t *surface,Mat &MC3);

          /*
           * CairoToMat overload. 
           *
           * surface - the cairo surface target
           * MC3 - the openCV Mat destination (rbg 3 channels)
           * MC1 - a second openCV Mat destination for an alpha channel
           */
                static void 
                CairoToMat(cairo_surface_t *surface, Mat &MC3, Mat &MC1);

          /*
           * 
           *
           * out -
           * top -
           * bottom - 
           * topMask - 
           * bottomMask - 
           */
                static void 
                blendWeighted(Mat& out, Mat& top, Mat& bottom, float topMask, float bottomMask);

          /*
           * blendWeighted overload.
           *
           * out -
           * top -
           * bottom - 
           * topMask_ - 
           * bottomMask_ - 
           */
                static void 
                blendWeighted(Mat& out, Mat& top, Mat& bottom, Mat& topMask_, Mat& bottomMask_);

          /*
           * 
           *
           * out -
           * top - 
           * bottom - 
           * topMask - 
           */
                static void 
                blendOverlay(Mat& out, Mat& top, Mat& bottom, Mat& topMask);

          /*
           * blendOverlay overload.
           *
           * out -
           * topCol - 
           * bottomCol -
           * topMask -
           */
                static void 
                blendOverlay(Mat& out, Scalar topCol, Scalar bottomCol, Mat& topMask);

          /*
           * Adds gaussian noise to out to make the resulting image 
           * look more realistic. (preparation for gaussian blur)
           *
           * out - the image the noise is applied to
           */
                static void 
                addGaussianNoise(Mat& out);

          /*
           * Adds gaussian blur to out to make the resulting image less sharp 
           * and look more realistic
           *
           * out - the image the blur is applied to
           */
                static void 
                addGaussianBlur(Mat& out);

          /*
           * Clears the existing list of font names and refetches all font names
           * from pango, setting them back into fntList
           *
           * fntList - the existing font name list to be cleared and reset
           */
                void 
                updateFontNameList(std::vector<String>& fntList);


          // Fields for holding different types of fonts
                std::vector<String> blockyFonts_;
                std::vector<String> regularFonts_;
                std::vector<String> cursiveFonts_;
                std::vector<String> availableFonts_;
                std::shared_ptr<std::vector<String> > fonts_[3];

          // The available words for text synthesization to sample from
                std::vector<String> sampleCaptions_;

          // helper classes
                MTS_BaseHelper helper;
                MTS_TextHelper th;
                MTS_BackgroundHelper bh;

            public:

          /*
           * Constructor
           *
           * sampleHeight - height of the image in pixels
           * rndState - seed for the rng. (optional parameter, default 0)
           */
                MTSImplementation(int sampleHeight = 50, uint64 rndState = 0);

          /*
           * A setter function for setting the blockyFonts_ field
           *
           * fntList - the list of fonts to put into the blockyFonts_ field
           */
                void 
                setBlockyFonts(std::vector<String>& fntList);

          /*
           * A setter function for setting the regualarFonts_ field
           *
           * fntList - the list of fonts to put into the regularFonts_ field
           */
                void 
                setRegularFonts(std::vector<String>& fntList);

          /*
           * A setter function for setting the cursiveFonts_ field
           *
           * fntList - the list of fonts to put into the cursiveFonts_ field
           */
                void 
                setCursiveFonts(std::vector<String>& fntList);

          /*
           * A setter function for setting the sampleCaptions_ field
           *
           * words - the list of words to put into the sampleCaptions_ field
           */
                void 
                setSampleCaptions (std::vector<String>& words);

          /*
           * Generates a map-like image containing text that is closely
           * bounded
           *
           * caption - the text that appears in the image
           * sample - the Mat that will hold the synthesized image
           */
                void 
                generateSample(CV_OUT String &caption, CV_OUT Mat & sample);
        };

    }  //namespace text
}  //namespace cv

#endif
