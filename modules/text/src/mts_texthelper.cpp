#include <pango/pangocairo.h>
#include "opencv2/text/mts_basehelper.hpp"
#include "opencv2/text/mts_texthelper.hpp"
#include <math.h>
#include <vector>
#include <memory>
#include <assert.h>
#include <string>
#include <sstream>

#include <iostream>
using namespace std;

// SEE mts_texthelper.hpp FOR ALL DOCUMENTATION

void 
MTS_TextHelper::generateFont(char *ret, int fontsize){

    cout << "in generate font" << endl;
    // assert that font vectors are not empty
    assert(availableFonts_.size());
    assert(blockyFonts_.size());
    assert(regularFonts_.size());
    assert(cursiveFonts_.size());

    double *fontProb;
    int font_prob = rand() % 10000;

    int bgIndex=static_cast<int>(this->bgType_); // DONT DO THIS

    fontProb=this->fontProbability_[bgIndex];


    for (int i = 0; i < 3; i++) {

        if(font_prob < 10000*fontProb[i]){
            cout << "font index " << i << endl;

            int listsize = this->fonts_[i]->size();
            cout << "list size " << listsize << endl;
            // assert list isn't empty
            assert(listsize);
            const char *fnt = this->fonts_[i]->at(rand() % listsize).c_str();
            strcpy(ret,fnt);
            break;
        }
    }
    cout << "after for" << endl;

    //set probability of being Italic
    int italic_prob = rand() % 10000;

    if(italic_prob < 10000 * italicProbability_[bgIndex]){
        strcat(ret," Italic");
    }

    strcat(ret," ");
    std::ostringstream stm;
    stm << fontsize;
    strcat(ret,stm.str().c_str());
}


