#ifndef MAP_TEXT_SYNTHESIZER_HPP
#define MAP_TEXT_SYNTHESIZER_HPP

#include <string>
#include <pango/pangocairo.h>

using namespace std;

namespace cv
{
    namespace text
    {

        /*
         * Class that renders synthetic text images for training a CNN 
         * on word recognition in historical maps
         *
         * This functionallity is based on "Synthetic Data and Artificial Neural
         * Networks for Natural Scene Text Recognition" by Max Jaderberg.
         * available at <http://arxiv.org/pdf/1406.2227.pdf>
         */
        class CV_EXPORTS_W MapTextSynthesizer{

            protected:
                int height_; //the height in pixels of the image

                /*
                 * Constructor. Kept protected so that a CV_WRAPed public
                 * wrapper for the constructor can be called in a python script.
                 *
                 * sampleHeight - the height in pixels of the image
                 */
                MapTextSynthesizer(int sampleHeight);

            public:
                /*
                 * Setter method to initialize the blockyFonts_ field
                 *
                 * fntList - a list of fonts contained in a vector
                 */
                CV_WRAP virtual void 
                    setBlockyFonts (std::vector<String>& fntList) = 0;


                /*
                 * Setter method to initialize the regularFonts_ field
                 *
                 * fntList - a list of fonts contained in a vector
                 */
                CV_WRAP virtual void 
                    setRegularFonts (std::vector<String>& fntList) = 0;


                /*
                 * Setter method to initialize the cursiveFonts_ field
                 *
                 * fntList - a list of fonts contained in a vector
                 */
                CV_WRAP virtual void 
                    setCursiveFonts (std::vector<String>& fntList) = 0;


                /*
                 * Set the collection of words to be displayed 
                 *
                 * words - a list of strings to be sampled
                 */
                CV_WRAP virtual void 
                    setSampleCaptions (std::vector<String>& words) = 0;


                /*
                 * Generates a random bounded map-like text sample given a string
                 * This is the principal function of the text synthciser
                 *
                 * caption - the transcription to be written.
                 * sample - the resulting text sample.
                 */
                CV_WRAP virtual void 
                    generateSample (CV_OUT String &caption, CV_OUT Mat& sample) = 0;

                /*
                 * A wrapper for the protected MapTextSynthesizer constructor.
                 * 
                 * sampleHeight - the height in pixels of the image to be 
                 * created. (optional parameter. Defaults to 50)
                 */
                CV_WRAP static Ptr<MapTextSynthesizer> 
                    create (int sampleHeight = 50);


                /*
                 * The destructor for the MapTextSynthesizer class 
                 * (does nothing)
                 */ 
                virtual ~MapTextSynthesizer () {}
        };


    }//text
}//cv

#endif // MAP_TEXT_SYNTHESIZER_HPP
