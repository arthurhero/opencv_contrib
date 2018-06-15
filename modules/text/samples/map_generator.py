#!/usr/bin/env python
# -*- coding: utf-8 -*-
import cv2
import sys
import numpy as np

# Global Variable definition

words=['PM','Charlie','Jerod','Anya','Titus']
fonts=['Chromaletter','cmmi10','eufm10','MathJax_Fraktur','Sans','Serif','URW Chancery L']
bg_folder_path = '/home/chenziwe/testfiles/bg/complete/'
bg_files=['1.jpg','2.jpg','3.jpg','4.jpg']
        
s=cv2.text.TextSynthesizer_create(500,32)
pause=200

# GUI Callsback functions

def updateCurvProb(x):
    global s
    s.setCurvingProbabillity(x/100.0)

def updateCurvPerc(x):
    global s
    s.setMaxHeightDistortionPercentage(float(x))

def updateCurvArch(x):
    global s
    s.setMaxCurveArch(x/500.0)

def updateTime(x):
    global pause
    pause=x

def initialiseSynthesizers():
    global s
    global words
    global fonts
    global bg_folder_path
    global bg_files
    for fname in bg_files:
        img=cv2.imread(bg_folder_path+fname,cv2.IMREAD_COLOR)
        s.addBgSampleImage(img)
    s.setCurvingProbabillity(0)
    s.setSampleCaptions(words)
    s.setAvailableFonts(fonts)
    s.setBlendAlpha(0.1)
    s.setBlendProb(1)

# Other functions

def initWindows():
    global s
    global pause
    cv2.namedWindow('Text Synthesizer Demo',cv2.WINDOW_NORMAL)
    cv2.resizeWindow('Text Synthesizer Demo',1000,500)
    cv2.moveWindow('Text Synthesizer Demo',100,100)
    cv2.createTrackbar('Curve Prob.','Text Synthesizer Demo',int(s.getCurvingProbabillity()*100),100,updateCurvProb)
    cv2.createTrackbar('Curve %','Text Synthesizer Demo',int(s.getMaxHeightDistortionPercentage()),10,updateCurvPerc)
    cv2.createTrackbar('Curve rad.','Text Synthesizer Demo',int(s.getMaxCurveArch()*500),100,updateCurvArch)
    cv2.createTrackbar('Pause ms','Text Synthesizer Demo',int(pause),500,updateTime)

def updateTrackbars():
    global s
    global pause
    cv2.setTrackbarPos('Curve Prob.','Text Synthesizer Demo',int(s.getCurvingProbabillity()*100))
    cv2.setTrackbarPos('Curve %','Text Synthesizer Demo',int(s.getMaxHeightDistortionPercentage()))
    cv2.setTrackbarPos('Curve rad.','Text Synthesizer Demo',int(s.getMaxCurveArch()*500))
    cv2.setTrackbarPos('Pause ms','Text Synthesizer Demo',int(pause))

def guiLoop():
    global s
    global pause
    k=''
    while ord('q')!=k:
        if pause<500:
            mat=s.generateSample()
            cv2.imshow('Text Synthesizer Demo',mat)
        k=cv2.waitKey(pause+1)

def generator():
    global s
    global sample
    k=0
    #generate 100000 images
    while True:
        yield s.generateSample()

# Main Programm
# Main Programm

if __name__=='__main__':
    #colorImg=cv2.imread('1000_color_clusters.png',cv2.IMREAD_COLOR)
    #1000_color_clusters.png has the 3 most dominant color clusters 
    #from the first 1000 samples of MSCOCO-text trainset
    #print helpStr
    initialiseSynthesizers()
    #initWindows()
    #updateTrackbars()
    #guiLoop()
    map_generator=generator()
    next(map_generator)
    next(map_generator)
    next(map_generator)
    next(map_generator)
    next(map_generator)
    next(map_generator)
    next(map_generator)
