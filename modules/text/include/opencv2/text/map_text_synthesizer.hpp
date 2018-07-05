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

#ifndef MAP_TEXT_SYNTHESIZER_HPP
#define MAP_TEXT_SYNTHESIZER_HPP

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


        class CV_EXPORTS_W MapTextSynthesizer{

            protected:
                int height_;

                MapTextSynthesizer(int sampleHeight);

            public:
                CV_WRAP virtual void setBlockyFonts (std::vector<String>& fntList) = 0;
                CV_WRAP virtual void setRegularFonts (std::vector<String>& fntList) = 0;
                CV_WRAP virtual void setCursiveFonts (std::vector<String>& fntList) = 0;

                /** @brief set the collection of words to be displayed 
                 *
                 * @param words a list of strings to be sampled
                 */
                CV_WRAP virtual void setSampleCaptions (std::vector<String>& words) = 0;

                /** @brief generates a random text sample given a string
                 *
                 * This is the principal function of the text synthciser
                 *
                 * @param caption the transcription to be written.
                 *
                 * @param sample the resulting text sample.
                 */
                CV_WRAP virtual void generateSample (CV_OUT String &caption, CV_OUT Mat& sample) = 0;

                CV_WRAP static Ptr<MapTextSynthesizer> create (int sampleHeight = 50);

                virtual ~MapTextSynthesizer () {}
        };


    }//text
}//cv

#endif // MAP_TEXT_SYNTHESIZER_HPP
