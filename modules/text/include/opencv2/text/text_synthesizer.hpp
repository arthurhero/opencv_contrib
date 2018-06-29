/*M//////////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef TEXT_SYNTHESIZER_HPP
#define TEXT_SYNTHESIZER_HPP

using namespace std;

namespace cv
{
    namespace text
    {

        /** @brief class that renders synthetic text images for training a CNN on
         * word spotting
         *
         * This functionallity is based on "Synthetic Data and Artificial Neural
         * Networks for Natural Scene Text Recognition" by Max Jaderberg.
         * available at <http://arxiv.org/pdf/1406.2227.pdf>
         *
         * @note
         * - (Python) a demo generating some samples in Greek can be found in:
         * <https://github.com/Itseez/opencv_contrib/blob/master/modules/text/samples/text_synthesiser.py>
         */

        enum BGType {Water=0, Bigland, Smallland};
        enum BGType4 {Flow=0, Waterbody, Big, Small};
        enum BGFeature {Colordiff=0, Distracttext, Boundry, Colorblob, Straight, Grid, Citypoint, Parallel, Vparallel, Mountain, Railroad, Riverline};

        class CV_EXPORTS_W TextSynthesizer{

            protected:
                int resHeight_;

                double bgProbability_[2];
                BGType bgType_;
                BGType4 bgType4_;

                //text features
                double stretchProbability_[3][4];
                double spacingProbability_[3][4];

                double curvingProbability_[3];

                double italicProbability_[3];
                double weightProbability_[3][2];

                double fontProbability_[3][2];

                //independent text properties
                double missingProbability_;
                double noiseProbability_;
                double rotatedProbability_;
                double rotatedAngle_;

                double finalBlendAlpha_;
                double finalBlendProb_;

                //bg features
                double bgProbs_[12][4];

                //max num of bg features each bg4 type can have
                int maxnum_[4];


                TextSynthesizer(int sampleHeight);

            public:

                /** @brief adds ttf fonts to the Font Database system
                 *
                 * Fonts should be added to the system if the are to be used with the syntheciser
                 *
                 * @param fntList a list of TTF files to be incorporated in to the system.
                 */
                CV_WRAP virtual void addFontFiles (const std::vector<String>& fntList) = 0;

                /** @brief retrieves the font family names that are beeing used by the text
                 * synthesizer
                 *
                 * @return a list of strings with the names from which fonts are sampled.
                 */
                CV_WRAP virtual std::vector<String> listAvailableFonts () const = 0;

                /** @brief updates retrieves the font family names that are randomly sampled
                 *
                 * This function indirectly allows you to define arbitrary font occurence
                 * probabilities. Since fonts are uniformly sampled from this list if a font
                 * is repeated, its occurence probabillity doubles.
                 *
                 * @param fntList a list of strings with the family names from which fonts
                 * are sampled. Only font families available in the system can be added.
                 */
                CV_WRAP virtual void setAvailableFonts (std::vector<String>& fntList) = 0;
                CV_WRAP virtual void setBlockyFonts (std::vector<String>& fntList) = 0;
                CV_WRAP virtual void setRegularFonts (std::vector<String>& fntList) = 0;
                CV_WRAP virtual void setCursiveFonts (std::vector<String>& fntList) = 0;

                /** @brief set the collection of words to be displayed 
                 *
                 * @param words a list of strings to be sampled
                 */
                CV_WRAP virtual void setSampleCaptions (std::vector<String>& words) = 0;

                /** @brief provides the randomly rendered text with border and shadow.
                 *
                 * @param caption the string which will be rendered. Multilingual strings in
                 * UTF8 are suported but some fonts might not support it. The syntheciser should
                 * be created with a specific script for fonts guarantiing rendering of the script.
                 *
                 * @param sample an out variable containing a 32FC3 matrix with the rendered text
                 * including border and shadow.
                 *
                 * @param sampleMask a result parameter which contains the alpha value which is usefull
                 * for overlaying the text sample on other images.
                 */
                CV_WRAP virtual void generateTxtSample (CV_OUT String &caption, CV_OUT Mat& sample, CV_OUT Mat& sampleMask, int text_color, bool distract) = 0;


                /** @brief generates a random text sample given a string
                 *
                 * This is the principal function of the text synthciser
                 *
                 * @param caption the transcription to be written.
                 *
                 * @param sample the resulting text sample.
                 */
                CV_WRAP virtual void generateSample (CV_OUT String &caption, CV_OUT Mat& sample) = 0;

                CV_WRAP static Ptr<TextSynthesizer> create (int sampleHeight = 50);

                virtual ~TextSynthesizer () {}
        };


    }//text
}//cv

#endif // TEXT_SYNTHESIZER_HPP
