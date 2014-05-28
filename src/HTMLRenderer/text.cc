/*
 * text.cc
 *
 * Handling text & font, and relative stuffs
 *
 * Copyright (C) 2012 Lu Wang <coolwanglu@gmail.com>
 */


#include <algorithm>

#include "HTMLRenderer.h"

#include "util/namespace.h"
#include "util/unicode.h"

namespace pdf2htmlEX {

using std::all_of;
using std::cerr;
using std::endl;

void HTMLRenderer::drawString(GfxState * state, GooString * s)
{
    if(s->getLength() == 0)
        return;

    auto font = state->getFont();
    // unscaled
    double cur_letter_space = state->getCharSpace();
    double cur_word_space   = state->getWordSpace();
    double cur_horiz_scaling = state->getHorizScaling();

    // Writing mode fonts and Type 3 fonts are rendered as images
    // I don't find a way to display writing mode fonts in HTML except for one div for each character, which is too costly
    // For type 3 fonts, due to the font matrix, still it's hard to show it on HTML
    if( (font == nullptr) 
        || (font->getWMode())
        || ((font->getType() == fontType3) && (!param.process_type3))
      )
    {
        return;
    }

    // see if the line has to be closed due to state change
    check_state_change(state);
    prepare_text_line(state);

    // Now ready to output
    // get the unicodes
    char *p = s->getCString();
    int len = s->getLength();

    double dx = 0;
    double dy = 0;
    double dx1,dy1;
    double ox, oy;

    int nChars = 0;
    int nSpaces = 0;
    int uLen;

    CharCode code;
    Unicode *u = nullptr;

    while (len > 0) 
    {
        auto n = font->getNextChar(p, len, &code, &u, &uLen, &dx1, &dy1, &ox, &oy);

        HTMLTextLine * tom_line = html_text_page.get_cur_line();
        double tom_font_size = cur_text_state.font_size;
        double tom_letter_space = cur_text_state.letter_space ;//>=0 ? cur_text_state.letter_space : 0;
        double tom_word_space = cur_text_state.word_space;
        double tom_horiz_scaling = tom_line->line_state.transform_matrix[0];
        std::cout << tom_letter_space << ' ' << (char) code << ' ' ; 

        //std::cout << HTMLRenderer::TOM_getFontSize(state) << endl; // TOMTRACK

        if(!(equal(ox, 0) && equal(oy, 0)))
        {
            cerr << "TODO: non-zero origins" << endl;
        }

        bool is_space = false;
        if (n == 1 && *p == ' ') 
        {
            /*
             * This is by standard
             * however some PDF will use ' ' as a normal encoding slot
             * such that it will be mapped to other unicodes
             * In that case, when space_as_offset is on, we will simply ignore that character...
             *
             * Checking mapped unicode may or may not work
             * There are always ugly PDF files with no useful info at all.
             */
            is_space = true;
            ++nSpaces;
        }

        if(is_space && (param.space_as_offset))
        {
            // ignore horiz_scaling, as it has been merged into CTM
            ////html_text_page.get_cur_line()->append_offset((dx1 * cur_font_size + cur_letter_space + cur_word_space) * draw_text_scale); 
            tom_line->append_offset((dx1 * tom_font_size + tom_letter_space + tom_word_space) * draw_text_scale);
            //std::cout << (dx1 * tom_font_size + tom_letter_space + tom_word_space) * draw_text_scale << ' ' << 2 << ' ';
        }
        else
        {
            if((param.decompose_ligature) && (uLen > 1) && all_of(u, u+uLen, isLegalUnicode))
            {
                // TODO: why multiply cur_horiz_scaling here?
                ////html_text_page.get_cur_line()->append_unicodes(u, uLen, (dx1 * cur_font_size + cur_letter_space) * cur_horiz_scaling);
                tom_line->append_unicodes(u, uLen, (dx1 * tom_font_size + tom_letter_space) );
                //std::cout << (dx1 * tom_font_size + tom_letter_space) << ' ';
            }
            else
            {
                Unicode uu;
                if(cur_text_state.font_info->use_tounicode)
                {
                    uu = check_unicode(u, uLen, code, font);
                }
                else
                {
                    uu = unicode_from_font(code, font);
                }
                // TODO: why multiply cur_horiz_scaling here?
                ////html_text_page.get_cur_line()->append_unicodes(&uu, 1, (dx1 * cur_font_size + cur_letter_space) * cur_horiz_scaling);
                tom_line->append_unicodes(&uu, 1, (dx1 * tom_font_size + tom_letter_space) );
                //std::cout << (dx1 * tom_font_size + tom_letter_space) << ' ';
                /*
                 * In PDF, word_space is appended if (n == 1 and *p = ' ')
                 * but in HTML, word_space is appended if (uu == ' ')
                 */
                int space_count = (is_space ? 1 : 0) - ((uu == ' ') ? 1 : 0);
                if(space_count != 0)
                {
                    ////html_text_page.get_cur_line()->append_offset(cur_word_space * draw_text_scale * space_count); //????TOM
                    tom_line->append_offset(tom_word_space * draw_text_scale * space_count);
                    //std::cout << tom_word_space * draw_text_scale * space_count << ' ' << 1 << ' ' ;
                }
            }
        }

        dx += dx1;
        dy += dy1;

        ++nChars;
        p += n;
        len -= n;
    }

    std::cout << endl ;
    //std::cout << endl;// << tom_font_size << ' ' << tom_letter_space << ' ' << tom_word_space << ' ' << tom_horiz_scaling << ' ' << nChars << ' ' << nSpaces << endl ; 

    // horiz_scaling is merged into ctm now, 
    // so the coordinate system is ugly
    // TODO: why multiply cur_horiz_scaling here
    

    dx = (dx * cur_font_size + nChars * cur_letter_space + nSpaces * cur_word_space) * cur_horiz_scaling;
    //dx = (dx * tom_font_size + nChars * tom_letter_space + nSpaces * tom_word_space);
    //dx += (nChars * tom_letter_space + nSpaces * tom_letter_space) * tom_horiz_scaling;
    //std::cout << cur_letter_space << ' ' << tom_letter_space * tom_horiz_scaling << ' ' << cur_word_space << ' ' << tom_word_space * tom_horiz_scaling << endl ;
    
    dy *= cur_font_size;
    //dy *= tom_font_size;

    cur_tx += dx;
    cur_ty += dy;
        
    draw_tx += dx;
    draw_ty += dy;
}

/*double HTMLRenderer::TOM_getFontSize (GfxState * state) {

    GfxFont *tom_font;
    double *tom_fm;
    double tom_fontSize;
    char *tom_name;
    int tom_code;
    double tom_w;

    // adjust the font size
    tom_fontSize = state->getTransformedFontSize();
    if ((tom_font = state->getFont()) && tom_font->getType() == fontType3) {
        // This is a hack which makes it possible to deal with some Type 3
        // fonts.  The problem is that it's impossible to know what the
        // base coordinate system used in the font is without actually
        // rendering the font.  This code tries to guess by looking at the
        // width of the character 'm' (which breaks if the font is a
        // subset that doesn't contain 'm').
        for (tom_code = 0; tom_code < 256; ++tom_code) { if ((tom_name = ((Gfx8BitFont *)tom_font)->getCharName(tom_code)) && tom_name[0] == 'm' && tom_name[1] == '\0') { break; } }
        
        if (tom_code < 256) {
            tom_w = ((Gfx8BitFont *)tom_font)->getWidth(tom_code);
            if (tom_w != 0) {
                // 600 is a generic average 'm' width -- yes, this is a hack
                tom_fontSize *= tom_w / 0.6;
            }
        }
        tom_fm = tom_font->getFontMatrix();
        if (tom_fm[0] != 0) {
            tom_fontSize *= fabs(tom_fm[3] / tom_fm[0]);
        }
    }

    return tom_fontSize;
}*/

} // namespace pdf2htmlEX
