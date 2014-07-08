/*
 * image.cc
 *
 * Handling images
 *
 * by WangLu
 * 2012.08.14
 */

#include "HTMLRenderer.h"
#include "util/namespace.h"
#include "util/css_const.h"
#include <goo/ImgWriter.h>
#include <goo/PNGWriter.h>
#include <sstream>

namespace pdf2htmlEX {

using std::cerr;

void HTMLRenderer::drawImage(GfxState * state, Object * ref, Stream * str, int width, int height, GfxImageColorMap * colorMap, GBool interpolate, int *maskColors, GBool inlineImg)
{
    if(maskColors) return;

    unsigned char *row;
    unsigned char *rowp;
    Guchar *p;
    GfxRGB rgb;

    std::stringstream sstm;
    const char *filename;
    const char *filepath;
    FILE * f;
    std::unique_ptr<ImgWriter> writer;
    ImageStream *imgStr;

    image_count++;
    cerr << "Page '" << hex <<  HTMLRenderer::pageNum << dec << "' going through draw image " << image_count << ":" << endl;
    
    sstm << hex << "Image-" << HTMLRenderer::pageNum << "-" << dec << image_count << ".png";
    filename = sstm.str().c_str();
    sstm.str("");
    sstm.clear();
    sstm << param.dest_dir << "/" << filename;
    filepath = sstm.str().c_str();
    f = fopen(filepath, "wb");
    
    writer = std::unique_ptr<ImgWriter>(new PNGWriter);
    if(!writer->init(f, width, height, param.h_dpi, param.v_dpi))
        throw "Cannot initialize PNGWriter";
    
    imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
    imgStr->reset();

    row = (unsigned char *) gmallocn(width, sizeof(unsigned int));
    for (int y = 0; y < height; y++) {
        p = imgStr->getLine();
        rowp = row;
        for (int x = 0; x < width; ++x) {
        if (p) {
          colorMap->getRGB(p, &rgb);
          *rowp++ = colToByte(rgb.r);
          *rowp++ = colToByte(rgb.g);
          *rowp++ = colToByte(rgb.b);
          p += colorMap->getNumPixelComps();
        } else {
          *rowp++ = 0;
          *rowp++ = 0;
          *rowp++ = 0;
        }
      }
      writer->writeRow(&row);
    }
    gfree(row);
    imgStr->close();
    delete imgStr;
    writer->close();
    fclose(f);

    double ctm[6];
    memcpy(ctm, state->getCTM(), sizeof(ctm));
    //ctm[4] = ctm[5] = 0.0;

    auto & all_manager = HTMLRenderer::all_manager;
    
    //double h_scale = HTMLRenderer::text_zoom_factor() * DEFAULT_DPI / param.h_dpi;
    //double v_scale = HTMLRenderer::text_zoom_factor() * DEFAULT_DPI / param.v_dpi;

    //cerr << "1:" << ctm[0] << ":2:" << ctm[1] << ":3:" << ctm[2] << ":4:" << ctm[3] << ":5:" << ctm[4] << ":6:" << ctm[5] << endl;

    double left = ctm[4] ;
    double bottom = ctm[5] ;
    double cssWidth = ctm[0] ;
    double cssHeight = ctm[3] ; // TO BE BETTER MANAGED , SOME IMAGES ARE ROTATED


    (*f_curpage) << "<img class=\"" << CSS::IMAGE_CN
                 << " " << CSS::LEFT_CN             << all_manager.left.install(left)
                 << " " << CSS::BOTTOM_CN           << all_manager.bottom.install(bottom)
                 << " " << CSS::WIDTH_CN            << all_manager.width.install(cssWidth)
                 << " " << CSS::HEIGHT_CN           << all_manager.height.install(cssHeight)
                 //<< " " << CSS::TRANSFORM_MATRIX_CN << all_manager.transform_matrix.install(ctm)
                 << "\" src=\"" << filename << "\"/>" ;   

    return OutputDev::drawImage(state,ref,str,width,height,colorMap,interpolate,maskColors,inlineImg);
}
} // namespace pdf2htmlEX

#if 0
    if(maskColors)
        return;

    rgb8_image_t img(width, height);
    auto imgview = view(img);
    auto loc = imgview.xy_at(0,0);

    ImageStream * img_stream = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
    img_stream->reset();

    for(int i = 0; i < height; ++i)
    {
        auto p = img_stream->getLine();
        for(int j = 0; j < width; ++j)
        {
            GfxRGB rgb;
            colorMap->getRGB(p, &rgb);

            *loc = rgb8_pixel_t(colToByte(rgb.r), colToByte(rgb.g), colToByte(rgb.b));

            p += colorMap->getNumPixelComps();

            ++ loc.x();
        }

        loc = imgview.xy_at(0, i+1);
    }

    png_write_view((format("i%|1$x|.png")%image_count).str(), imgview);
    
    img_stream->close();
    delete img_stream;

    close_line();

    double ctm[6];
    memcpy(ctm, state->getCTM(), sizeof(ctm));
    ctm[4] = ctm[5] = 0.0;
    html_fout << format("<img class=\"i t%2%\" style=\"left:%3%px;bottom:%4%px;width:%5%px;height:%6%px;\" src=\"i%|1$x|.png\" />") % image_count % install_transform_matrix(ctm) % state->getCurX() % state->getCurY() % width % height << endl;


    ++ image_count;
#endif