void 
MTS_TextHelper::generateTxtPatch(cairo_surface_t *textSurface, 
        string caption,int height,int &width, int text_color, bool distract){

    size_t len = caption.length();

    // if determined by probability of rotation, set rotated angle
    if (MTS_BaseHelper::rndProbUnder(rotatedProbability_)){
        int degree = rand() % 21 - 10;

        cout << "degree " << degree << endl;
        rotatedAngle_=((double)degree / 180) * M_PI;

    } else {
        rotatedAngle_ = 0;
    }

    int bgIndex=static_cast<int>(bgType_);

    double curvingProb=curvingProbability_[bgIndex];
    double *spacingProb=spacingProbability_[bgIndex];
    double *stretchProb=stretchProbability_[bgIndex];

    // set probability of being curved
    bool curved = false;
    if(MTS_BaseHelper::rndProbUnder(curvingProb)){
        curved = true;
    }

    //get spacing degree
    int spacing_prob = rand()%10000;
    double spacing_deg=-0.6+exp(4)/5;
    for (int i=0;i<4;i++) {
        if (spacing_prob<spacingProb[i]*10000) {
            spacing_deg=-0.6+exp((double)i)/5;
            break;
        }
    }

    cout << "spacing deg " << spacing_deg << endl;

    //get stretch degree
    int stretch_prob = rand()%10000;
    double stretch_deg=-0.5+exp(4/4.0);

    for (int i = 0; i < 4; i++) {

        if (stretch_prob < stretchProb[i]*10000) {
            stretch_deg=-0.5+exp(i/4.0);
            break;
        }
    }

    cout << "stretch deg " << stretch_deg << endl;

    double fontsize = (double)height / 4 * 3;
    double spacing = fontsize / 20 * spacing_deg;

    int maxpad=height / 10;
    int x_pad = rand() % maxpad - maxpad/2;
    int y_pad = rand() % maxpad - maxpad/2;
    cout << "pad " << x_pad << " " << y_pad << endl;

    double scale = (rand()%5)/20.0+0.9;
    cout << "scale " << scale << endl;

    //Start to draw a pango img
    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 40*height, height);
    cr = cairo_create (surface);

    char font[50];
    cout << "generate font" << endl;
    generateFont(font,(int)fontsize);
    cout << font << endl;
    //cout << caption << endl;
    cairo_set_source_rgb(cr, text_color/255.0,text_color/255.0,text_color/255.0);

    PangoLayout *layout;
    PangoFontDescription *desc;

    layout = pango_cairo_create_layout (cr);

    //set font destcription
    desc = pango_font_description_from_string(font);

    //Weight
    double *weightProb=weightProbability_[bgIndex];
    int weight_prob = rand()%10000;

    if(weight_prob < 10000*weightProb[0]){
        pango_font_description_set_weight(desc, PANGO_WEIGHT_LIGHT);
    } else if(weight_prob < 10000*weightProb[1]){
        pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
    } else {
        pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
    }

    int spacing_ = (int)(1024*spacing);

    std::ostringstream stm;
    stm << spacing_;
    // set the markup string and put into pango layout
    string mark = "<span letter_spacing='"+stm.str()+"'>"+caption+"</span>";
    cout << "mark " << mark << endl;

    pango_layout_set_markup(layout, mark.c_str(), -1);

    PangoRectangle *ink_rect = new PangoRectangle;
    PangoRectangle *logical_rect = new PangoRectangle;
    pango_layout_get_extents(layout, ink_rect, logical_rect);

    int ink_x=ink_rect->x/1024;
    int ink_y=ink_rect->y/1024;
    int ink_w=ink_rect->width/1024;
    int ink_h=ink_rect->height/1024;

    cout << "x " << ink_x << endl;
    cout << "y " << ink_y << endl;
    cout << "width " << ink_w << endl;
    cout << "height " << ink_h << endl;

    int cur_size = pango_font_description_get_size(desc);
    cout << "cur size " << cur_size << endl;
    int size = (int)((double)cur_size/ink_h*height);
    cout << "size " << size << endl;
    pango_font_description_set_size(desc, size);
    pango_layout_set_font_description (layout, desc);

    pango_layout_get_extents(layout, ink_rect, logical_rect);
    ink_x=ink_rect->x/1024;
    ink_y=ink_rect->y/1024;
    ink_w=ink_rect->width/1024;
    ink_h=ink_rect->height/1024;

    cout << "new x " << ink_x << endl;
    cout << "new y " << ink_y << endl;
    cout << "new width " << ink_w << endl;
    cout << "new height " << ink_h << endl;
    cur_size = pango_font_description_get_size(desc);
    cout << "new cur size " << cur_size << endl;

    ink_w=stretch_deg*(ink_w);
    int patchWidth = (int)ink_w;
    cout << "patch width " << patchWidth << endl;

    cout << "after stretch" << endl;
    if (this->rotatedAngle_!=0) {
        cout << "rotated angle" << this->rotatedAngle_ << endl;
        cairo_rotate(cr, this->rotatedAngle_);

        double sine = abs(sin(this->rotatedAngle_));
        double cosine = abs(cos(this->rotatedAngle_));

        double ratio = ink_h/(double)ink_w;
        double textWidth, textHeight;

        textWidth=(height/(cosine*ratio+sine));
        cout << "new width " << textWidth << endl;
        textHeight = (ratio*textWidth);
        cout << "new height " << textHeight << endl;
        patchWidth = (int)ceil(cosine*textWidth+sine*textHeight)+1;
        cout << "real h " << sine*textWidth+cosine*textHeight << endl;

        cur_size = pango_font_description_get_size(desc);
        size = (int)((double)cur_size/ink_h*textHeight);
        cout << "rotate size " << size << endl;
        pango_font_description_set_size(desc, size);
        pango_layout_set_font_description (layout, desc);

        spacing_ = (int)floor((double)spacing_/ink_h*textHeight);

        std::ostringstream stm;
        stm << spacing_;
        string mark = "<span letter_spacing='"+stm.str()+"'>"+caption+"</span>";
        cout << "mark " << mark << endl;

        pango_layout_set_markup(layout, mark.c_str(), -1);

        double x_off=0, y_off=0;
        if (this->rotatedAngle_<0) {
            x_off=-sine*sine*textWidth;
            y_off=cosine*sine*textWidth;
        } else {
            x_off=cosine*sine*textHeight;
            y_off=-sine*sine*textHeight;
        }   
        cairo_translate (cr, x_off, y_off);

        cairo_scale(cr, stretch_deg, 1);

        double height_ratio=textHeight/ink_h;
        y_off=(ink_y*height_ratio);
        x_off=(ink_x*height_ratio);
        cairo_translate (cr, -x_off, -y_off);

        pango_cairo_show_layout (cr, layout);

    } else if (curved && patchWidth > 10*height && spacing_deg>=10) {
        cairo_scale(cr, stretch_deg, 1);

        cairo_path_t *path;
        PangoLayoutLine *line;
        cout << "before tt" << endl;

        create_curved_path(cr,path,line,layout,(double)patchWidth,
                (double) height,0,0,rand()%2+3, rand());

        cout << "after tt" << endl;
        double x1,x2,y1,y2;
        cairo_path_extents(cr,&x1,&y1,&x2,&y2);
        cout << "x1 " << x1 << endl;
        cout << "y1 " << y1 << endl;
        cout << "x2 " << x2 << endl;
        cout << "y2 " << y2 << endl;

        cairo_path_t *path_n=cairo_copy_path(cr);
        cairo_new_path(cr);
        cairo_path_data_t *path_data_n;
        manual_translate(cr, path_n, path_data_n, -x1, -y1);

        cairo_path_extents(cr,&x1,&y1,&x2,&y2);
        cout << "x1 " << x1 << endl;
        cout << "y1 " << y1 << endl;
        cout << "x2 " << x2 << endl;
        cout << "y2 " << y2 << endl;

        path_n=cairo_copy_path(cr);
        cairo_new_path(cr);

        cairo_surface_t *surface_c;
        cairo_t *cr_c;

        surface_c = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, (int)ceil(x2-x1), (int)ceil(y2-y1));
        cr_c = cairo_create (surface_c);
        cairo_new_path(cr_c);
        cairo_append_path(cr_c,path_n);
        cairo_fill(cr_c);

        double ratio = height/(y2-y1);
        cout << "ratio " << ratio << endl;
        cairo_scale(cr,ratio,ratio);
        patchWidth=(int)(ceil((x2-x1)*ratio)*stretch_deg);
        cairo_set_source_surface(cr, surface_c, 0, 0);
        cairo_rectangle(cr, 0, 0, x2-x1, y2-y1);
        cairo_fill(cr);
        cairo_destroy (cr_c);
        cairo_surface_destroy (surface_c);
    } else {
        cairo_scale(cr, stretch_deg, 1);
        cairo_translate (cr, -ink_x, -ink_y);
        pango_cairo_show_layout (cr, layout);
    }

    //free layout
    cout << "freeing" << endl;
    g_object_unref(layout);
    pango_font_description_free (desc);
    free(logical_rect);
    free(ink_rect);

    cairo_identity_matrix(cr);

    cout << "destroy cr" << endl;
    cout << "ref count " << cairo_get_reference_count(cr) << endl;
    cairo_destroy (cr);

    cairo_surface_t *surface_n;
    cairo_t *cr_n;

    surface_n = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, patchWidth, height);
    cr_n = cairo_create (surface_n);

    //apply arbitrary padding and scaling
    cairo_translate (cr_n, x_pad, y_pad);

    cairo_translate (cr_n, patchWidth/2, height/2);
    cairo_scale(cr_n, scale, scale);
    cairo_translate (cr_n, -patchWidth/2, -height/2);

    cairo_set_source_surface(cr_n, surface, 0, 0);
    cairo_rectangle(cr_n, 0, 0, patchWidth, height);
    cairo_fill(cr_n);

    cairo_set_source_rgb(cr_n, text_color/255.0,text_color/255.0,text_color/255.0);
    //draw distracting text
    if (distract) {
        int dis_num = this->rng_.next()%3+1;

        for (int i=0;i<dis_num;i++) {
            char font2[50];
            int shrink = this->rng_.next()%3+2;
            generateFont(font2,size/1024/shrink);
            distractText(cr_n, patchWidth, height, font2, this->rng_.next());
        }
    }

    cout << "destroy cr_n" << endl;
    cout << "ref count " << cairo_get_reference_count(cr_n) << endl;
    cairo_destroy (cr_n);
    cout << "destroy surface" << endl;
    cout << "ref count " << cairo_surface_get_reference_count(surface) << endl;
    cairo_surface_destroy (surface);

    cout << "add spots" << endl;
    if(this->rndProbUnder(this->missingProbability_)){
        addSpots(surface_n,2,true,0);
    }

    //pass back values
    *textSurface=*surface_n;
    width=patchWidth;
}


