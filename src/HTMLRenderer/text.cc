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
        || ((font->getWMode()) && (!param.try_vertical))
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

    bool first_char = true;
    double first_x=0;
    double first_y=0;

    while (len > 0) 
    {
        auto n = font->getNextChar(p, len, &code, &u, &uLen, &dx1, &dy1, &ox, &oy);

        double tom_font_size = cur_text_state.font_size;
        double tom_letter_space = cur_text_state.letter_space ;
        double tom_word_space = cur_text_state.word_space;
        double tom_horiz_scaling = html_text_page.get_cur_line()->line_state.transform_matrix[0];

        if (font->getWMode() && param.try_vertical) {
            cerr << "Attempting vertical " << ox << ' ' << oy << ' ' << endl;
        }

        if(!(equal(ox, 0) && equal(oy, 0)) && !(state->getFont()->getWMode()))
        {
            cerr << "TODO: non-zero origins in non writing-mode = 1" << endl;
        }

        if (font->getWMode() && param.try_vertical) {
            // update position such that they will be recorded by text_line_buf
            if (first_char) { // UGLY, BUT IS WEIRDLY THE ONLY THING THAT WORKS TO INVESTIGATE
                first_x = cur_text_state.font_size * dy1 * cur_line_state.transform_matrix[2];
                first_y = cur_text_state.font_size * dy1 * cur_line_state.transform_matrix[3];
            }
            cur_line_state.x += cur_text_state.font_size * dy1 * cur_line_state.transform_matrix[2];
            cur_line_state.y += cur_text_state.font_size * dy1 * cur_line_state.transform_matrix[3];

            cur_line_state.x -= ox * cur_text_state.font_size * cur_line_state.transform_matrix[0]
                              + oy * cur_text_state.font_size * cur_line_state.transform_matrix[2] + first_x;
            cur_line_state.y -= ox * cur_text_state.font_size * cur_line_state.transform_matrix[1]
                              + oy * cur_text_state.font_size * cur_line_state.transform_matrix[3] + first_y;

            html_text_page.open_new_line(cur_line_state); // OFFSET IS REMOVED THEN PUT BACK AS APPROPRIATE ONLY BEFORE AND AFTER INSTALLING THE LINE

            cur_line_state.x += ox * cur_text_state.font_size * cur_line_state.transform_matrix[0]
                              + oy * cur_text_state.font_size * cur_line_state.transform_matrix[2] + first_x;
            cur_line_state.y += ox * cur_text_state.font_size * cur_line_state.transform_matrix[1]
                              + oy * cur_text_state.font_size * cur_line_state.transform_matrix[3] + first_y;

            cur_text_state.vertical_align = 0;

            //resync position
            draw_ty = cur_ty;
            draw_tx = cur_tx;    
            html_text_page.get_cur_line()->append_state(cur_text_state);        
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
            //html_text_page.get_cur_line()->append_offset((dx1 * tom_font_size + tom_letter_space + tom_word_space));
            if(font->getWMode() && param.try_vertical) {
                cur_line_state.y += (dy1 * cur_font_size + cur_letter_space + cur_word_space) * draw_text_scale ; // IS A SPACE IN VERTICAL MODE POSSIBLE ?
                cerr << "Space as offset " << (dy1 * cur_font_size + cur_letter_space + cur_word_space) * draw_text_scale;
            } else {
                html_text_page.get_cur_line()->append_offset((dx1 * cur_font_size + cur_letter_space + cur_word_space) * draw_text_scale,(dx1 * cur_font_size + cur_letter_space + cur_word_space) * draw_text_scale);
            }
        }
        else
        {
            if((param.decompose_ligature) && (uLen > 1) && all_of(u, u+uLen, isLegalUnicode))
            {
                // TODO: why multiply cur_horiz_scaling here?
                //html_text_page.get_cur_line()->append_unicodes(u, uLen, (dx1 * tom_font_size + tom_letter_space) );
                html_text_page.get_cur_line()->append_unicodes(u, uLen, (dx1 * cur_font_size + cur_letter_space) * cur_horiz_scaling,(dx1 * tom_font_size + tom_letter_space));
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
                //html_text_page.get_cur_line()->append_unicodes(&uu, 1, (dx1 * tom_font_size + tom_letter_space) );
                html_text_page.get_cur_line()->append_unicodes(&uu, 1, (dx1 * cur_font_size + cur_letter_space) * cur_horiz_scaling,(dx1 * tom_font_size + tom_letter_space));
                /*
                 * In PDF, word_space is appended if (n == 1 and *p = ' ')
                 * but in HTML, word_space is appended if (uu == ' ')
                 */
                int space_count = (is_space ? 1 : 0) - ((uu == ' ') ? 1 : 0);
                if(space_count != 0)
                {
                    //html_text_page.get_cur_line()->append_offset(tom_word_space * draw_text_scale * space_count);
                    if(font->getWMode() && param.try_vertical) {
                        cur_line_state.y -= cur_word_space * draw_text_scale * space_count ;
                        cerr << "End space " <<  -cur_word_space * draw_text_scale * space_count ;
                    } else {
                        html_text_page.get_cur_line()->append_offset(cur_word_space * draw_text_scale * space_count,cur_word_space * draw_text_scale * space_count);
                    }
                }
            }
        }

        dx += dx1;
        dy += dy1;

        ++nChars;
        p += n;
        len -= n;

        first_char = false;

    }

    // horiz_scaling is merged into ctm now, 
    // so the coordinate system is ugly
    // TODO: why multiply cur_horiz_scaling here
    dx = (dx * cur_font_size + nChars * cur_letter_space + nSpaces * cur_word_space) * cur_horiz_scaling;
    
    dy *= cur_font_size;

    cur_tx += dx;
    cur_ty += dy;
        
    draw_tx += dx;
    draw_ty += dy;
}
} // namespace pdf2htmlEX
