Scene Text Detection and Recognition in Natural Scene Images
============================================================

The module contains algorithms to detect text, segment words and recognise the text.
It's mainly intended for the "text in the wild", i.e. short phrases and separate words that occur on navigation signs and such. It's not an OCR tool for scanned documents, do not treat it as such.
The detection part can in theory handle different languages, but will likely fail on hieroglyphic texts.

The recognition part currently uses open-source Tesseract OCR (https://code.google.com/p/tesseract-ocr/). If Tesseract OCR is not installed on your system, the corresponding part of the functionality will be unavailable.

Here are instructions on how to install Tesseract on your machine (Linux or Mac; Windows users should look for precompiled binaries or try to adopt the instructions below):

Tesseract installation instruction (Linux, Mac)
-----------------------------------------------

0. Linux users may try to install tesseract-3.03-rc1 (or later) and leptonica-1.70 (or later) with the corresponding developement packages using their package manager. Mac users may try brew. The instructions below are for those who wants to build tesseract from source.

1. download leptonica 1.70 tarball (helper image processing library, used by tesseract. Later versions might work too):
http://www.leptonica.com/download.html
unpack and build it:

cd leptonica-1.70
mkdir build && cd build && ../configure && make && sudo make install

leptonica will be installed to /usr/local.

2. download tesseract-3.03-rc1 tarball from https://drive.google.com/folderview?id=0B7l10Bj_LprhQnpSRkpGMGV2eE0&usp=sharing
unpack and build it:

# needed only to build tesseract
export LIBLEPT_HEADERSDIR=/usr/local/include/
cd tesseract-3.03
mkdir build && cd build
../configure --with-extra-includes=/usr/local --with-extra-libraries=/usr/local
make && sudo make install

tessract will be installed to /usr/local.

3. download the pre-trained classifier data for english language:
https://code.google.com/p/tesseract-ocr/downloads/detail?name=eng.traineddata.gz

unzip it (gzip -d eng.traineddata.gz) and copy to /usr/local/share/tessdata.

Notes
-----
1. Google announced that they close code.google.com, so at some moment in the future you may have to find Tesseract 3.03rc1 or later.

2. Tesseract configure script may fail to detect leptonica, so you may have to edit the configure script - comment off some if's around this message and retain only "then" branch.

3. You are encouraged to search the Net for some better pre-trained classifiers, as well as classifiers for other languages.


Word spotting CNN
=================

Intro
-----

A word spotting CNN is a CNN that takes an image assumed to contain a single word and provides a probabillity over a given vocabulary.
Although other backends will be supported, for the moment only the Caffe backend is supported.




Instalation of Caffe backend
----------------------------
The caffe wrapping backend has the requirements caffe does.
* Caffe can be built against OpenCV, if the caffe backend is enabled, a circular bependency arises.
The simplest solution is to build caffe without support for OpenCV.
* Only the OS supported by Caffe are supported by the backend. 
The module has been developed in ubuntu 16.04 and OSX should also be supported.



```bash
#!/bin/bash
SRCROOT="${HOME}/caffe_inst/"
echo $SRCROOT
mkdir -p "$SRCROOT"
cd "$SRCROOT"
git clone https://github.com/BVLC/caffe.git
cd caffe
cat Makefile.config.example  > Makefile.config
echo 'USE_OPENCV := 0' >> Makefile.config
echo 'INCLUDE_DIRS += /usr/include/hdf5/serial/' >> Makefile.config
echo 'LIBRARY_DIRS += /usr/lib/x86_64-linux-gnu/hdf5/serial/' >> Makefile.config


echo "--- /tmp/caffe/include/caffe/net.hpp	2017-05-28 04:55:47.929623902 +0200
+++ caffe/distribute/include/caffe/net.hpp	2017-05-28 04:51:33.437090768 +0200
@@ -234,6 +234,7 @@
 
     template <typename T>
     friend class Net;
+    virtual ~Callback(){}
   };
   const vector<Callback*>& before_forward() const { return before_forward_; }
   void add_before_forward(Callback* value) {
">/tmp/patch_caffe


make -j 6

make pycaffe

make distribute
```