void 
MTS_TextHelper::generateTxtSample (string &caption, cairo_surface_t *textSurface, int height, int &width, int text_color, bool distract){

    if(sampleCaptions_->size()!=0){
        caption = sampleCaptions_->at(rand()%sampleCaptions_->size());
        cout << "generating text patch" << endl;
        generateTxtPatch(textSurface,caption,height,width,text_color,distract);
    } else {
        int len = rand()%10+1;
        char text[len+1];
        for (int i=0;i<len;i++) {
            text[i]=randomChar();
        }   
        text[len]='\0';
        caption=string(text);
        generateTxtPatch(textSurface,caption,height,width,text_color,distract);
    }
}


char
MTS_TextHelper::randomChar() {

    char ch;
    int number = rand() % 5; // 1/5 chance number==0

    // set ch to be a number (range 0-9)
    if (number == 0){
        ch=(char)(rand()%10+48);

    } else { // set ch to be a latin character (range a-z)
        // 50% chance to be upper case, 50% to be lower case
        ch=(char)((rand() % 26 + 65) + (rand() % 2) * 32);
    }

    return ch;
}


void
MTS_TextHelper::distractText (cairo_t *cr, int width, int height, char *font) {

    //generate text
    int len = rand()%10+1;
    char text[len+1];

    for (int i=0;i<len;i++) {
        text[i]=randomChar();
    }
    text[len]='\0'; //null terminate the cstring

    //use pango to turn cstring into vector text
    PangoLayout *layout;
    PangoFontDescription *desc;
    layout = pango_cairo_create_layout (cr);

    desc = pango_font_description_from_string(font);
    pango_layout_set_font_description (layout, desc);
    pango_layout_set_text(layout, text, -1);

    PangoRectangle *ink_rect = new PangoRectangle;
    PangoRectangle *logical_rect = new PangoRectangle;
    pango_layout_get_extents(layout, ink_rect, logical_rect);

    int ink_w=ink_rect->width/1024;
    int ink_h=ink_rect->height/1024;

    int x = rand()%width;
    int y = rand()%height;

    cairo_translate (cr, (double)x, (double)y);

    //randomly choose rotation degree
    int deg = rand()%360;
    double rad = deg/180.0*3.14;

    cairo_translate (cr, ink_w/2.0, ink_h/2.0);
    cairo_rotate(cr, rad);
    cairo_translate (cr, -ink_w/2.0, -ink_h/2.0);

    //put text on cairo context
    pango_cairo_show_layout (cr, layout);

    //clean up
    cairo_translate (cr, -x, -y);
    cairo_rotate(cr, 0);
    g_object_unref(layout);
    pango_font_description_free (desc);
    free(logical_rect);
    free(ink_rect);
}


void
MTS_TextHelper::set_fonts(std::shared_ptr<std::vector<String> > *data) {
    this->fonts_ = data;
}


void
set_sampleCaptions(std::shared_ptr<std::vector<String> > data) {
    this->sampleCaptions_ = data;
}
