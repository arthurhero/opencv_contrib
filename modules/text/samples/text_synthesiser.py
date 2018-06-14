#!/usr/bin/env python
# -*- coding: utf-8 -*-
import cv2
import sys
import numpy as np

# Global Variable definition

helpStr="""Usage: """+sys.argv[0]+""" scenetext_segmented_word01.jpg scenetext_segmented_word02.jpg ...

    This program is a demonstration of the text syntheciser and the effect some of its parameters have.
    The file parameters are optional and they are used to sample backgrounds for the sample synthesis.
    
    In order to quit press (Q) or (q) while the window is in focus.
    """

words=['PM','Charlie','Jerod','Anya','Titus']
#words=[]
        
s=cv2.text.TextSynthesizer_create(1000,400)
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
    filenames=sys.argv[1:]
    for fname in filenames:
        img=cv2.imread(fname,cv2.IMREAD_COLOR)
        s.addBgSampleImage(img)
    s.setCurvingProbabillity(0)
    s.setSampleCaptions(words)

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

# Main Programm

if __name__=='__main__':
    #colorImg=cv2.imread('1000_color_clusters.png',cv2.IMREAD_COLOR)
    #1000_color_clusters.png has the 3 most dominant color clusters 
    #from the first 1000 samples of MSCOCO-text trainset
    #print helpStr
    initialiseSynthesizers()
    initWindows()
    updateTrackbars()
    guiLoop()
