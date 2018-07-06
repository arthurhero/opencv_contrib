#ifndef MTS_TEXTHELPER_HPP
#define MTS_TEXTHELPER_HPP

#include <pango/pangocairo.h>
#include <vector>
#include <string>
#include <memory>

#include "mts_basehelper.hpp"

namespace cv{
    namespace text{

        /*
         * A class to handle text transformation in vector space, and pango
         * text rendering 
         */ 
        class MTS_TextHelper : MTS_BaseHelper {
            private:// --------------- PRIVATE METHODS AND FIELDS ------------------------

                static std::shared_ptr<std::vector<String> > *fonts_;
                static std::shared_ptr<std::vector<String> > sampleCaptions_;

                /*
                 * Returns a random latin character or numeral
                 */
                static char
                    randomChar();


                /*
                 * Generates distractor text with random size and rotation to appear
                 * on the surface along with the main text
                 *
                 * cr - cairo context
                 * width - surface width
                 * height - surface height
                 * font - cstring holding the desired font for the distractor text
                 */
                static void
                    distractText (cairo_t *cr, int width, int height, char *font); 


                /*
                 * 
                 */
                static void
                    generateFont(char *ret, int fontsize);

                static void
                    generateTxtPatch(cairo_surface_t *&textSurface, std::string caption,int height,int &width, int text_color, bool distract);


            public:// --------------------- PUBLIC METHODS -------------------------------


                /*
                 * A setter method for the private fonts_ field
                 *
                 * data - an array of vectors of strings that are font names
                 */
                static void
                    setFonts(std::shared_ptr<std::vector<String> > *data);


                /*
                 * A setter method for the private sampleCaptions_ field
                 *
                 * data - a vector of strings containing words to be displayed
                 */
                static void
                    setSampleCaptions(std::shared_ptr<std::vector<String> > data);


                /*
                 * Provides the randomly rendered text 
                 *
                 * caption - the string which will be rendered. 
                 * textSurface - an out variable containing a 32FC3 matrix with the rendered 
                 *          text including border and shadow.
                 * height - height of the surface
                 * width - width of the surface that will be determined
                 * text_color - the grayscale color value for the text
                 * distract - flag that dictates whether distractor text will be present
                 */
                void 
                    generateTxtSample (std::string &caption, cairo_surface_t *&textSurface, int height, int &width, int text_color, bool distract);

        };

    }
}

#endif
