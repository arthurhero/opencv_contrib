#include "precomp.hpp"

#include "opencv2/text/mts_implementation.hpp"

using namespace std;

namespace cv{
    namespace text{

        MapTextSynthesizer::MapTextSynthesizer(int sampleHeight):
            height_(sampleHeight){}

        Ptr<MapTextSynthesizer> MapTextSynthesizer::create(int sampleHeight){
            Ptr<MapTextSynthesizer> res(new MTSImplementation(sampleHeight));
            return res;
        }

    }  //namespace text
}  //namespace cv