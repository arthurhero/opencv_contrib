#!/usr/bin/env python
# -*- coding: utf-8 -*-
import cv2
import sys
import numpy as np
import time

# Global Variable definition

words=[]
#words=['PM','Charlie','Jerod','Anya','Titus']
#fonts=['Chromaletter','cmmi10','eufm10','MathJax_Fraktur','Sans','Serif','URW Chancery L']
blocky_fonts=['Chromaletter','MathJax_Fraktur','eufm10']
regular_fonts=['cmmi10','Sans','Serif']
cursive_fonts=['URW Chancery L']

        
s=cv2.text.TextSynthesizer_create(32)

# GUI Callsback functions

def initialiseSynthesizers():
    global s
    global words
    global blocky_fonts
    global regular_fonts
    global cursive_fonts
    s.setSampleCaptions(words)

    s.setBlockyFonts(blocky_fonts)
    s.setRegularFonts(regular_fonts)
    s.setCursiveFonts(cursive_fonts)

# Other functions

def generator():
    global s
    k=0
    #while True:
    while k<1000:
        caption,mat=s.generateSample()
        print caption
        k=k+1

# Main Programm
# Main Programm

if __name__=='__main__':
    initialiseSynthesizers()
    start=time.time()
    generator()
    end=time.time()
    print end-start
    '''
    map_generator=generator()
    next(map_generator)
    '''
