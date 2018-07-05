#ifndef MTS_TEXTHELPER_HPP
#define MTS_TEXTHELPER_HPP

#include <pango/pangocairo.h>
#include "opencv2/text/mts_basehelper.hpp"
#include <vector>
#include <string>
#include <memory>


// The different categories for text features
enum TextType {Water, BigLand, SmallLand};


/*
 * A class to handle text transformation in vector space, and pango
 * text rendering 
 */ 
class MTS_TextHelper : MTS_BaseHelper {
private:// --------------- PRIVATE METHODS AND FIELDS ------------------------

  std::shared_ptr<std::vector<String> > fonts_[3];
  std::shared_ptr<std::vector<String> > sampleCaptions_;

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


public:// --------------------- PUBLIC METHODS -------------------------------
  
 
  /*
   * A setter method for the private fonts_ field
   *
   * data - an array of vectors of strings
   */
  static void
  set_fonts(std::shared_ptr<std::vector<String> > data[]);


  /*
   * A setter method for the private sampleCaptions_ field
   *
   * data - a vector of strings
   */
  static void
  set_fonts(std::shared_ptr<std::vector<String> > data);


  /*
   * Provides the randomly rendered text 
   *
   * caption - the string which will be rendered. 
   * sample - an out variable containing a 32FC3 matrix with the rendered 
   *          text including border and shadow.
   * sampleMask - a result parameter which contains the alpha value which 
   *              is usefull for overlaying the text sample on other images.
   * text_color - the grayscale color value for the text
   * distract - flag that dictates whether distractor text will be present
   */
  void 
  generateTxtSample (CV_OUT String &caption, CV_OUT Mat& sample, 
		     CV_OUT Mat& sampleMask, int text_color, bool distract);

};

#endif
